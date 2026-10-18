// Campaign data, scenario objectives and challenge content (the full proofs run in tools/validation/validate.cpp).
#include <cstring>

#include "cl_campaign.h"
#include "cl_movegen.h"
#include "cl_solver.h"
#include "cl_text_layout.h"
#include "cl_tutorial.h"
#include "test_util.h"

namespace
{
    cl::ObjectiveTracker& tracker()
    {
        static cl::ObjectiveTracker instance;
        return instance;
    }

    // Applies an action and feeds the tracker, as the match scene does.
    bool act(cl::Game& game, const char* coordinate)
    {
        cl::ActionInfo info;

        if(! test_util::play(game, coordinate, &info))
        {
            return false;
        }

        tracker().on_action(info, game);
        return true;
    }
}

TEST_CASE("campaign", every_encounter_sets_up)
{
    cl::Game& game = test_util::fresh_game();

    for(int i = 0; i < cl::campaign_encounters; ++i)
    {
        const cl::Encounter& e = cl::encounter(i);
        CHECK_MSG(cl::setup_encounter(e, game), e.title);
        CHECK_MSG(! game.over(), e.title);
        CHECK_MSG(game.position().side == e.player, e.title);
        CHECK_MSG(game.position().validate(), e.title);
        CHECK(e.intro.count > 0 && e.victory.count > 0 && e.defeat.count > 0);
        CHECK(e.world == i / 3);
    }

    for(int w = 0; w < cl::worlds_count; ++w)
    {
        CHECK(cl::encounter(w * 3 + 2).type == cl::EncounterType::BOSS);
        CHECK(cl::encounter(w * 3 + 2).opponent == cl::world(w).boss);
    }
}

TEST_CASE("campaign", every_gauntlet_stage_sets_up)
{
    cl::Game& game = test_util::fresh_game();

    for(int i = 0; i < cl::gauntlet_stages; ++i)
    {
        const cl::GauntletStage& stage = cl::gauntlet_stage(i);
        CHECK(cl::setup_gauntlet_stage(stage, game));
        CHECK(! game.over());
        CHECK(game.position().side == cl::WHITE);
        CHECK_EQ(int(game.position().mana[cl::WHITE]), 1);      // turn-start mana, as in a normal game
        CHECK_EQ(int(game.position().mana[cl::BLACK]), 0);
        CHECK(stage.boss == cl::world(i).boss);
    }
}

TEST_CASE("campaign", standard_encounters_regenerate_like_a_normal_game)
{
    cl::Game& game = test_util::fresh_game();
    CHECK(cl::setup_encounter(cl::encounter(2), game));     // boss with the standard start
    CHECK_EQ(int(game.position().mana[cl::WHITE]), 1);
    CHECK_EQ(int(game.position().mana[cl::BLACK]), 0);

    // Crown of Ash: mana cap 4 and sealed royal abilities for both sides.
    const cl::Encounter& ash = cl::encounter(15);
    CHECK(cl::setup_encounter(ash, game));
    CHECK_EQ(int(game.position().rules.max_mana), 4);
    CHECK(! (game.position().rules.ability_mask[cl::WHITE] & (1 << cl::ABILITY_ROYAL_COMMAND)));
    CHECK(! (game.position().rules.ability_mask[cl::BLACK] & (1 << cl::ABILITY_ROYAL_ESCAPE)));

    // Explicit FEN mana is used as written.
    CHECK(cl::setup_encounter(cl::encounter(1), game));
    CHECK_EQ(int(game.position().mana[cl::WHITE]), 2);
    CHECK_EQ(int(game.position().mana[cl::BLACK]), 0);
}

TEST_CASE("campaign", every_challenge_sets_up_with_a_legal_key)
{
    cl::Game& game = test_util::fresh_game();

    for(int i = 0; i < cl::challenges_count; ++i)
    {
        const cl::Challenge& c = cl::challenge(i);
        CHECK_MSG(cl::setup_challenge(c, game), c.title);
        CHECK_MSG(! game.over(), c.title);
        CHECK(c.par >= 1 && c.par <= c.objective.limit);
        CHECK_MSG(c.solution && test_util::is_legal(game, c.solution), c.title);
        CHECK(c.arcane || c.required_abilities == 0);

        if(c.required_abilities)
        {
            // The key of a magic puzzle is itself one of the required abilities, or prepares them.
            const cl::Move key = test_util::find(game, c.solution);
            CHECK(! key.is_null());
        }
    }
}

