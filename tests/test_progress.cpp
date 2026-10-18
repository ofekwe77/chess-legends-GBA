// Achievements, statistics, gallery and unlock rules.
#include <cstring>

#include "cl_progress.h"
#include "test_util.h"

namespace
{
    using cl::Achievement;
    using cl::achievement_bit;

    cl::MatchRecorder& recorder()
    {
        static cl::MatchRecorder instance;
        return instance;
    }

    cl::MatchContext vs_computer(cl::MatchKind kind, cl::Difficulty difficulty = cl::Difficulty::KNIGHT,
                                 bool boss = false)
    {
        cl::MatchContext context;
        context.kind = kind;
        context.human_mask = 1;     // the person plays White
        context.difficulty = difficulty;
        context.boss = boss;
        return context;
    }

    // Plays one action through the game and the recorder, exactly as the match scene does.
    bool act(cl::Game& game, const char* coordinate)
    {
        cl::ActionInfo info;

        if(! test_util::play(game, coordinate, &info))
        {
            return false;
        }

        recorder().on_action(info, game);
        return true;
    }

    bool earned(Achievement achievement)
    {
        return recorder().earned() & achievement_bit(achievement);
    }
}

TEST_CASE("progress", achievement_table_is_complete)
{
    for(int i = 0; i < cl::achievements_count; ++i)
    {
        const cl::AchievementInfo& info = cl::achievement_info(Achievement(i));
        CHECK(info.key && info.key[0]);
        CHECK(info.title && info.title[0]);
        CHECK(info.description && info.description[0]);
        CHECK_EQ(int(info.icon), i);

        for(int j = 0; j < i; ++j)
        {
            CHECK(std::strcmp(info.key, cl::achievement_info(Achievement(j)).key) != 0);
            CHECK(std::strcmp(info.title, cl::achievement_info(Achievement(j)).title) != 0);
        }
    }

    CHECK(cl::achievements_count >= 24);
}

TEST_CASE("progress", move_events_award_person_actions_only)
{
    cl::Game& game = test_util::fresh_game();

    // Castling: the computer castling does not count, the person castling does.
    recorder().start(vs_computer(cl::MatchKind::CLASSIC));
    CHECK(test_util::setup(game, "r3k2r/8/8/8/8/8/8/R3K2R b KQkq - 0 1", false));
    CHECK(act(game, "e8c8"));
    CHECK(! earned(Achievement::SAFE_HARBOR));
    CHECK(act(game, "e1g1"));
    CHECK(earned(Achievement::SAFE_HARBOR));

    // En passant.
    recorder().start(vs_computer(cl::MatchKind::CLASSIC));
    CHECK(test_util::setup(game, "4k3/8/8/3pP3/8/8/8/4K3 w - d6 0 1", false));
    CHECK(act(game, "e5d6"));
    CHECK(earned(Achievement::IN_PASSING));
    CHECK(! earned(Achievement::FIRST_CHECK));

    // Promotion with check.
    recorder().start(vs_computer(cl::MatchKind::CLASSIC));
    CHECK(test_util::setup(game, "4k3/P7/8/8/8/8/8/4K3 w - - 0 1", false));
    CHECK(act(game, "a7a8q"));
    CHECK(earned(Achievement::CORONATION));
    CHECK(earned(Achievement::FIRST_CHECK));
    CHECK(! earned(Achievement::DOUBLE_TROUBLE));

    // Queen capture.
    recorder().start(vs_computer(cl::MatchKind::CLASSIC));
    CHECK(test_util::setup(game, "4k3/8/8/3q4/8/8/3R4/4K3 w - - 0 1", false));
    CHECK(act(game, "d2d5"));
    CHECK(earned(Achievement::QUEENSBANE));

    // Discovered double check: bishop e4-g6 uncovers the rook on e1.
    recorder().start(vs_computer(cl::MatchKind::CLASSIC));
    CHECK(test_util::setup(game, "4k3/8/8/8/4B3/8/8/4R1K1 w - - 0 1", false));
    CHECK(act(game, "e4g6"));
    CHECK(earned(Achievement::FIRST_CHECK));
    CHECK(earned(Achievement::DOUBLE_TROUBLE));

    // A spell, also during a challenge or a tutorial.
    recorder().start(vs_computer(cl::MatchKind::CHALLENGE));
    CHECK(test_util::setup(game, "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1 mana=2/0", true));
    CHECK(act(game, "ss:e2d2"));
    CHECK(earned(Achievement::FIRST_SPARK));
}

