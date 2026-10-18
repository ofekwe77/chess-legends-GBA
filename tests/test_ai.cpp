// Artificial intelligence: legality, determinism, tactics, magic awareness, cancellation (sections 37-45, 97).
#include "test_util.h"

#include "cl_ai.h"

using namespace test_util;

namespace
{
    cl::SearchMemory& memory()
    {
        static cl::SearchMemory storage;
        return storage;
    }

    cl::TranspositionTable& table()
    {
        static cl::TTEntry entries[1 << 14];
        static cl::TranspositionTable tt;
        static bool initialized = false;

        if(! initialized)
        {
            tt.init(entries, 1 << 14);
            initialized = true;
        }

        return tt;
    }

    cl::Ai& ai()
    {
        static cl::Ai instance(memory(), table());
        return instance;
    }

    cl::AiDecision think(cl::Game& game, cl::Difficulty difficulty, uint32_t seed = 1,
                         cl::Personality personality = cl::Personality::BALANCED)
    {
        cl::AiSettings settings;
        settings.difficulty = difficulty;
        settings.personality = personality;
        settings.seed = seed;
        return ai().choose(game, settings, false);
    }

    std::string_view coordinate(cl::Move move, char* buffer)
    {
        cl::move_to_coordinate(move, buffer, 24);
        return std::string_view(buffer);
    }
}

TEST_CASE("ai", ai_always_plays_legal_moves)
{
    const char* fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
    };

    for(int arcane = 0; arcane < 2; ++arcane)
    {
        for(const char* fen : fens)
        {
            for(int d = 0; d < int(cl::Difficulty::COUNT); ++d)
            {
                cl::Game& g = fresh_game();
                char buffer[160];
                std::snprintf(buffer, sizeof(buffer), "%s%s", fen, arcane ? " mana=4/4" : "");
                CHECK(setup(g, buffer, arcane != 0));
                cl::AiDecision decision = think(g, cl::Difficulty(d), 11);
                cl::Position pos = g.position();
                CHECK(cl::is_legal(pos, decision.move));
                CHECK(g.apply(decision.move));
            }
        }
    }
}

TEST_CASE("ai", search_is_deterministic_and_leak_free)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 mana=3/3", true));
    cl::AiDecision first = think(g, cl::Difficulty::SORCERER, 5);

    // A different search in between must not influence the next one (cross-system test 20).
    cl::Game other;
    other.start(cl::RuleSet::arcane_rules());
    (void) think(other, cl::Difficulty::GRANDMASTER, 9);

    cl::AiDecision second = think(g, cl::Difficulty::SORCERER, 5);
    CHECK(first.move == second.move);
    CHECK_EQ(first.nodes, second.nodes);
    CHECK_EQ(first.score, second.score);
}

TEST_CASE("ai", finds_mate_in_one_and_two)
{
    cl::Game& g = fresh_game();
    char text[24];

    CHECK(setup(g, "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", false));
    cl::AiDecision decision = think(g, cl::Difficulty::KNIGHT);
    CHECK(coordinate(decision.move, text) == "a1a8");

    // Re8+ Rxe8 Rxe8# (doubled rooks on the e-file).
    CHECK(setup(g, "r5k1/5ppp/8/8/8/8/4RPPP/4R1K1 w - - 0 1", false));
    decision = think(g, cl::Difficulty::SORCERER);
    CHECK(decision.score >= cl::SCORE_MATE_BOUND);
    CHECK(coordinate(decision.move, text) == "e2e8");
}

TEST_CASE("ai", finds_magical_mate)
{
    // Smothered mate that only Phase Shift can deliver (Ne6 -> f7).
    cl::Game& g = fresh_game();
    char text[24];
    CHECK(setup(g, "6rk/6pp/4N3/8/8/8/8/6K1 w - - 0 1 mana=3/0", true));
    cl::AiDecision decision = think(g, cl::Difficulty::KNIGHT);
    CHECK(coordinate(decision.move, text) == "ps:e6f7");
    CHECK(g.apply(decision.move));
    CHECK(g.reason() == cl::ResultReason::CHECKMATE);

    // Without the mana the same idea is impossible and the AI must not pretend otherwise.
    CHECK(setup(g, "6rk/6pp/4N3/8/8/8/8/6K1 w - - 0 1 mana=2/0", true));
    decision = think(g, cl::Difficulty::KNIGHT);
    CHECK(! decision.move.is_magic());
}

