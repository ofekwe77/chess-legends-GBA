// Save system: format, integrity, alternating records, suspended matches (design sections 85-88, 97.16-17).
#include "test_util.h"

#include <cstring>

#include "cl_save.h"

using namespace test_util;

namespace
{
    // Simulated cartridge SRAM, deliberately filled with garbage (never assume zeroes).
    struct Sram
    {
        uint8_t bytes[cl::save_layout::total_size];

        explicit Sram(uint8_t fill = 0xFF)
        {
            for(int i = 0; i < cl::save_layout::total_size; ++i)
            {
                bytes[i] = uint8_t(fill ^ (i * 37));
            }
        }
    };

    cl::Profile sample_profile()
    {
        cl::Profile p;
        p.settings.music_volume = 3;
        p.settings.sfx_volume = 9;
        p.settings.music_on = false;
        p.settings.animations = 2;
        p.settings.text_speed = 3;
        p.settings.board_theme = 5;
        p.settings.high_contrast = true;
        p.settings.preferred_difficulty = 3;
        p.progress.encounters_cleared = (1u << 18) - 1;   // completed campaign
        for(int i = 0; i < cl::encounters_count; ++i)
        {
            p.progress.encounter_best[i] = uint8_t(5 + i * 13);
        }
        p.progress.achievements = 0x3FFFFFFFull;
        p.progress.challenge_medals = 0xFFFFu;
        p.progress.music_unlocked = 0x1FFF;
        p.progress.tutorial_lessons = 0x3FF;
        p.progress.gauntlet_best = 6;
        p.progress.gauntlet_cleared = true;
        p.progress.intro_seen = true;
        p.progress.ending_seen = true;
        p.progress.story_world = 5;
        p.stats.games_played = 123;
        p.stats.games_won = 77;
        p.stats.spells[cl::ABILITY_ROYAL_ESCAPE] = 4;
        p.stats.best_streak = 9;
        p.stats.best_difficulty_beaten = 4;
        return p;
    }

    cl::Profile& stored_profile()
    {
        static cl::Profile p;
        return p;
    }

    cl::SuspendedMatch& suspended()
    {
        static cl::SuspendedMatch m;
        return m;
    }

    cl::SuspendedMatch& suspended_copy()
    {
        static cl::SuspendedMatch m;
        return m;
    }

    cl::Game& second_game()
    {
        static cl::Game g;
        return g;
    }

    bool same_position(const cl::Position& a, const cl::Position& b)
    {
        char fa[160];
        char fb[160];
        a.write_fen(fa, sizeof(fa));
        b.write_fen(fb, sizeof(fb));
        return std::strcmp(fa, fb) == 0 && a.hash == b.hash && a.ply == b.ply;
    }
}

TEST_CASE("save", empty_and_garbage_sram_reads_as_empty)
{
    Sram sram(0xFF);
    cl::Profile profile;
    uint32_t sequence = 0;
    CHECK(cl::read_profile_record(sram.bytes + cl::save_layout::profile_a, cl::save_layout::profile_size, profile,
                                  sequence) == cl::RecordStatus::BLANK);
    Sram zeroes(0);
    CHECK(cl::read_profile_record(zeroes.bytes, cl::save_layout::profile_size, profile, sequence) ==
          cl::RecordStatus::BLANK);
    CHECK_EQ(cl::choose_record(cl::RecordStatus::BLANK, 0, cl::RecordStatus::BLANK, 0), -1);
}

TEST_CASE("save", profile_round_trip)
{
    Sram sram;
    const cl::Profile original = sample_profile();
    int size = cl::write_profile_record(original, 7, sram.bytes, cl::save_layout::profile_size);
    CHECK(size > cl::header_size);
    CHECK(size <= cl::save_layout::profile_size);
    cl::Profile& loaded = stored_profile();
    loaded = cl::Profile();
    uint32_t sequence = 0;
    CHECK(cl::read_profile_record(sram.bytes, cl::save_layout::profile_size, loaded, sequence) ==
          cl::RecordStatus::VALID);
    CHECK_EQ(sequence, 7u);
    CHECK(loaded == original);

    // Defaults also round-trip (fresh save creation).
    const cl::Profile fresh;
    CHECK(cl::write_profile_record(fresh, 1, sram.bytes, cl::save_layout::profile_size) > 0);
    CHECK(cl::read_profile_record(sram.bytes, cl::save_layout::profile_size, loaded, sequence) ==
          cl::RecordStatus::VALID);
    CHECK(loaded == fresh);
}