TEST_CASE("campaign", quick_proofs_of_one_move_challenges)
{
    cl::Game& game = test_util::fresh_game();

    for(int i = 0; i < cl::challenges_count; ++i)
    {
        const cl::Challenge& c = cl::challenge(i);

        if(c.par != 1 || c.objective.type != cl::Objective::CHECKMATE_WITHIN)
        {
            continue;
        }

        CHECK(cl::setup_challenge(c, game));
        cl::Position pos = game.position();
        cl::SolverStats stats;
        CHECK_MSG(cl::forced_mate(pos, 1, stats), c.title);

        // Each required ability is necessary.
        for(int a = 0; a < cl::ABILITIES_COUNT; ++a)
        {
            if(c.required_abilities & (1 << a))
            {
                cl::Position without = game.position();
                without.rules.ability_mask[without.side] &= uint8_t(~(1 << a));
                cl::SolverStats stats2;
                CHECK_MSG(! cl::forced_mate(without, 1, stats2), c.title);
            }
        }

        // The key mates at once.
        CHECK(test_util::play(game, c.solution));
        CHECK(game.over() && game.reason() == cl::ResultReason::CHECKMATE);
    }
}

TEST_CASE("campaign", material_objective_survives_a_dead_position)
{
    // The Royal Fork: winning the queen leaves king and knight against a bare king, a dead position that ends the
    // game at once. The objective is met all the same.
    cl::Game& game = test_util::fresh_game();
    const cl::Challenge& fork = cl::challenge(2);
    CHECK(std::strcmp(fork.title, "The Royal Fork") == 0);
    CHECK(cl::setup_challenge(fork, game));
    tracker().start(fork.objective, cl::WHITE, game.position());
    CHECK(act(game, "b5c7"));
    CHECK(tracker().status(game) == cl::ObjectiveStatus::ONGOING);
    CHECK(act(game, "e8d7"));
    CHECK(act(game, "c7a8"));
    CHECK(game.over());
    CHECK(game.reason() == cl::ResultReason::DEAD_POSITION);
    CHECK(tracker().status(game) == cl::ObjectiveStatus::SUCCESS);
}

TEST_CASE("campaign", objective_rules)
{
    cl::Game& game = test_util::fresh_game();

    // A checkmate completes every objective, also "promote a pawn".
    CHECK(test_util::setup(game, "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1", false));
    tracker().start({ cl::Objective::PROMOTE, 5, 0 }, cl::WHITE, game.position());
    CHECK(act(game, "d1d8"));
    CHECK(tracker().status(game) == cl::ObjectiveStatus::SUCCESS);

    // Checkmate within 1: a quiet move fails once the opponent has replied.
    CHECK(test_util::setup(game, "6k1/5ppp/8/8/8/8/5PPP/3R2K1 w - - 0 1", false));
    tracker().start({ cl::Objective::CHECKMATE_WITHIN, 1, 0 }, cl::WHITE, game.position());
    CHECK(act(game, "d1d2"));
    CHECK(tracker().status(game) == cl::ObjectiveStatus::ONGOING);
    CHECK(act(game, "g8f8"));
    CHECK(tracker().status(game) == cl::ObjectiveStatus::FAILED);

    // Survive 1: succeeds when the reply to the last move did not mate.
    CHECK(test_util::setup(game, "6k1/5ppp/8/8/8/8/5PPP/4r1K1 w - - 0 1 mana=5/0", true));
    tracker().start({ cl::Objective::SURVIVE, 1, 0 }, cl::WHITE, game.position());
    CHECK(act(game, "re:g1g3"));
    CHECK(tracker().status(game) == cl::ObjectiveStatus::ONGOING);
    CHECK(act(game, "e1e3"));
    CHECK(tracker().status(game) == cl::ObjectiveStatus::SUCCESS);

    // Promote: the promotion itself completes the objective.
    CHECK(test_util::setup(game, "4k3/P7/8/8/8/8/8/4K3 w - - 0 1", false));
    tracker().start({ cl::Objective::PROMOTE, 3, 0 }, cl::WHITE, game.position());
    CHECK(act(game, "a7a8q"));
    CHECK(tracker().status(game) == cl::ObjectiveStatus::SUCCESS);

    // Material lead is measured after the reply: an immediate recapture cancels it.
    CHECK(test_util::setup(game, "4k3/8/8/3p4/4p3/8/8/3RK3 w - - 0 1", false));
    tracker().start({ cl::Objective::WIN_MATERIAL, 2, 1 }, cl::WHITE, game.position());
    CHECK(act(game, "d1d5"));
    CHECK(tracker().status(game) == cl::ObjectiveStatus::ONGOING);
    CHECK(act(game, "e8e7"));
    CHECK(tracker().status(game) == cl::ObjectiveStatus::SUCCESS);

    // Pack / unpack keeps the tracker exact.
    const uint32_t packed = tracker().pack();
    cl::ObjectiveTracker restored;
    restored.start({ cl::Objective::WIN_MATERIAL, 2, 1 }, cl::WHITE, game.position());
    restored.unpack(packed);
    CHECK_EQ(restored.player_moves(), tracker().player_moves());
    CHECK(restored.status(game) == tracker().status(game));
}