TEST_CASE("progress", victories_and_strategy_achievements)
{
    cl::Game& game = test_util::fresh_game();

    // Classic checkmate against a Grandmaster, nothing lost.
    recorder().start(vs_computer(cl::MatchKind::CLASSIC, cl::Difficulty::GRANDMASTER));
    CHECK(test_util::setup(game, "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1", false));
    CHECK(act(game, "d1d8"));
    CHECK(game.over());
    recorder().on_finish(game, true);
    CHECK(earned(Achievement::FIRST_VICTORY));
    CHECK(earned(Achievement::CLASSICIST));
    CHECK(earned(Achievement::CHECKMATE_ARTIST));
    CHECK(earned(Achievement::GIANT_SLAYER));
    CHECK(earned(Achievement::GRANDMASTER_SLAYER));
    CHECK(earned(Achievement::FLAWLESS));
    CHECK(! earned(Achievement::MANA_MISER));       // not an Arcane match
    CHECK(! earned(Achievement::UNSHAKEN));         // not a boss

    // Arcane checkmate while holding full mana; against a Knight there is no slayer award.
    recorder().start(vs_computer(cl::MatchKind::ARCANE, cl::Difficulty::KNIGHT));
    CHECK(test_util::setup(game, "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1 mana=6/0", true));
    CHECK(act(game, "d1d8"));
    recorder().on_finish(game, true);
    CHECK(earned(Achievement::MANA_MISER));
    CHECK(! earned(Achievement::CLASSICIST));
    CHECK(! earned(Achievement::GIANT_SLAYER));

    // A story boss won after casting Royal Escape: Great Escape, but not Unshaken.
    recorder().start(vs_computer(cl::MatchKind::STORY, cl::Difficulty::SORCERER, true));
    CHECK(test_util::setup(game, "6k1/5ppp/8/8/8/8/5PPP/4r1K1 w - - 0 1 mana=5/0", true));
    CHECK(act(game, "re:g1g3"));
    game.finish_objective(true, cl::WHITE);
    recorder().on_finish(game, true);
    CHECK(earned(Achievement::GREAT_ESCAPE));
    CHECK(! earned(Achievement::UNSHAKEN));
    CHECK(earned(Achievement::FIRST_VICTORY));

    // The same boss won without Royal Escape.
    recorder().start(vs_computer(cl::MatchKind::STORY, cl::Difficulty::SORCERER, true));
    CHECK(test_util::setup(game, "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1 mana=6/0", true));
    CHECK(act(game, "d1d8"));
    recorder().on_finish(game, true);
    CHECK(earned(Achievement::UNSHAKEN));

    // A failed story objective is not a victory.
    recorder().start(vs_computer(cl::MatchKind::STORY));
    CHECK(test_util::setup(game, "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1 mana=6/0", true));
    CHECK(act(game, "d1d8"));
    recorder().on_finish(game, false);
    CHECK(! earned(Achievement::FIRST_VICTORY));

    // Losing a piece spoils Flawless.
    recorder().start(vs_computer(cl::MatchKind::CLASSIC));
    CHECK(test_util::setup(game, "4k3/8/8/8/8/2n5/8/R2BK3 b - - 0 1", false));
    CHECK(act(game, "c3d1"));
    CHECK(act(game, "e1d1"));
    game.resign(cl::BLACK);
    recorder().on_finish(game, true);
    CHECK(earned(Achievement::FIRST_VICTORY));
    CHECK(! earned(Achievement::FLAWLESS));

    // Four different spells in a won match: Spellweaver and four spell achievements.
    recorder().start(vs_computer(cl::MatchKind::ARCANE));
    CHECK(test_util::setup(game, "4k3/8/8/8/8/8/8/R3K3 w - - 0 1", true));
    cl::ActionInfo info;
    const cl::Ability spells[4] = { cl::ABILITY_SHADOW_STEP, cl::ABILITY_PHASE_SHIFT, cl::ABILITY_GHOST_WALK,
                                    cl::ABILITY_CASTLE_SWAP };

    for(cl::Ability spell : spells)
    {
        info = cl::ActionInfo();
        info.mover = cl::WHITE;
        info.move = cl::Move::make(cl::A1, cl::make_square(0, 1), cl::ability_move_kind(spell));
        recorder().on_action(info, game);
    }

    game.resign(cl::BLACK);
    recorder().on_finish(game, true);
    CHECK(earned(Achievement::SPELLWEAVER));
    CHECK(earned(Achievement::SHADOW_STEPPER));
    CHECK(earned(Achievement::MOVING_WALLS));
    CHECK(! earned(Achievement::ROYAL_DECREE));
}

