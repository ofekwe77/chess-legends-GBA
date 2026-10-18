// Arcane Chess: mana, the six abilities and their interactions with the chess rules
// (design document sections 14-31, 96 and 97).
#include "test_util.h"

#include "cl_movegen.h"

using namespace test_util;
using cl::GameResult;
using cl::ResultReason;

// ---------------------------------------------------------------------------------------------------------------
// Mana
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("arcane", mana_start_regeneration_and_cap)
{
    cl::Game& g = fresh_game();
    g.start(cl::RuleSet::arcane_rules());
    CHECK_EQ(g.position().mana[cl::WHITE], 1);   // White already received the first turn-start point
    CHECK_EQ(g.position().mana[cl::BLACK], 0);
    CHECK(play(g, "g1f3"));
    CHECK_EQ(g.position().mana[cl::BLACK], 1);   // Black's first turn starts
    CHECK_EQ(g.position().mana[cl::WHITE], 1);
    CHECK(play(g, "g8f6"));
    CHECK_EQ(g.position().mana[cl::WHITE], 2);

    const char* cycle[4] = { "f3g1", "f6g8", "g1f3", "g8f6" };

    for(int i = 0; i < 16; ++i)
    {
        CHECK(play(g, cycle[i % 4]));
        CHECK(g.position().mana[cl::WHITE] <= 6);
        CHECK(g.position().mana[cl::BLACK] <= 6);
    }

    CHECK_EQ(g.position().mana[cl::WHITE], 6);   // excess regeneration is discarded
    CHECK_EQ(g.position().mana[cl::BLACK], 6);

    // Classic chess has no mana at all.
    g.start(cl::RuleSet::classic());
    CHECK_EQ(g.position().mana[cl::WHITE], 0);
    CHECK(play(g, "e2e4"));
    CHECK_EQ(g.position().mana[cl::BLACK], 0);
    CHECK_EQ(count_kind(g, cl::MK_SHADOW_STEP), 0);
}

TEST_CASE("arcane", no_mana_for_captures_or_checks)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4k3/8/8/8/8/8/3p4/3QK3 w - - 0 1 mana=1/1", true));
    CHECK(play(g, "d1d2"));   // capture
    CHECK_EQ(g.position().mana[cl::WHITE], 1);
    CHECK_EQ(g.position().mana[cl::BLACK], 2);   // regeneration only
}

TEST_CASE("arcane", illegal_actions_consume_nothing)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4r1k1/8/8/8/8/8/4P3/4K3 w - - 0 1 mana=2/0", true));
    cl::Move pinned_step = cl::Move::make(0x14, 0x13, cl::MK_SHADOW_STEP);   // e2-d2 exposes the king
    CHECK(! g.apply(pinned_step));
    CHECK_EQ(g.position().mana[cl::WHITE], 2);
    CHECK(g.position().side == cl::WHITE);
}

// ---------------------------------------------------------------------------------------------------------------
// Shadow Step
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("arcane", shadow_step)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4k3/8/8/8/8/8/3P4/4K3 w - - 10 1 mana=2/0", true));
    CHECK_EQ(count_kind(g, cl::MK_SHADOW_STEP), 2);
    cl::ActionInfo info;
    CHECK(play(g, "ss:d2c2", &info));
    CHECK_EQ(info.mana_spent, 2);
    CHECK_EQ(g.position().mana[cl::WHITE], 0);
    CHECK_EQ(g.position().mana[cl::BLACK], 1);
    CHECK(g.position().side == cl::BLACK);       // the spell consumed the whole turn
    CHECK_EQ(g.position().board[0x12], cl::W_PAWN);
    CHECK_EQ(g.position().board[0x13], cl::EMPTY);
    CHECK_EQ(g.position().halfmove, 0);           // pawn movement resets the halfmove counter
    CHECK_EQ(g.position().ep, cl::NO_SQUARE);     // magic never creates en passant rights
    CHECK(play(g, "e8d8"));
    CHECK(! is_legal(g, "c2c4"));                 // the stepped pawn lost its double step
    CHECK(is_legal(g, "c2c3"));

    // Insufficient mana.
    CHECK(setup(g, "4k3/8/8/8/8/8/3P4/4K3 w - - 0 1 mana=1/0", true));
    CHECK_EQ(count_kind(g, cl::MK_SHADOW_STEP), 0);

    // Board edge.
    CHECK(setup(g, "4k3/8/8/8/8/8/P7/4K3 w - - 0 1 mana=2/0", true));
    CHECK_EQ(count_kind(g, cl::MK_SHADOW_STEP), 1);
    CHECK(is_legal(g, "ss:a2b2"));

    // Occupied destinations (friendly and enemy): no capture.
    CHECK(setup(g, "4k3/8/8/8/8/8/2NPn3/4K3 w - - 0 1 mana=2/0", true));
    CHECK_EQ(count_kind(g, cl::MK_SHADOW_STEP), 0);

    // Self-check prevention: pinned pawn.
    CHECK(setup(g, "4r1k1/8/8/8/8/8/4P3/4K3 w - - 0 1 mana=2/0", true));
    CHECK_EQ(count_kind(g, cl::MK_SHADOW_STEP), 0);

    // No promotion through Shadow Step; the pawn keeps its forward direction.
    CHECK(setup(g, "4k3/1P6/8/8/8/8/8/4K3 w - - 0 1 mana=2/0", true));
    CHECK(play(g, "ss:b7a7"));
    CHECK_EQ(g.position().board[0x60], cl::W_PAWN);
    CHECK(play(g, "e8d7"));
    CHECK(is_legal(g, "a7a8q"));
}