TEST_CASE("save", corruption_and_unsupported_versions)
{
    Sram sram;
    const cl::Profile original = sample_profile();
    int size = cl::write_profile_record(original, 3, sram.bytes, cl::save_layout::profile_size);
    cl::Profile loaded;
    uint32_t sequence = 0;

    // Flip every payload byte in turn: the CRC must reject each modification.
    for(int i = cl::header_size; i < size; ++i)
    {
        sram.bytes[i] ^= 0x5A;
        CHECK(cl::read_profile_record(sram.bytes, size, loaded, sequence) == cl::RecordStatus::CORRUPTED);
        sram.bytes[i] ^= 0x5A;
    }

    CHECK(cl::read_profile_record(sram.bytes, size, loaded, sequence) == cl::RecordStatus::VALID);

    // A newer format version is reported as unsupported (never misread).
    sram.bytes[4] = uint8_t(cl::save_version + 1);
    CHECK(cl::read_profile_record(sram.bytes, size, loaded, sequence) == cl::RecordStatus::UNSUPPORTED);
    sram.bytes[4] = uint8_t(cl::save_version);

    // A profile record is not accepted as a suspend record.
    cl::SuspendedMatch& match = suspended();
    CHECK(cl::read_suspend_record(sram.bytes, size, match, sequence) == cl::RecordStatus::CORRUPTED);

    // Out-of-range content with a valid CRC is rejected too.
    cl::Profile bad = original;
    bad.settings.music_volume = 200;
    size = cl::write_profile_record(bad, 4, sram.bytes, cl::save_layout::profile_size);
    CHECK(cl::read_profile_record(sram.bytes, size, loaded, sequence) == cl::RecordStatus::CORRUPTED);
}

TEST_CASE("save", alternating_records_survive_torn_writes)
{
    // Slot A holds generation 10, slot B generation 11. A write of generation 12 goes to A (the older slot); if it
    // is interrupted at any byte, B (generation 11) must still be chosen.
    Sram sram;
    cl::Profile gen11 = sample_profile();
    gen11.stats.games_played = 11;
    cl::Profile gen12 = sample_profile();
    gen12.stats.games_played = 12;
    uint8_t* a = sram.bytes + cl::save_layout::profile_a;
    uint8_t* b = sram.bytes + cl::save_layout::profile_b;
    cl::Profile gen10 = sample_profile();
    gen10.stats.games_played = 10;
    const int size_a = cl::write_profile_record(gen10, 10, a, cl::save_layout::profile_size);
    CHECK(size_a > 0);
    CHECK(cl::write_profile_record(gen11, 11, b, cl::save_layout::profile_size) > 0);

    static uint8_t full[cl::save_layout::profile_size];
    const int size12 = cl::write_profile_record(gen12, 12, full, cl::save_layout::profile_size);
    static uint8_t original_a[cl::save_layout::profile_size];
    std::memcpy(original_a, a, cl::save_layout::profile_size);

    for(int cut = 0; cut <= size12; ++cut)
    {
        std::memcpy(a, original_a, cl::save_layout::profile_size);
        std::memcpy(a, full, cut);   // power lost after `cut` bytes
        cl::Profile pa;
        cl::Profile pb;
        uint32_t sa = 0;
        uint32_t sb = 0;
        cl::RecordStatus status_a = cl::read_profile_record(a, cl::save_layout::profile_size, pa, sa);
        cl::RecordStatus status_b = cl::read_profile_record(b, cl::save_layout::profile_size, pb, sb);
        int chosen = cl::choose_record(status_a, sa, status_b, sb);
        CHECK(chosen >= 0);
        const cl::Profile& result = chosen == 0 ? pa : pb;

        // If the bytes now in slot A equal the complete new record (the unwritten tail happened to be identical
        // already), generation 12 is genuinely present; otherwise the newest complete record (11) must win and
        // the stale generation 10 must never be chosen.
        if(std::memcmp(a, full, size12) == 0)
        {
            CHECK_EQ(result.stats.games_played, 12u);
        }
        else
        {
            CHECK_EQ(result.stats.games_played, 11u);
        }
    }

    // Sequence numbers are compared wrap-safely.
    CHECK_EQ(cl::choose_record(cl::RecordStatus::VALID, 0xFFFFFFFFu, cl::RecordStatus::VALID, 0u), 1);
}

TEST_CASE("save", position_serialization_round_trip)
{
    const char* fens[] = {
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 mana=4/2 escape=1/0 home=c3/cf",
        "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1",
        "4k3/8/8/8/8/8/3P4/4K3 w - - 37 61 mana=6/6 escape=0/0 home=00/00",
    };

    for(const char* fen : fens)
    {
        cl::Position pos;
        bool arcane = std::strstr(fen, "mana=") != nullptr;
        CHECK(pos.set_fen(fen, arcane ? cl::RuleSet::arcane_rules() : cl::RuleSet::classic()) == cl::FenError::NONE);
        pos.ply = 77;
        uint8_t buffer[cl::position_bytes];
        CHECK_EQ(cl::write_position(pos, buffer), cl::position_bytes);
        cl::Position copy;
        CHECK(cl::read_position(buffer, copy));
        CHECK(same_position(pos, copy));
        CHECK(copy.validate());
        CHECK(copy.rules == pos.rules);
    }
}

