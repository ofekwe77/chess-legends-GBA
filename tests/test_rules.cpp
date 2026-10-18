// Classic chess rules (design document section 94).
#include "test_util.h"

using namespace test_util;
using cl::GameResult;
using cl::ResultReason;

TEST_CASE("rules", piece_movement_counts)
{
    cl::Game& g = fresh_game();

    // Lone pieces on d4 (kings on a8 / h1 and a black pawn on h7, outside every line through d4; the pawn keeps
    // the material from being a dead position).
    CHECK(setup(g, "k7/7p/8/8/3N4/8/8/7K w - - 0 1", false));
    CHECK_EQ(count_legal(g), 8 + 3);    // knight 8 + king 3
    CHECK(setup(g, "k7/7p/8/8/3B4/8/8/7K w - - 0 1", false));
    CHECK_EQ(count_legal(g), 13 + 3);
    CHECK(setup(g, "k7/7p/8/8/3R4/8/8/7K w - - 0 1", false));
    CHECK_EQ(count_legal(g), 14 + 3);
    CHECK(setup(g, "k7/7p/8/8/3Q4/8/8/7K w - - 0 1", false));
    CHECK_EQ(count_legal(g), 27 + 3);
    CHECK(setup(g, "k7/7p/8/8/3K4/8/8/8 w - - 0 1", false));
    CHECK_EQ(count_legal(g), 8);
}

TEST_CASE("rules", pawn_movement)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4k3/8/8/8/8/8/4P3/4K3 w - - 0 1", false));
    CHECK(is_legal(g, "e2e3"));
    CHECK(is_legal(g, "e2e4"));
    CHECK(! is_legal(g, "e2e5"));
    CHECK(! is_legal(g, "e2d3"));   // diagonal only when capturing

    // Blocked pawn.
    CHECK(setup(g, "4k3/8/8/8/8/4n3/4P3/4K3 w - - 0 1", false));
    CHECK(! is_legal(g, "e2e3"));
    CHECK(! is_legal(g, "e2e4"));

    // Double step blocked on the second square.
    CHECK(setup(g, "4k3/8/8/8/4n3/8/4P3/4K3 w - - 0 1", false));
    CHECK(is_legal(g, "e2e3"));
    CHECK(! is_legal(g, "e2e4"));

    // Captures.
    CHECK(setup(g, "4k3/8/8/8/8/3r1b2/4P3/4K3 w - - 0 1", false));
    CHECK(is_legal(g, "e2d3"));
    CHECK(is_legal(g, "e2f3"));

    // Black pawns move down the board.
    CHECK(setup(g, "4k3/3p4/8/8/8/8/8/4K3 b - - 0 1", false));
    CHECK(is_legal(g, "d7d6"));
    CHECK(is_legal(g, "d7d5"));
    CHECK(! is_legal(g, "d7d8"));
}

TEST_CASE("rules", captures_and_pins)
{
    cl::Game& g = fresh_game();

    // The knight on e2 is pinned by the rook on e8: it may not move at all.
    CHECK(setup(g, "4r1k1/8/8/8/8/8/4N3/4K3 w - - 0 1", false));
    CHECK(! is_legal(g, "e2c3"));
    CHECK(! is_legal(g, "e2g3"));

    // A pinned rook may move along the pin line and capture the pinning piece.
    CHECK(setup(g, "4r1k1/8/8/8/8/8/4R3/4K3 w - - 0 1", false));
    CHECK(is_legal(g, "e2e5"));
    CHECK(is_legal(g, "e2e8"));
    CHECK(! is_legal(g, "e2d2"));

    cl::ActionInfo info;
    CHECK(play(g, "e2e8", &info));
    CHECK_EQ(info.captured, cl::B_ROOK);
}

TEST_CASE("rules", check_and_double_check)
{
    cl::Game& g = fresh_game();

    // Double check: only king moves are legal.
    CHECK(setup(g, "4k3/8/8/8/1b6/8/4r3/R3K2R w KQ - 0 1", false));
    CHECK(g.position().in_check());
    cl::MoveList list;
    g.legal_actions(list);

    CHECK(list.count > 0);

    for(int i = 0; i < list.count; ++i)
    {
        CHECK(cl::type_of(g.position().board[list.moves[i].from()]) == cl::KING);
    }

    // Discovered check.
    CHECK(setup(g, "4k3/8/8/8/8/8/4N3/4R1K1 w - - 0 1", false));
    cl::ActionInfo info;
    CHECK(play(g, "e2c3", &info));
    CHECK(info.check);
}