// ---------------------------------------------------------------------------------------------------------------
// Phase Shift
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("arcane", phase_shift)
{
    cl::Game& g = fresh_game();
    // (The black h7 pawn keeps these minimal positions from being dead positions.)
    CHECK(setup(g, "4k3/7p/8/8/3N4/8/8/4K3 w - - 7 1 mana=3/0", true));
    CHECK_EQ(count_kind(g, cl::MK_PHASE_SHIFT), 8);
    CHECK(play(g, "ps:d4e5"));
    CHECK_EQ(g.position().mana[cl::WHITE], 0);
    CHECK_EQ(g.position().halfmove, 8);   // non-pawn magic does not reset the counter
    CHECK_EQ(g.position().board[0x44], cl::W_KNIGHT);

    CHECK(setup(g, "4k3/7p/8/8/3N4/8/8/4K3 w - - 0 1 mana=2/0", true));
    CHECK_EQ(count_kind(g, cl::MK_PHASE_SHIFT), 0);

    // Corner.
    CHECK(setup(g, "4k3/7p/8/8/8/8/8/N3K3 w - - 0 1 mana=3/0", true));
    CHECK_EQ(count_kind(g, cl::MK_PHASE_SHIFT), 3);

    // Occupied neighbours (own pawn d4, enemy pawn c4) are not destinations: no capture.
    CHECK(setup(g, "4k3/8/8/8/2pP4/3N4/8/4K3 w - - 0 1 mana=3/0", true));
    CHECK_EQ(count_kind(g, cl::MK_PHASE_SHIFT), 6);
    CHECK(! is_legal(g, "ps:d3c4"));

    // A Phase Shift that creates a normal knight check.
    CHECK(setup(g, "6k1/p7/8/4N3/8/8/8/4K3 w - - 0 1 mana=3/0", true));
    cl::ActionInfo info;
    CHECK(play(g, "ps:e5f6", &info));
    CHECK(info.check);
    CHECK(g.position().in_check());

    // Pinned knight: only the square that keeps blocking the file is legal.
    CHECK(setup(g, "4r1k1/8/8/8/8/8/4N3/4K3 w - - 0 1 mana=3/0", true));
    CHECK_EQ(count_kind(g, cl::MK_PHASE_SHIFT), 1);
    CHECK(is_legal(g, "ps:e2e3"));
}