TEST_CASE("save", suspended_arcane_match_resumes_exactly)
{
    // A match with castling rights, a used Royal Escape, spent mana and a repetition history.
    cl::Game& g = fresh_game();
    CHECK(setup(g, "r3k2r/pppp1ppp/8/4p3/4P3/8/PPPP1PPP/R3K2R w KQkq - 0 1 mana=6/6", true));
    const char* moves[] = { "h1g1", "h8g8", "g1h1", "g8h8", "ss:d2e2", "a7a6", "e1d1", "e8d8" };

    for(const char* move : moves)
    {
        CHECK(play(g, move));
    }

    cl::SuspendedMatch& match = suspended();
    match = cl::SuspendedMatch();
    match.mode = 1;
    match.human_mask = 1;
    match.difficulty = 2;
    match.match_seed = 0xDEADBEEF;
    cl::capture_game(g, match);

    static uint8_t sram[cl::save_layout::suspend_size];
    const int size = cl::write_suspend_record(match, 5, sram, cl::save_layout::suspend_size);
    CHECK(size > 0);

    cl::SuspendedMatch& loaded = suspended_copy();
    uint32_t sequence = 0;
    CHECK(cl::read_suspend_record(sram, size, loaded, sequence) == cl::RecordStatus::VALID);
    CHECK_EQ(loaded.match_seed, 0xDEADBEEFu);
    cl::Game& resumed = second_game();
    CHECK(cl::restore_game(loaded, resumed));

    // Identical state: position, hash, castling rights (lost K-side rights stay lost), mana (not regenerated
    // again), Royal Escape flags, counters and repetition counts.
    CHECK(same_position(g.position(), resumed.position()));
    CHECK_EQ(resumed.position().castling, g.position().castling);
    CHECK_EQ(resumed.position().mana[cl::WHITE], g.position().mana[cl::WHITE]);
    CHECK_EQ(resumed.position().mana[cl::BLACK], g.position().mana[cl::BLACK]);
    CHECK_EQ(resumed.repetition_count(), g.repetition_count());
    CHECK_EQ(resumed.position().halfmove, g.position().halfmove);

    // Both games continue identically afterwards.
    CHECK(play(g, "d1e1"));
    CHECK(play(resumed, "d1e1"));
    CHECK(same_position(g.position(), resumed.position()));
}

TEST_CASE("save", reload_during_check_and_after_regeneration)
{
    // Cross-system tests 16 and 17: Black is in check with mana that just regenerated to 5 (Royal Escape ready).
    cl::Game& g = fresh_game();
    CHECK(setup(g, "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1 mana=0/4", true));
    CHECK(play(g, "a1a8"));
    CHECK(g.position().in_check());
    CHECK_EQ(g.position().mana[cl::BLACK], 5);

    cl::SuspendedMatch& match = suspended();
    match = cl::SuspendedMatch();
    cl::capture_game(g, match);
    static uint8_t sram[cl::save_layout::suspend_size];
    const int size = cl::write_suspend_record(match, 1, sram, cl::save_layout::suspend_size);
    cl::SuspendedMatch& loaded = suspended_copy();
    uint32_t sequence = 0;
    CHECK(cl::read_suspend_record(sram, size, loaded, sequence) == cl::RecordStatus::VALID);
    cl::Game& resumed = second_game();
    CHECK(cl::restore_game(loaded, resumed));
    CHECK(resumed.position().in_check());
    CHECK_EQ(resumed.position().mana[cl::BLACK], 5);   // not regenerated a second time
    CHECK(is_legal(resumed, "re:g8g6"));
    CHECK(! resumed.over());

    // Royal Escape used, then saved: the flag survives.
    CHECK(play(resumed, "re:g8g6"));
    match = cl::SuspendedMatch();
    cl::capture_game(resumed, match);
    const int size2 = cl::write_suspend_record(match, 2, sram, cl::save_layout::suspend_size);
    CHECK(cl::read_suspend_record(sram, size2, loaded, sequence) == cl::RecordStatus::VALID);
    cl::Game& again = fresh_game();
    CHECK(cl::restore_game(loaded, again));
    CHECK_EQ(again.position().escapes[cl::BLACK], 0);
}

TEST_CASE("save", corrupted_suspend_records_are_rejected)
{
    cl::Game& g = fresh_game();
    g.start(cl::RuleSet::arcane_rules());
    CHECK(play(g, "e2e4"));
    CHECK(play(g, "e7e5"));
    cl::SuspendedMatch& match = suspended();
    match = cl::SuspendedMatch();
    cl::capture_game(g, match);
    static uint8_t sram[cl::save_layout::suspend_size];
    const int size = cl::write_suspend_record(match, 1, sram, cl::save_layout::suspend_size);
    cl::SuspendedMatch& loaded = suspended_copy();
    uint32_t sequence = 0;

    for(int i = cl::header_size; i < size; i += 3)
    {
        sram[i] ^= 0x81;
        CHECK(cl::read_suspend_record(sram, size, loaded, sequence) == cl::RecordStatus::CORRUPTED);
        sram[i] ^= 0x81;
    }

    // An illegal move list (valid CRC) is rejected by the replay.
    match.moves[0] = cl::Move::make(0x00, 0x77, cl::MK_QUIET);
    const int size2 = cl::write_suspend_record(match, 2, sram, cl::save_layout::suspend_size);
    CHECK(cl::read_suspend_record(sram, size2, loaded, sequence) == cl::RecordStatus::VALID);
    cl::Game& resumed = second_game();
    CHECK(! cl::restore_game(loaded, resumed));
}