TEST_CASE("rules", checkmate_and_stalemate)
{
    cl::Game& g = fresh_game();

    // Fool's mate.
    g.start(cl::RuleSet::classic());
    CHECK(play(g, "f2f3"));
    CHECK(play(g, "e7e5"));
    CHECK(play(g, "g2g4"));
    CHECK(play(g, "d8h4"));
    CHECK(g.result() == GameResult::BLACK_WINS);
    CHECK(g.reason() == ResultReason::CHECKMATE);
    CHECK(! play(g, "a2a3"));   // no actions after the end

    // Stalemate.
    CHECK(setup(g, "7k/5Q2/6K1/8/8/8/8/8 b - - 0 1", false));
    CHECK(g.result() == GameResult::DRAW);
    CHECK(g.reason() == ResultReason::STALEMATE);

    // Back-rank mate delivered by a move.
    CHECK(setup(g, "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", false));
    CHECK(play(g, "a1a8"));
    CHECK(g.reason() == ResultReason::CHECKMATE);
    CHECK(g.result() == GameResult::WHITE_WINS);
}

TEST_CASE("rules", castling_requirements)
{
    cl::Game& g = fresh_game();
    const char* base = "r3k2r/pppppppp/8/8/8/8/PPPPPPPP/R3K2R w KQkq - 0 1";
    CHECK(setup(g, base, false));
    CHECK(is_legal(g, "e1g1"));
    CHECK(is_legal(g, "e1c1"));

    // Rook moved earlier -> right lost (and not restored by returning).
    CHECK(play(g, "h1g1"));
    CHECK(play(g, "a7a6"));
    CHECK(play(g, "g1h1"));
    CHECK(play(g, "a6a5"));
    CHECK(! is_legal(g, "e1g1"));
    CHECK(is_legal(g, "e1c1"));

    // King in check.
    CHECK(setup(g, "r3k2r/8/8/8/4r3/8/8/R3K2R w KQkq - 0 1", false));
    CHECK(! is_legal(g, "e1g1"));
    CHECK(! is_legal(g, "e1c1"));

    // Passing through an attacked square (f1 attacked by the rook on f8).
    CHECK(setup(g, "4kr2/8/8/8/8/8/8/R3K2R w KQ - 0 1", false));
    CHECK(! is_legal(g, "e1g1"));
    CHECK(is_legal(g, "e1c1"));

    // Finishing on an attacked square (g1).
    CHECK(setup(g, "4k1r1/8/8/8/8/8/8/R3K2R w KQ - 0 1", false));
    CHECK(! is_legal(g, "e1g1"));

    // Pieces in between (b1 occupied blocks queen side even though b1 is not crossed by the king).
    CHECK(setup(g, "4k3/8/8/8/8/8/8/RN2K2R w KQ - 0 1", false));
    CHECK(! is_legal(g, "e1c1"));
    CHECK(is_legal(g, "e1g1"));

    // b1 attacked does not prevent queen-side castling.
    CHECK(setup(g, "1r2k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", false));
    CHECK(is_legal(g, "e1c1"));

    // Castling moves the rook.
    CHECK(setup(g, base, false));
    CHECK(play(g, "e1g1"));
    CHECK_EQ(g.position().board[cl::F1], cl::W_ROOK);
    CHECK_EQ(g.position().board[cl::G1], cl::W_KING);
    CHECK(play(g, "e8c8"));
    CHECK_EQ(g.position().board[cl::D8], cl::B_ROOK);
    CHECK_EQ(g.position().board[cl::C8], cl::B_KING);
    CHECK_EQ(g.position().castling, 0);

    // Capturing a rook on its corner removes that right.
    CHECK(setup(g, "r3k2r/8/8/8/8/8/6B1/R3K2R w KQkq - 0 1", false));
    CHECK(play(g, "g2a8"));
    CHECK(! (g.position().castling & cl::CASTLE_BQ));
}

TEST_CASE("rules", en_passant)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4k3/8/8/8/3p4/8/4P3/4K3 w - - 0 1", false));
    CHECK(play(g, "e2e4"));
    CHECK(is_legal(g, "d4e3"));
    cl::ActionInfo info;
    CHECK(play(g, "d4e3", &info));
    CHECK_EQ(info.captured, cl::W_PAWN);
    CHECK_EQ(g.position().board[0x34], cl::EMPTY);   // e4 emptied

    // Only immediately after the double step.
    CHECK(setup(g, "4k3/8/8/8/3p4/8/4P3/4K3 w - - 0 1", false));
    CHECK(play(g, "e2e4"));
    CHECK(play(g, "e8d8"));
    CHECK(play(g, "e1d1"));
    CHECK(! is_legal(g, "d4e3"));

    // En passant that would expose the king along the rank is illegal.
    CHECK(setup(g, "8/8/8/8/k2p3R/8/4P3/4K3 w - - 0 1", false));
    CHECK(play(g, "e2e4"));
    CHECK(! is_legal(g, "d4e3"));
}