// ---------------------------------------------------------------------------------------------------------------
// Ghost Walk
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("arcane", ghost_walk)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4k3/8/8/8/8/8/3P4/2B1K3 w - - 0 1 mana=3/0", true));
    CHECK_EQ(count_kind(g, cl::MK_GHOST_WALK), 4);   // e3 f4 g5 h6 through the d2 pawn
    CHECK(play(g, "gw:c1f4"));
    CHECK_EQ(g.position().board[0x35], cl::W_BISHOP);
    CHECK_EQ(g.position().mana[cl::WHITE], 0);

    // Second blocker stops the walk.
    CHECK(setup(g, "4k3/8/8/8/5P2/8/3P4/2B1K3 w - - 0 1 mana=3/0", true));
    CHECK_EQ(count_kind(g, cl::MK_GHOST_WALK), 1);
    CHECK(is_legal(g, "gw:c1e3"));

    // Enemy first blocker: no ghost walk (the normal capture remains).
    CHECK(setup(g, "4k3/8/8/8/8/8/3n4/2B1K3 w - - 0 1 mana=3/0", true));
    CHECK_EQ(count_kind(g, cl::MK_GHOST_WALK), 0);
    CHECK(is_legal(g, "c1d2"));

    // Enemy second blocker: cannot be captured by the walk.
    CHECK(setup(g, "4k3/8/8/8/5n2/8/3P4/2B1K3 w - - 0 1 mana=3/0", true));
    CHECK_EQ(count_kind(g, cl::MK_GHOST_WALK), 1);
    CHECK(! is_legal(g, "gw:c1f4"));

    // Insufficient mana.
    CHECK(setup(g, "4k3/8/8/8/8/8/3P4/2B1K3 w - - 0 1 mana=2/0", true));
    CHECK_EQ(count_kind(g, cl::MK_GHOST_WALK), 0);

    // Leaving a pin line is illegal; without the pin the same walks are legal.
    CHECK(setup(g, "4r1k1/8/8/8/8/3P4/4B3/4K3 w - - 0 1 mana=3/0", true));
    CHECK_EQ(count_kind(g, cl::MK_GHOST_WALK), 0);
    CHECK(setup(g, "6k1/8/8/8/8/3P4/4B3/4K3 w - - 0 1 mana=3/0", true));
    CHECK_EQ(count_kind(g, cl::MK_GHOST_WALK), 3);

    // After the walk the bishop attacks normally (here it gives check from b5).
    CHECK(setup(g, "4k3/8/8/8/8/3P4/4B3/4K3 w - - 0 1 mana=3/0", true));
    cl::ActionInfo info;
    CHECK(play(g, "gw:e2b5", &info));
    CHECK(info.check);
}

// ---------------------------------------------------------------------------------------------------------------
// Castle Swap
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("arcane", castle_swap)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4k3/8/8/8/8/8/8/RN2K3 w Q - 3 1 mana=4/0", true));
    CHECK(play(g, "cs:a1b1"));
    CHECK_EQ(g.position().board[cl::A1], cl::W_KNIGHT);
    CHECK_EQ(g.position().board[cl::B1], cl::W_ROOK);
    CHECK_EQ(g.position().castling, 0);   // the rook moved: castling right lost
    CHECK_EQ(g.position().mana[cl::WHITE], 0);
    CHECK_EQ(g.position().halfmove, 4);

    // Distances 2 and 3 with empty squares between; distance 4 is too far; blockers stop it.
    CHECK(setup(g, "4k3/8/8/8/8/7B/8/4K2R w K - 0 1 mana=4/0", true));
    CHECK(is_legal(g, "cs:h1h3"));
    CHECK(setup(g, "4k3/8/8/8/7B/8/8/4K2R w K - 0 1 mana=4/0", true));
    CHECK(is_legal(g, "cs:h1h4"));
    CHECK(setup(g, "3k4/8/8/7B/8/8/8/4K2R w K - 0 1 mana=4/0", true));
    CHECK_EQ(count_kind(g, cl::MK_CASTLE_SWAP), 0);
    CHECK(setup(g, "4k3/8/8/8/8/7B/7P/4K2R w K - 0 1 mana=4/0", true));
    CHECK_EQ(count_kind(g, cl::MK_CASTLE_SWAP), 0);

    // Only friendly bishops and knights.
    CHECK(setup(g, "4k3/8/8/8/8/8/8/RQ2K3 w Q - 0 1 mana=4/0", true));
    CHECK_EQ(count_kind(g, cl::MK_CASTLE_SWAP), 0);
    CHECK(setup(g, "4k3/8/8/8/8/8/8/Rb2K3 w Q - 0 1 mana=4/0", true));
    CHECK_EQ(count_kind(g, cl::MK_CASTLE_SWAP), 0);

    // Insufficient mana.
    CHECK(setup(g, "4k3/8/8/8/8/8/8/RN2K3 w Q - 0 1 mana=3/0", true));
    CHECK_EQ(count_kind(g, cl::MK_CASTLE_SWAP), 0);

    // A swap can create check through the normal attack map (discovered rook line).
    CHECK(setup(g, "3k4/8/8/8/8/3N4/8/3R2K1 w - - 0 1 mana=4/0", true));
    cl::ActionInfo info;
    CHECK(play(g, "cs:d1d3", &info));
    CHECK(info.check);

    // A swap never resolves check (both squares stay occupied): not available while in check.
    CHECK(setup(g, "4r1k1/8/8/8/8/8/8/RN2K3 w - - 0 1 mana=4/0", true));
    CHECK(g.position().in_check());
    CHECK_EQ(count_kind(g, cl::MK_CASTLE_SWAP), 0);
}