TEST_CASE("ai", uses_royal_escape_when_it_is_the_only_defense)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1 mana=0/4", true));
    CHECK(play(g, "a1a8"));
    cl::AiDecision decision = think(g, cl::Difficulty::SORCERER);
    CHECK(decision.move.kind() == cl::MK_ROYAL_ESCAPE);
    CHECK(g.apply(decision.move));
}

TEST_CASE("ai", wins_hanging_material)
{
    cl::Game& g = fresh_game();
    char text[24];
    CHECK(setup(g, "4k3/8/8/3q4/8/8/8/3QK3 w - - 0 1", false));

    for(int d = int(cl::Difficulty::KNIGHT); d < int(cl::Difficulty::COUNT); ++d)
    {
        cl::AiDecision decision = think(g, cl::Difficulty(d));
        CHECK(coordinate(decision.move, text) == "d1d5");
    }
}

TEST_CASE("ai", search_can_be_cancelled)
{
    cl::Game& g = fresh_game();
    g.start(cl::RuleSet::arcane_rules());
    cl::AiProfile profile = cl::make_ai_profile(cl::AiSettings{ cl::Difficulty::GRANDMASTER, cl::Personality::MASTER, 3, 100 });
    cl::Searcher searcher(memory(), table());
    int polls = 0;
    auto poll = [](void* user, uint32_t) -> bool
    {
        int& count = *static_cast<int*>(user);
        return ++count >= 3;   // cancel on the third poll
    };
    cl::SearchResult result = searcher.search(g.position(), g.window_hashes(), g.window_count(), profile.limits,
                                              profile.eval, poll, &polls);
    CHECK(result.aborted);
    CHECK(result.nodes < profile.limits.max_nodes);
    CHECK_EQ(polls, 3);
}

TEST_CASE("ai", personalities_and_difficulties_are_bounded)
{
    for(int d = 0; d < int(cl::Difficulty::COUNT); ++d)
    {
        for(int p = 0; p < int(cl::Personality::COUNT); ++p)
        {
            cl::AiProfile profile = cl::make_ai_profile(cl::AiSettings{ cl::Difficulty(d), cl::Personality(p), 1, 100 });
            const cl::EvalProfile& e = profile.eval;

            for(int value : { int(e.mobility), int(e.king_safety), int(e.pawn_structure), int(e.passed_pawns),
                              int(e.development), int(e.king_attack), int(e.piece_activity), int(e.mana),
                              int(e.magic_threats) })
            {
                CHECK(value >= 80 && value <= 150);
            }
        }
    }

    // Higher levels search more.
    CHECK(cl::make_ai_profile(cl::AiSettings{ cl::Difficulty::APPRENTICE }).limits.max_nodes <
          cl::make_ai_profile(cl::AiSettings{ cl::Difficulty::KNIGHT }).limits.max_nodes);
    CHECK(cl::make_ai_profile(cl::AiSettings{ cl::Difficulty::KNIGHT }).limits.max_nodes <
          cl::make_ai_profile(cl::AiSettings{ cl::Difficulty::SORCERER }).limits.max_nodes);
    CHECK(cl::make_ai_profile(cl::AiSettings{ cl::Difficulty::SORCERER }).limits.max_nodes <
          cl::make_ai_profile(cl::AiSettings{ cl::Difficulty::GRANDMASTER }).limits.max_nodes);
}

TEST_CASE("ai", opening_book_respects_personality)
{
    cl::Game& g = fresh_game();
    g.start(cl::RuleSet::classic());
    cl::AiSettings settings;
    settings.personality = cl::Personality::DEFENSIVE;
    cl::AiDecision decision = ai().choose(g, settings, true);
    CHECK(decision.from_book);
    char text[24];
    std::string_view move = coordinate(decision.move, text);
    CHECK(move == "d2d4" || move == "g1f3");
}