TEST_CASE("rules", promotion_and_underpromotion)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "1n2k3/P7/8/8/8/8/8/4K3 w - - 0 1", false));
    CHECK(is_legal(g, "a7a8q"));
    CHECK(is_legal(g, "a7a8r"));
    CHECK(is_legal(g, "a7a8b"));
    CHECK(is_legal(g, "a7a8n"));
    CHECK(is_legal(g, "a7b8q"));
    CHECK(is_legal(g, "a7b8n"));
    CHECK(! is_legal(g, "a7a8"));   // a promotion piece must be chosen
    CHECK(play(g, "a7b8n"));
    CHECK_EQ(g.position().board[cl::B8], cl::W_KNIGHT);
    CHECK_EQ(g.position().halfmove, 0);
}

TEST_CASE("rules", repetition_claims_and_fivefold)
{
    cl::Game& g = fresh_game();
    g.start(cl::RuleSet::classic());
    const char* cycle[4] = { "g1f3", "g8f6", "f3g1", "f6g8" };

    for(int i = 0; i < 4; ++i)
    {
        CHECK(play(g, cycle[i]));
    }

    CHECK_EQ(g.repetition_count(), 2);
    CHECK(! g.can_claim_threefold());

    for(int i = 0; i < 4; ++i)
    {
        CHECK(play(g, cycle[i]));
    }

    CHECK_EQ(g.repetition_count(), 3);
    CHECK(g.can_claim_threefold());
    CHECK(! g.over());   // threefold is a claim, not automatic

    for(int r = 0; r < 2; ++r)
    {
        for(int i = 0; i < 4; ++i)
        {
            CHECK(play(g, cycle[i]));
        }
    }

    CHECK(g.over());
    CHECK(g.reason() == ResultReason::FIVEFOLD_REPETITION);

    // Claiming.
    g.start(cl::RuleSet::classic());

    for(int r = 0; r < 2; ++r)
    {
        for(int i = 0; i < 4; ++i)
        {
            CHECK(play(g, cycle[i]));
        }
    }

    CHECK(g.claim_draw());
    CHECK(g.reason() == ResultReason::THREEFOLD_CLAIM);
}

TEST_CASE("rules", fifty_and_seventy_five_move_rules)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4k3/8/8/8/8/8/8/R3K3 w - - 98 60", false));
    CHECK(! g.can_claim_fifty());
    CHECK(play(g, "a1a2"));
    CHECK(play(g, "e8d8"));
    CHECK(g.can_claim_fifty());
    CHECK(! g.over());
    CHECK(g.claim_draw());
    CHECK(g.reason() == ResultReason::FIFTY_MOVE_CLAIM);

    CHECK(setup(g, "4k3/8/8/8/8/8/8/R3K3 w - - 149 90", false));
    CHECK(play(g, "a1a2"));
    CHECK(g.over());
    CHECK(g.reason() == ResultReason::SEVENTY_FIVE_MOVES);

    // Checkmate on the 150th ply takes precedence over the seventy-five-move rule.
    CHECK(setup(g, "6k1/5ppp/8/8/8/8/8/R5K1 w - - 149 90", false));
    CHECK(play(g, "a1a8"));
    CHECK(g.reason() == ResultReason::CHECKMATE);

    // A capture resets the counter.
    CHECK(setup(g, "4k3/8/8/8/8/8/r7/R3K3 w - - 120 90", false));
    CHECK(play(g, "a1a2"));
    CHECK_EQ(g.position().halfmove, 0);
}