// ---------------------------------------------------------------------------------------------------------------
// Royal Command
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("arcane", royal_command)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4k3/8/8/8/8/8/2N5/3QK3 w - - 0 1 mana=5/0", true));
    CHECK_EQ(count_kind(g, cl::MK_ROYAL_COMMAND), 2);
    CHECK(is_legal(g, "rc:c2c1@d1"));
    CHECK(is_legal(g, "rc:c2d2@d1"));
    CHECK(play(g, "rc:c2d2@d1"));
    CHECK_EQ(g.position().board[0x13], cl::W_KNIGHT);
    CHECK_EQ(g.position().board[cl::D1], cl::W_QUEEN);   // the queen stays where she is
    CHECK_EQ(g.position().mana[cl::WHITE], 0);

    // Never the king, another queen or an enemy.
    CHECK(setup(g, "4k3/8/8/8/8/8/3Q4/3QK3 w - - 0 1 mana=5/0", true));
    {
        cl::MoveList list;
        g.legal_actions(list);

        for(int i = 0; i < list.count; ++i)
        {
            if(list.moves[i].kind() == cl::MK_ROYAL_COMMAND)
            {
                cl::PieceType type = cl::type_of(g.position().board[list.moves[i].from()]);
                CHECK(type != cl::KING && type != cl::QUEEN);
            }
        }
    }

    CHECK(setup(g, "4k3/8/8/8/8/8/2n5/3QK3 w - - 0 1 mana=5/0", true));
    CHECK_EQ(count_kind(g, cl::MK_ROYAL_COMMAND), 0);

    // A commanded pawn counts as moved and resets the halfmove counter (cross-system test 4).
    CHECK(setup(g, "4k3/8/8/8/8/8/4P3/3QK3 w - - 9 1 mana=5/0", true));
    CHECK_EQ(count_kind(g, cl::MK_ROYAL_COMMAND), 1);
    CHECK(play(g, "rc:e2d2@d1"));
    CHECK_EQ(g.position().halfmove, 0);
    CHECK(play(g, "e8f8"));
    CHECK(! is_legal(g, "d2d4"));
    CHECK(is_legal(g, "d2d3"));

    // Never onto the promotion rank (nor the first rank).
    CHECK(setup(g, "k7/3PQ3/8/8/8/8/8/4K3 w - - 0 1 mana=5/0", true));
    CHECK_EQ(count_kind(g, cl::MK_ROYAL_COMMAND), 2);
    CHECK(! is_legal(g, "rc:d7d8@e7"));
    CHECK(is_legal(g, "rc:d7d6@e7"));
    CHECK(setup(g, "3k4/8/8/8/8/8/3P4/4QK2 w - - 0 1 mana=5/0", true));
    CHECK(! is_legal(g, "rc:d2d1@e1"));
    CHECK(is_legal(g, "rc:d2e2@e1"));

    // A commanded rook loses its castling right, and returning never restores it (cross-system test 3).
    CHECK(setup(g, "4k3/8/8/8/8/8/6Q1/4K2R w K - 0 1 mana=5/0", true));
    CHECK(play(g, "rc:h1h2@g2"));
    CHECK_EQ(g.position().castling, 0);
    CHECK(play(g, "e8d8"));
    CHECK(play(g, "h2h1"));
    CHECK(play(g, "d8e8"));
    CHECK(! is_legal(g, "e1g1"));

    // A pinned ally cannot be commanded off the pin line.
    CHECK(setup(g, "4r1k1/8/8/8/8/8/4N3/3QK3 w - - 0 1 mana=5/0", true));
    CHECK_EQ(count_kind(g, cl::MK_ROYAL_COMMAND), 0);

    // Royal Command resolves check by blocking (cross-system test 9).
    CHECK(setup(g, "4r1k1/8/8/8/3N4/3Q4/8/4K3 w - - 0 1 mana=5/0", true));
    CHECK(g.position().in_check());
    CHECK_EQ(count_kind(g, cl::MK_ROYAL_COMMAND), 2);
    CHECK(is_legal(g, "rc:d4e3@d3"));
    CHECK(is_legal(g, "rc:d4e4@d3"));
    CHECK(! is_legal(g, "rc:d4c4@d3"));
}