TEST_CASE("campaign", tutorial_lessons_and_goals)
{
    cl::Game& game = test_util::fresh_game();

    for(int i = 0; i < cl::lessons_count; ++i)
    {
        const cl::Lesson& l = cl::lesson(i);
        CHECK_MSG(cl::setup_lesson(l, game), l.title);
        CHECK(! game.over());
        CHECK(game.position().side == cl::WHITE);
        CHECK(l.pages[0] && l.task && l.success);
    }

    // Action goals are recognised from the applied action only.
    const cl::Lesson& knight = cl::lesson(1);
    CHECK(cl::setup_lesson(knight, game));
    cl::ActionInfo info;
    CHECK(test_util::play(game, "g1f3", &info));
    CHECK(cl::lesson_action_completes(knight, info));
    CHECK(cl::setup_lesson(knight, game));
    CHECK(test_util::play(game, "b1c3", &info));
    CHECK(! cl::lesson_action_completes(knight, info));

    const cl::Lesson& capture = cl::lesson(3);
    CHECK(cl::setup_lesson(capture, game));
    CHECK(test_util::play(game, "d3f5", &info));
    CHECK(cl::lesson_action_completes(capture, info));

    const cl::Lesson& spell = cl::lesson(6);
    CHECK(cl::setup_lesson(spell, game));
    CHECK(test_util::play(game, "ss:d2e2", &info));
    CHECK(cl::lesson_action_completes(spell, info));
    CHECK(cl::setup_lesson(spell, game));
    CHECK(test_util::play(game, "d2d3", &info));
    CHECK(! cl::lesson_action_completes(spell, info));
}

TEST_CASE("campaign", text_layout_wraps_like_the_screen)
{
    cl::TextSpan lines[8];
    const char* text = "The quick brown fox jumps over the lazy dog";
    bool overflow = false;
    const int count = cl::wrap_lines(cl::FontId::SMALL, text, int(std::strlen(text)), 60, lines, 8, &overflow);
    CHECK(count >= 3);
    CHECK(! overflow);

    for(int i = 0; i < count; ++i)
    {
        CHECK(cl::text_pixel_width(cl::FontId::SMALL, text + lines[i].start, lines[i].length) <= 60);
        CHECK(lines[i].length == 0 || text[lines[i].start] != ' ');
    }

    // Overflow is reported when text remains.
    CHECK(cl::wrap_lines(cl::FontId::SMALL, text, int(std::strlen(text)), 60, lines, 2, &overflow) == 2);
    CHECK(overflow);

    // Explicit line breaks and missing glyphs.
    CHECK(cl::wrap_lines(cl::FontId::SMALL, "a\nb", 3, 200, lines, 8) == 2);
    CHECK(cl::font_supports(cl::FontId::SMALL, "Royal Escape \xE2\x97\x86"));     // "◆" is a font extra
    CHECK(! cl::font_supports(cl::FontId::SMALL, "caf\xC3\xA9"));                 // "é" is not in the font
    CHECK_EQ(cl::text_pixel_width(cl::FontId::SMALL, "caf\xC3\xA9"), -1);
}