TEST_CASE("progress", modes_restrict_victory_achievements)
{
    cl::Game& game = test_util::fresh_game();

    // Solving a challenge is not a match victory.
    recorder().start(vs_computer(cl::MatchKind::CHALLENGE, cl::Difficulty::GRANDMASTER));
    CHECK(test_util::setup(game, "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1", false));
    CHECK(act(game, "d1d8"));
    recorder().on_finish(game, true);
    CHECK(! earned(Achievement::FIRST_VICTORY));
    CHECK(! earned(Achievement::GRANDMASTER_SLAYER));

    // Local two-player: Friendly Rivals, but no victory for "you".
    cl::MatchContext local;
    local.kind = cl::MatchKind::CLASSIC;
    local.human_mask = 3;
    recorder().start(local);
    CHECK(test_util::setup(game, "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1", false));
    CHECK(act(game, "d1d8"));
    recorder().on_finish(game, true);
    CHECK(earned(Achievement::FRIENDLY_RIVALS));
    CHECK(! earned(Achievement::FIRST_VICTORY));

    // Marathon: 160 actions (80 moves each side) in a finished match.
    recorder().start(vs_computer(cl::MatchKind::ARCANE));
    CHECK(test_util::setup(game, "4k3/8/8/8/8/8/8/R3K3 w - - 0 1", true));
    cl::ActionInfo quiet;
    quiet.mover = cl::BLACK;
    quiet.move = cl::Move::make(cl::E8, cl::make_square(4, 6), cl::MK_QUIET);

    for(int i = 0; i < 159; ++i)
    {
        recorder().on_action(quiet, game);
    }

    game.agree_draw();
    cl::MatchRecorder copy = recorder();
    copy.on_finish(game, false);
    CHECK(! (copy.earned() & achievement_bit(Achievement::MARATHON)));
    recorder().on_action(quiet, game);
    recorder().on_finish(game, false);
    CHECK(earned(Achievement::MARATHON));
}