// ---------------------------------------------------------------------------------------------------------------
// Royal Escape
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("arcane", royal_escape)
{
    cl::Game& g = fresh_game();

    // Not in check: unavailable.
    CHECK(setup(g, "4k3/8/8/8/8/8/8/R3K3 w - - 0 1 mana=5/0", true));
    CHECK_EQ(count_kind(g, cl::MK_ROYAL_ESCAPE), 0);

    // In check: leaps of exactly two squares onto empty, unattacked squares.
    CHECK(setup(g, "4k3/8/8/8/8/8/4P3/r3K3 w - - 0 1 mana=5/0", true));
    CHECK(g.position().in_check());
    CHECK(is_legal(g, "re:e1e3"));   // jumps over the occupied e2 square
    CHECK(is_legal(g, "re:e1g3"));
    CHECK(is_legal(g, "re:e1c3"));
    CHECK(! is_legal(g, "re:e1g1"));  // still on the rook's rank
    CHECK(! is_legal(g, "re:e1c1"));
    CHECK_EQ(count_kind(g, cl::MK_ROYAL_ESCAPE), 3);

    // Insufficient mana / already used.
    CHECK(setup(g, "4k3/8/8/8/8/8/4P3/r3K3 w - - 0 1 mana=4/0", true));
    CHECK_EQ(count_kind(g, cl::MK_ROYAL_ESCAPE), 0);
    CHECK(setup(g, "4k3/8/8/8/8/8/4P3/r3K3 w - - 0 1 mana=6/0 escape=0/1", true));
    CHECK_EQ(count_kind(g, cl::MK_ROYAL_ESCAPE), 0);

    // Occupied destination.
    CHECK(setup(g, "4k3/8/8/8/8/4N3/4P3/r3K3 w - - 0 1 mana=5/0", true));
    CHECK(! is_legal(g, "re:e1e3"));

    // Removes castling rights permanently and uses the single charge.
    CHECK(setup(g, "4k3/8/8/8/1b6/8/8/R3K2R w KQ - 0 1 mana=5/0", true));
    CHECK_EQ(count_kind(g, cl::MK_ROYAL_ESCAPE), 4);
    CHECK(play(g, "re:e1e3"));
    CHECK_EQ(g.position().castling, 0);
    CHECK_EQ(g.position().escapes[cl::WHITE], 0);
    CHECK_EQ(g.position().mana[cl::WHITE], 0);
}

// ---------------------------------------------------------------------------------------------------------------
// Checkmate and stalemate with magical defenses
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("arcane", checkmate_considers_royal_escape)
{
    cl::Game& g = fresh_game();

    // Black has 3 mana: turn-start regeneration gives 4, not enough for Royal Escape -> checkmate.
    CHECK(setup(g, "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1 mana=0/3", true));
    CHECK(play(g, "a1a8"));
    CHECK(g.reason() == ResultReason::CHECKMATE);

    // Black has 4 mana: regeneration makes Royal Escape affordable -> not checkmate (cross-system tests 1, 12).
    CHECK(setup(g, "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1 mana=0/4", true));
    CHECK(play(g, "a1a8"));
    CHECK(! g.over());
    CHECK_EQ(g.position().mana[cl::BLACK], 5);
    CHECK(is_legal(g, "re:g8g6"));
    CHECK(play(g, "re:g8g6"));
    CHECK_EQ(g.position().escapes[cl::BLACK], 0);

    // Royal Escape already used: checkmate.
    CHECK(setup(g, "6k1/5ppp/8/8/8/8/8/R5K1 w - - 0 1 mana=0/6 escape=1/0", true));
    CHECK(play(g, "a1a8"));
    CHECK(g.reason() == ResultReason::CHECKMATE);
}