TEST_CASE("rules", dead_positions)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4k3/8/8/8/8/8/8/4K3 w - - 0 1", false));
    CHECK(g.reason() == ResultReason::DEAD_POSITION);
    CHECK(setup(g, "4k3/8/8/8/8/8/8/2B1K3 w - - 0 1", false));
    CHECK(g.reason() == ResultReason::DEAD_POSITION);
    CHECK(setup(g, "4k3/8/8/8/8/8/8/1N2K3 w - - 0 1", false));
    CHECK(g.reason() == ResultReason::DEAD_POSITION);
    // Two knights can mate (with cooperation): not dead.
    CHECK(setup(g, "4k3/8/8/8/8/8/8/1N1NK3 w - - 0 1", false));
    CHECK(! g.over());
    // Bishops on the same color: dead. c1 and f8 are both dark squares.
    CHECK(setup(g, "4kb2/8/8/8/8/8/8/2B1K3 w - - 0 1", false));
    CHECK(g.reason() == ResultReason::DEAD_POSITION);
    // Opposite-colored bishops: mate is possible.
    CHECK(setup(g, "4k1b1/8/8/8/8/8/8/2B1K3 w - - 0 1", false));
    CHECK(! g.over());
    // Bishop vs knight: not dead.
    CHECK(setup(g, "4kn2/8/8/8/8/8/8/2B1K3 w - - 0 1", false));
    CHECK(! g.over());
    // Capturing into a dead position ends the game.
    CHECK(setup(g, "4k3/8/8/8/8/8/3r4/3QK3 w - - 0 1", false));
    CHECK(play(g, "d1d2"));
    CHECK(! g.over());   // K+Q vs K is not dead
    CHECK(setup(g, "4k3/8/8/8/8/8/3r4/3NK3 w - - 0 1", false));
    CHECK(play(g, "e1d2"));
    CHECK(g.reason() == ResultReason::DEAD_POSITION);
}

TEST_CASE("rules", resignation_and_agreement)
{
    cl::Game& g = fresh_game();
    g.start(cl::RuleSet::classic());
    g.resign(cl::WHITE);
    CHECK(g.result() == GameResult::BLACK_WINS);
    CHECK(g.reason() == ResultReason::RESIGNATION);
    g.start(cl::RuleSet::classic());
    g.agree_draw();
    CHECK(g.result() == GameResult::DRAW);
}

TEST_CASE("rules", fen_validation)
{
    cl::Position pos;
    CHECK(pos.set_fen("8/8/8/8/8/8/8/8 w - - 0 1", cl::RuleSet::classic()) == cl::FenError::KING_COUNT);
    CHECK(pos.set_fen("4k3/8/8/8/8/8/8/4K2P w - - 0 1", cl::RuleSet::classic()) == cl::FenError::PAWN_ON_BACK_RANK);
    CHECK(pos.set_fen("4k3/8/8/8/8/8/8/4K2R w - - 0 1", cl::RuleSet::classic()) == cl::FenError::NONE);
    CHECK(pos.set_fen("4k2R/8/8/8/8/8/8/4K3 w - - 0 1", cl::RuleSet::classic()) == cl::FenError::OPPONENT_IN_CHECK);
    CHECK(pos.set_fen("4k3/8/8/8/8/8/8/4K3 x - - 0 1", cl::RuleSet::classic()) == cl::FenError::SYNTAX);
    // Inconsistent castling rights are dropped.
    CHECK(pos.set_fen("4k3/8/8/8/8/8/8/4K3 w KQkq - 0 1", cl::RuleSet::classic()) == cl::FenError::NONE);
    CHECK_EQ(pos.castling, 0);
    // Round trip.
    char text[128];
    CHECK(pos.set_fen(cl::start_fen, cl::RuleSet::classic()) == cl::FenError::NONE);
    pos.write_fen(text, sizeof(text));
    CHECK(std::string_view(text) == cl::start_fen);
}

TEST_CASE("rules", san_notation)
{
    cl::Game& g = fresh_game();
    g.start(cl::RuleSet::classic());
    cl::Position pos = g.position();
    char text[32];
    cl::move_to_display(pos, cl::parse_coordinate(pos, "g1f3"), text, sizeof(text));
    CHECK(std::string_view(text) == "Nf3");
    CHECK(setup(g, "4k3/8/8/8/8/8/4K3/R6R w - - 0 1", false));
    pos = g.position();
    cl::move_to_display(pos, cl::parse_coordinate(pos, "a1d1"), text, sizeof(text));
    CHECK(std::string_view(text) == "Rad1");
    CHECK(setup(g, "4k3/8/8/8/8/8/8/R3K2R w KQ - 0 1", false));
    pos = g.position();
    cl::move_to_display(pos, cl::parse_coordinate(pos, "e1g1"), text, sizeof(text));
    CHECK(std::string_view(text) == "O-O");
    CHECK(setup(g, "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1", false));
    pos = g.position();
    cl::move_to_display(pos, cl::parse_coordinate(pos, "a1a8"), text, sizeof(text));
    CHECK(std::string_view(text) == "Ra8#");
}
