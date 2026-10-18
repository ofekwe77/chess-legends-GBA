// CHESS LEGENDS: ARCANE WARS - cartridge SRAM persistence.
#include "cl_storage.h"

#include "bn_sram.h"

namespace cl::gba
{

namespace
{
    struct ProfileBlock
    {
        uint8_t bytes[save_layout::profile_size];
    };

    struct SuspendBlock
    {
        uint8_t bytes[save_layout::suspend_size];
    };

    // Record buffers live in EWRAM (the suspend block alone is 4 KB).
    BN_DATA_EWRAM_BSS ProfileBlock profile_block;
    BN_DATA_EWRAM_BSS ProfileBlock profile_check;
    BN_DATA_EWRAM_BSS SuspendBlock suspend_block;
    BN_DATA_EWRAM_BSS SuspendBlock suspend_check;

    // Newest valid copy of each kind: slot (0 = A, 1 = B, -1 = none) and its sequence number.
    int profile_slot = -1;
    uint32_t profile_sequence = 0;
    int suspend_slot = -1;
    uint32_t suspend_sequence = 0;
    bool suspend_scanned = false;

    constexpr int profile_offset(int slot)
    {
        return slot == 0 ? save_layout::profile_a : save_layout::profile_b;
    }

    constexpr int suspend_offset(int slot)
    {
        return slot == 0 ? save_layout::suspend_a : save_layout::suspend_b;
    }

    void scan_suspend()
    {
        if(suspend_scanned)
        {
            return;
        }

        suspend_scanned = true;
        SuspendedMatch& scratch = *new SuspendedMatch();
        uint32_t sequence[2] = {};
        RecordStatus status[2];

        for(int slot = 0; slot < 2; ++slot)
        {
            bn::sram::read_offset(suspend_block, suspend_offset(slot));
            status[slot] = read_suspend_record(suspend_block.bytes, save_layout::suspend_size, scratch,
                                               sequence[slot]);
        }

        delete &scratch;
        suspend_slot = choose_record(status[0], sequence[0], status[1], sequence[1]);
        suspend_sequence = suspend_slot >= 0 ? sequence[suspend_slot] : 0;
    }
}

LoadStatus load_profile(Profile& profile)
{
    RecordStatus status[2];
    uint32_t sequence[2] = {};
    Profile copies[2];

    for(int slot = 0; slot < 2; ++slot)
    {
        bn::sram::read_offset(profile_block, profile_offset(slot));
        status[slot] = read_profile_record(profile_block.bytes, save_layout::profile_size, copies[slot],
                                           sequence[slot]);
    }

    profile_slot = choose_record(status[0], sequence[0], status[1], sequence[1]);

    if(profile_slot < 0)
    {
        profile = Profile();
        profile_sequence = 0;
        const bool blank = status[0] == RecordStatus::BLANK && status[1] == RecordStatus::BLANK;

        // Leave damaged or newer-format data untouched until the next save writes over slot A.
        return blank ? LoadStatus::FRESH : LoadStatus::RESET;
    }

    profile = copies[profile_slot];
    profile_sequence = sequence[profile_slot];
    const RecordStatus other = status[profile_slot ^ 1];
    return other == RecordStatus::CORRUPTED || other == RecordStatus::UNSUPPORTED ? LoadStatus::RECOVERED :
                                                                                   LoadStatus::LOADED;
}

bool save_profile(const Profile& profile)
{
    const int slot = profile_slot < 0 ? 0 : profile_slot ^ 1;
    const uint32_t sequence = profile_sequence + 1;
    profile_block = ProfileBlock();
    const int size = write_profile_record(profile, sequence, profile_block.bytes, save_layout::profile_size);

    if(size <= 0)
    {
        return false;
    }

    bn::sram::write_offset(profile_block, profile_offset(slot));

    // Verify the copy actually stored before treating it as the newest one.
    Profile verify;
    uint32_t verify_sequence = 0;
    bn::sram::read_offset(profile_check, profile_offset(slot));

    if(read_profile_record(profile_check.bytes, save_layout::profile_size, verify, verify_sequence) !=
       RecordStatus::VALID || verify_sequence != sequence || ! (verify == profile))
    {
        return false;
    }

    profile_slot = slot;
    profile_sequence = sequence;
    return true;
}

bool has_suspended_match()
{
    scan_suspend();
    return suspend_slot >= 0;
}

bool load_suspended_match(SuspendedMatch& match)
{
    scan_suspend();

    if(suspend_slot < 0)
    {
        return false;
    }

    uint32_t sequence = 0;
    bn::sram::read_offset(suspend_block, suspend_offset(suspend_slot));
    return read_suspend_record(suspend_block.bytes, save_layout::suspend_size, match, sequence) ==
           RecordStatus::VALID;
}

bool save_suspended_match(const SuspendedMatch& match)
{
    scan_suspend();
    const int slot = suspend_slot < 0 ? 0 : suspend_slot ^ 1;
    const uint32_t sequence = suspend_sequence + 1;
    suspend_block = SuspendBlock();
    const int size = write_suspend_record(match, sequence, suspend_block.bytes, save_layout::suspend_size);

    if(size <= 0)
    {
        return false;
    }

    bn::sram::write_offset(suspend_block, suspend_offset(slot));
    bn::sram::read_offset(suspend_check, suspend_offset(slot));
    SuspendedMatch& verify = *new SuspendedMatch();
    uint32_t verify_sequence = 0;
    const bool ok = read_suspend_record(suspend_check.bytes, save_layout::suspend_size, verify, verify_sequence) ==
                    RecordStatus::VALID && verify_sequence == sequence;
    delete &verify;

    if(! ok)
    {
        return false;
    }

    suspend_slot = slot;
    suspend_sequence = sequence;
    return true;
}

void clear_suspended_match()
{
    // Erasing the signature of both copies turns them into blank records.
    for(int slot = 0; slot < 2; ++slot)
    {
        bn::sram::clear(16, suspend_offset(slot));
    }

    suspend_scanned = true;
    suspend_slot = -1;
}

void erase_all_saves()
{
    bn::sram::clear(save_layout::total_size, 0);
    profile_slot = -1;
    profile_sequence = 0;
    suspend_scanned = true;
    suspend_slot = -1;
    suspend_sequence = 0;
}

}