TEST_CASE("arcane", stalemate_considers_spells)
{
    cl::Game& g = fresh_game();

    // No normal moves: with 2 mana Shadow Step is available -> not stalemate (cross-system test 13).
    CHECK(setup(g, "7k/7P/7K/8/1p6/1P6/8/8 b - - 0 1 mana=0/2", true));
    CHECK(! g.over());
    CHECK_EQ(count_legal(g), 2);
    CHECK(setup(g, "7k/7P/7K/8/1p6/1P6/8/8 b - - 0 1 mana=0/1", true));
    CHECK(g.reason() == ResultReason::STALEMATE);

    // The same position in Classic chess is stalemate.
    CHECK(setup(g, "7k/7P/7K/8/1p6/1P6/8/8 b - - 0 1", false));
    CHECK(g.reason() == ResultReason::STALEMATE);
}

TEST_CASE("arcane", magic_resolves_check)
{
    cl::Game& g = fresh_game();

    // Phase Shift block: knight d3 teleports to e2... only e-file squares block the rook.
    CHECK(setup(g, "4r1k1/8/8/8/8/3N4/8/3K4 w - - 0 1 mana=3/0", true));
    CHECK(! g.position().in_check());
    CHECK(setup(g, "4r1k1/8/8/8/8/3N4/8/4K3 w - - 0 1 mana=3/0", true));
    CHECK(g.position().in_check());
    CHECK(is_legal(g, "ps:d3e2"));
    CHECK(is_legal(g, "ps:d3e3"));
    CHECK(is_legal(g, "ps:d3e4"));
    CHECK(! is_legal(g, "ps:d3c3"));

    // Ghost Walk block through a friendly piece.
    CHECK(setup(g, "4r1k1/8/8/8/8/8/1P6/B3K3 w - - 0 1 mana=3/0", true));
    CHECK(g.position().in_check());
    CHECK(! is_legal(g, "gw:a1c3"));   // c3 does not block the e-file
    CHECK(setup(g, "4r1k1/8/8/8/8/8/3P4/2B1K3 w - - 0 1 mana=3/0", true));
    CHECK(is_legal(g, "gw:c1e3"));
}

// ---------------------------------------------------------------------------------------------------------------
// En passant, counters and position identity
// ---------------------------------------------------------------------------------------------------------------

TEST_CASE("arcane", en_passant_expires_after_magic)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4k3/8/8/8/3p4/8/4P3/4K1n1 w - - 0 1 mana=0/2", true));
    CHECK(play(g, "e2e4"));
    CHECK(is_legal(g, "d4e3"));
    CHECK(play(g, "ps:g1h1"));
    CHECK_EQ(g.position().ep, cl::NO_SQUARE);
    CHECK(play(g, "e1d1"));
    CHECK(! is_legal(g, "d4e3"));
}

TEST_CASE("arcane", halfmove_counter_rules)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4k3/8/8/8/3N4/8/3P4/4K3 w - - 40 1 mana=6/6", true));
    CHECK(play(g, "ps:d4e4"));
    CHECK_EQ(g.position().halfmove, 41);
    CHECK(play(g, "e8d8"));
    CHECK(play(g, "ss:d2c2"));
    CHECK_EQ(g.position().halfmove, 0);
}