TEST_CASE("progress", commit_updates_statistics_and_unlocks_once)
{
    cl::Game& game = test_util::fresh_game();
    static cl::Profile profile;
    profile = cl::Profile();

    recorder().start(vs_computer(cl::MatchKind::CLASSIC, cl::Difficulty::SORCERER));
    CHECK(test_util::setup(game, "4k3/8/8/3q4/8/8/3R4/4K3 w - - 0 1", false));
    CHECK(act(game, "d2d5"));
    game.resign(cl::BLACK);
    recorder().on_finish(game, true);
    const uint64_t first = recorder().commit(profile);
    CHECK(first & achievement_bit(Achievement::QUEENSBANE));
    CHECK(first & achievement_bit(Achievement::FIRST_VICTORY));
    CHECK_EQ(profile.stats.games_played, 1u);
    CHECK_EQ(profile.stats.games_won, 1u);
    CHECK_EQ(profile.stats.captures, 1u);
    CHECK_EQ(profile.stats.queens_captured, 1u);
    CHECK_EQ(int(profile.stats.win_streak), 1);
    CHECK_EQ(int(profile.stats.best_difficulty_beaten), 3);

    // A loss resets the streak but keeps the best streak; nothing is unlocked twice.
    recorder().start(vs_computer(cl::MatchKind::CLASSIC, cl::Difficulty::SORCERER));
    CHECK(test_util::setup(game, "4k3/8/8/3q4/8/8/3R4/4K3 w - - 0 1", false));
    CHECK(act(game, "d2d5"));
    game.resign(cl::WHITE);
    recorder().on_finish(game, false);
    const uint64_t second = recorder().commit(profile);
    CHECK(! (second & achievement_bit(Achievement::QUEENSBANE)));
    CHECK_EQ(profile.stats.games_played, 2u);
    CHECK_EQ(int(profile.stats.win_streak), 0);
    CHECK_EQ(int(profile.stats.best_streak), 1);
}

TEST_CASE("progress", recorder_pack_round_trip)
{
    cl::Game& game = test_util::fresh_game();
    recorder().start(vs_computer(cl::MatchKind::ARCANE));
    CHECK(test_util::setup(game, "4k3/8/8/3q4/8/8/3RP3/4K3 w - - 0 1 mana=6/0", true));
    CHECK(act(game, "ss:e2f2"));
    CHECK(act(game, "d5d6"));
    CHECK(act(game, "d2d6"));
    uint8_t packed[cl::MatchRecorder::packed_size];
    recorder().pack(packed);

    cl::MatchRecorder restored;
    restored.start(vs_computer(cl::MatchKind::ARCANE));
    CHECK(restored.unpack(packed));
    uint8_t again[cl::MatchRecorder::packed_size];
    restored.pack(again);
    CHECK(std::memcmp(packed, again, sizeof(packed)) == 0);

    // Commit of the restored recorder counts the same statistics.
    static cl::Profile a;
    static cl::Profile b;
    a = cl::Profile();
    b = cl::Profile();
    recorder().commit(a);
    restored.commit(b);
    CHECK(a.stats == b.stats);
    CHECK_EQ(a.stats.spells[cl::ABILITY_SHADOW_STEP], 1u);

    // Corrupted counters are rejected.
    packed[3] = 7;
    CHECK(! restored.unpack(packed));
}

