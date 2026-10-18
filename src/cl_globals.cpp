// CHESS LEGENDS: ARCANE WARS - console-side global state.
#include "cl_globals.h"

#include "bn_common.h"

#include "cl_progress.h"
#include "cl_storage.h"

namespace cl::gba
{

namespace
{
    BN_DATA_EWRAM_BSS Globals globals_storage;
    BN_DATA_EWRAM_BSS cl_test_interface test_interface_storage;
    uint32_t cosmetic_state = 0x2545F491u;
}

Globals& globals()
{
    return globals_storage;
}

cl_test_interface& test_interface()
{
    return test_interface_storage;
}

uint32_t cosmetic_random()
{
    cosmetic_state ^= cosmetic_state << 13;
    cosmetic_state ^= cosmetic_state >> 17;
    cosmetic_state ^= cosmetic_state << 5;
    return cosmetic_state;
}

bool save_profile_now()
{
    Globals& g = globals();
    SaveStats& stats = g.profile.stats;
    const uint32_t seconds = g.play_frames / 60;
    stats.play_seconds = stats.play_seconds + seconds < stats.play_seconds ? 0xFFFFFFFFu : stats.play_seconds + seconds;
    g.play_frames -= seconds * 60;

    // Progress facts may complete profile-level achievements (gallery, campaign, challenges).
    grant_achievements(progress_achievements(g.profile));

    const bool ok = save_profile(g.profile);
    g.save_failed = ! ok;
    test_interface().save_writes += ok;
    test_interface().save_status = ok ? 0 : 1;
    return ok;
}

uint64_t grant_achievements(uint64_t earned)
{
    Globals& g = globals();
    const uint64_t fresh = unlock_achievements(g.profile.progress, earned);
    g.toasts |= fresh;
    return fresh;
}

}