TEST_CASE("arcane", position_identity_includes_mana_and_escape)
{
    cl::Position a;
    cl::Position b;
    cl::RuleSet rules = cl::RuleSet::arcane_rules();
    CHECK(a.set_fen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1 mana=3/2", rules) == cl::FenError::NONE);
    CHECK(b.set_fen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1 mana=3/3", rules) == cl::FenError::NONE);
    CHECK(a.hash != b.hash);
    cl::CompactPosition ca;
    cl::CompactPosition cb;
    cl::Game::compact(a, ca);
    cl::Game::compact(b, cb);
    CHECK(! (ca == cb));

    CHECK(b.set_fen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1 mana=3/2 escape=0/1", rules) == cl::FenError::NONE);
    CHECK(a.hash != b.hash);
    cl::Game::compact(b, cb);
    CHECK(! (ca == cb));

    CHECK(b.set_fen("4k3/8/8/8/8/8/8/R3K3 w - - 0 1 mana=3/2", rules) == cl::FenError::NONE);
    CHECK(a.hash == b.hash);
    cl::Game::compact(b, cb);
    CHECK(ca == cb);

    // Double-step eligibility is part of the identity too.
    CHECK(a.set_fen("4k3/8/8/8/8/8/3P4/4K3 w - - 0 1 mana=0/0", rules) == cl::FenError::NONE);
    CHECK(b.set_fen("4k3/8/8/8/8/8/3P4/4K3 w - - 0 1 mana=0/0 home=00/00", rules) == cl::FenError::NONE);
    CHECK(a.hash != b.hash);
}

TEST_CASE("arcane", repetition_requires_equal_mana)
{
    cl::Game& g = fresh_game();
    CHECK(setup(g, "1n2k3/8/8/8/8/8/8/1N2K3 w - - 0 1 mana=1/1", true));
    const char* cycle[4] = { "b1c3", "b8c6", "c3b1", "c6b8" };

    for(int i = 0; i < 4; ++i)
    {
        CHECK(play(g, cycle[i]));
    }

    CHECK_EQ(g.repetition_count(), 1);   // same board, different mana: a different position

    for(int r = 0; r < 3; ++r)
    {
        for(int i = 0; i < 4; ++i)
        {
            CHECK(play(g, cycle[i]));
        }
    }

    // Mana is capped at 6 for both now: the board repeats as a genuine repetition.
    CHECK_EQ(g.position().mana[cl::WHITE], 6);
    CHECK(g.repetition_count() >= 2);
}

TEST_CASE("arcane", arcane_repetition_window_survives_pawn_magic)
{
    // Magical pawn moves reset the halfmove counter but are reversible, so they must not erase repetition history.
    // Eight-ply cycle: White spends 2 + 2 mana on Shadow Steps and regenerates 4, Black stays capped at 6.
    cl::Game& g = fresh_game();
    CHECK(setup(g, "4k3/8/8/8/3P4/8/8/4K3 w - - 0 1 mana=6/6", true));
    const char* cycle[8] = { "ss:d4c4", "e8d8", "e1d1", "d8e8", "ss:c4d4", "e8d8", "d1e1", "d8e8" };

    for(int i = 0; i < 8; ++i)
    {
        CHECK(play(g, cycle[i]));
    }

    CHECK_EQ(g.position().mana[cl::WHITE], 6);
    CHECK_EQ(g.position().halfmove, 3);      // reset by the second Shadow Step...
    CHECK_EQ(g.repetition_count(), 2);       // ...yet the start position is still recognized as repeated

    for(int r = 0; r < 3; ++r)
    {
        for(int i = 0; i < 8; ++i)
        {
            CHECK(play(g, cycle[i]));
        }
    }

    CHECK(g.over());
    CHECK(g.reason() == ResultReason::FIVEFOLD_REPETITION);
}

TEST_CASE("arcane", one_action_per_turn)
{
    cl::Game& g = fresh_game();
    g.start(cl::RuleSet::arcane_rules());
    CHECK(play(g, "e2e4"));
    // After a normal move it is Black's turn: White cannot also cast.
    CHECK(g.position().side == cl::BLACK);
    cl::MoveList list;
    g.legal_actions(list);

    for(int i = 0; i < list.count; ++i)
    {
        CHECK(cl::color_of(g.position().board[list.moves[i].from()]) == cl::BLACK);
    }
}

TEST_CASE("arcane", arcane_perft_is_consistent)
{
    // Arcane perft from the start position with generous mana: every generated action must make / unmake cleanly
    // and keep the incremental state consistent.
    cl::Position pos;
    CHECK(pos.set_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 mana=6/6",
                      cl::RuleSet::arcane_rules()) == cl::FenError::NONE);
    uint64_t nodes = cl::perft(pos, 3);
    CHECK(nodes > 97862);   // strictly more actions than Classic perft(3) of the same position
    CHECK(pos.validate());

    cl::MoveList list;
    cl::generate_legal(pos, list);

    for(int i = 0; i < list.count; ++i)
    {
        cl::Undo undo;
        pos.make(list.moves[i], undo);
        CHECK(pos.validate());
        cl::MoveList replies;
        cl::generate_legal(pos, replies);

        for(int j = 0; j < replies.count; ++j)
        {
            cl::Undo undo2;
            pos.make(replies.moves[j], undo2);
            CHECK(pos.validate());
            pos.unmake(replies.moves[j], undo2);
        }

        pos.unmake(list.moves[i], undo);
        CHECK(pos.validate());
    }
}