TEST_CASE("progress", campaign_progress_and_unlocks)
{
    static cl::Profile profile;
    profile = cl::Profile();
    cl::SaveProgress& progress = profile.progress;

    CHECK(cl::encounter_unlocked(progress, 0));
    CHECK(! cl::encounter_unlocked(progress, 1));
    CHECK(cl::board_theme_unlocked(progress, 0));
    CHECK(cl::board_theme_unlocked(progress, 7));
    CHECK(! cl::board_theme_unlocked(progress, 1));

    // Only the hero, the mentor and the first kingdom are in the gallery at the start.
    CHECK_EQ(cl::gallery_unlocked_count(profile), 3);

    cl::record_encounter_cleared(progress, 0, 30);
    cl::record_encounter_cleared(progress, 1, 9);
    CHECK(cl::encounter_unlocked(progress, 2));
    CHECK(cl::gallery_unlocked(profile, cl::GallerySection::CHARACTERS, int(cl::Character::BRIN)));
    CHECK(! cl::gallery_unlocked(profile, cl::GallerySection::CHARACTERS, int(cl::Character::ROWAN)));
    CHECK_EQ(int(progress.story_world), 0);
    CHECK(! (cl::progress_achievements(profile) & achievement_bit(Achievement::WARDENS_FALL)));

    cl::record_encounter_cleared(progress, 2, 41);
    CHECK_EQ(int(progress.story_world), 1);
    CHECK(cl::board_theme_unlocked(progress, 1));
    CHECK(cl::gallery_unlocked(profile, cl::GallerySection::CHARACTERS, int(cl::Character::ROWAN)));
    CHECK(cl::gallery_unlocked(profile, cl::GallerySection::KINGDOMS, 1));
    CHECK(cl::progress_achievements(profile) & achievement_bit(Achievement::WARDENS_FALL));

    // Best move counts keep the minimum.
    cl::record_encounter_cleared(progress, 2, 55);
    CHECK_EQ(int(progress.encounter_best[2]), 41);
    cl::record_encounter_cleared(progress, 2, 33);
    CHECK_EQ(int(progress.encounter_best[2]), 33);

    for(int e = 3; e < 9; ++e)
    {
        cl::record_encounter_cleared(progress, e, 20);
    }

    CHECK_EQ(cl::bosses_defeated(progress), 3);
    CHECK(cl::progress_achievements(profile) & achievement_bit(Achievement::THREE_CROWNS));
    CHECK(! (cl::progress_achievements(profile) & achievement_bit(Achievement::CROWN_RESTORED)));

    for(int e = 9; e < cl::campaign_encounters; ++e)
    {
        cl::record_encounter_cleared(progress, e, 20);
    }

    CHECK_EQ(int(progress.story_world), 5);
    CHECK(cl::progress_achievements(profile) & achievement_bit(Achievement::CROWN_RESTORED));

    // Challenge medals keep the best; all gold earns Grand Tactician.
    cl::record_challenge_medal(progress, 4, 2);
    cl::record_challenge_medal(progress, 4, 1);
    CHECK_EQ(cl::challenge_medal_of(progress, 4), 2);
    CHECK(cl::progress_achievements(profile) & achievement_bit(Achievement::PUZZLE_SOLVER));

    for(int c = 0; c < cl::challenges_count; ++c)
    {
        cl::record_challenge_medal(progress, c, 3);
    }

    CHECK(cl::progress_achievements(profile) & achievement_bit(Achievement::GRAND_TACTICIAN));

    // Tutorial: every lesson is required.
    progress.tutorial_lessons = 0x1FF;
    CHECK(! (cl::progress_achievements(profile) & achievement_bit(Achievement::APT_PUPIL)));
    progress.tutorial_lessons = 0x3FF;
    CHECK(cl::progress_achievements(profile) & achievement_bit(Achievement::APT_PUPIL));

    // Lorekeeper needs every gallery entry: all music heard and every spell cast once.
    CHECK(! (cl::progress_achievements(profile) & achievement_bit(Achievement::LOREKEEPER)));
    progress.music_unlocked = (1u << cl::music_tracks_count) - 1;

    for(uint32_t& spell : profile.stats.spells)
    {
        spell = 1;
    }

    CHECK_EQ(cl::gallery_unlocked_count(profile), cl::gallery_entries);
    CHECK(cl::progress_achievements(profile) & achievement_bit(Achievement::LOREKEEPER));

    // unlock_achievements reports each bit once.
    const uint64_t fresh = cl::unlock_achievements(progress, cl::progress_achievements(profile));
    CHECK(fresh & achievement_bit(Achievement::LOREKEEPER));
    CHECK_EQ(cl::unlock_achievements(progress, cl::progress_achievements(profile)), 0ull);
}

TEST_CASE("progress", gallery_text_present)
{
    for(int c = 0; c < cl::gallery_characters; ++c)
    {
        CHECK(cl::character_lore(cl::Character(c))[0]);
        CHECK(c < 2 || cl::gallery_unlock_hint(cl::GallerySection::CHARACTERS, c)[0]);
    }

    for(int w = 0; w < cl::gallery_kingdoms; ++w)
    {
        CHECK(cl::kingdom_lore(w)[0]);
    }

    for(int t = 0; t < cl::gallery_music; ++t)
    {
        CHECK(cl::music_title(t)[0]);
    }
}
