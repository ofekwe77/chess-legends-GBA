// Classic chess move generation verified against established perft reference counts.
#include "cl_test.h"

#include <string_view>

#include "cl_movegen.h"

namespace
{
    struct PerftCase
    {
        const char* fen;
        int depth;
        uint64_t nodes;
    };

    // Reference values: chessprogramming.org perft results and the widely used "perftsuite" edge cases.
    constexpr PerftCase cases[] = {
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 1, 20 },
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 2, 400 },
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 3, 8902 },
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 4, 197281 },
        { "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1", 5, 4865609 },
        // Kiwipete: castling, en passant, promotions, pins.
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 1, 48 },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 2, 2039 },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 3, 97862 },
        { "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1", 4, 4085603 },
        // Position 3: en passant discovered checks along the rank.
        { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 1, 14 },
        { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 3, 2812 },
        { "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1", 5, 674624 },
        // Position 4 and its mirror.
        { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 1, 6 },
        { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 3, 9467 },
        { "r3k2r/Pppp1ppp/1b3nbN/nP6/BBP1P3/q4N2/Pp1P2PP/R2Q1RK1 w kq - 0 1", 4, 422333 },
        { "r2q1rk1/pP1p2pp/Q4n2/bbp1p3/Np6/1B3NBn/pPPP1PPP/R3K2R b KQ - 0 1", 4, 422333 },
        // Position 5.
        { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 1, 44 },
        { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 3, 62379 },
        { "rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8", 4, 2103487 },
        // Position 6.
        { "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 1, 46 },
        { "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 3, 89890 },
        { "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10", 4, 3894594 },
        // Edge cases.
        { "3k4/3p4/8/K1P4r/8/8/8/8 b - - 0 1", 6, 1134888 },         // illegal en passant (pinned along rank)
        { "8/8/4k3/8/2p5/8/B2P2K1/8 w - - 0 1", 6, 1015133 },       // illegal en passant (diagonal pin)
        { "8/8/1k6/2b5/2pP4/8/5K2/8 b - d3 0 1", 6, 1440467 },      // en passant capture checks the opponent
        { "5k2/8/8/8/8/8/8/4K2R w K - 0 1", 6, 661072 },            // short castling gives check
        { "3k4/8/8/8/8/8/8/R3K3 w Q - 0 1", 6, 803711 },            // long castling gives check
        { "r3k2r/1b4bq/8/8/8/8/7B/R3K2R w KQkq - 0 1", 4, 1274206 }, // castling rights
        { "r3k2r/8/3Q4/8/8/5q2/8/R3K2R b KQkq - 0 1", 4, 1720476 }, // castling prevented
        { "2K2r2/4P3/8/8/8/8/8/3k4 w - - 0 1", 6, 3821001 },         // promote out of check
        { "8/8/1P2K3/8/2n5/1q6/8/5k2 b - - 0 1", 5, 1004658 },       // discovered check
        { "4k3/1P6/8/8/8/8/K7/8 w - - 0 1", 6, 217342 },            // promote to give check
        { "8/P1k5/K7/8/8/8/8/8 w - - 0 1", 6, 92683 },              // underpromote to check
        { "K1k5/8/P7/8/8/8/8/8 w - - 0 1", 6, 2217 },               // self stalemate
        { "8/k1P5/8/1K6/8/8/8/8 w - - 0 1", 7, 567584 },            // stalemate and checkmate
        { "8/8/2k5/5q2/5n2/8/5K2/8 b - - 0 1", 4, 23527 },          // stalemate and checkmate
    };
}

TEST_CASE("perft", perft_reference_positions)
{
    for(const PerftCase& test : cases)
    {
        cl::Position pos;
        CHECK(pos.set_fen(test.fen, cl::RuleSet::classic()) == cl::FenError::NONE);
        uint64_t nodes = cl::perft(pos, test.depth);

        if(nodes != test.nodes)
        {
            std::printf("    perft mismatch: %s depth %d -> %llu (expected %llu)\n", test.fen, test.depth,
                        (unsigned long long) nodes, (unsigned long long) test.nodes);
        }

        CHECK_EQ(nodes, test.nodes);
        CHECK(pos.validate());
    }
}

TEST_CASE("perft", make_unmake_restores_everything)
{
    // Walk a few plies of every reference position and verify that make / unmake leaves no trace.
    for(const PerftCase& test : cases)
    {
        cl::Position pos;
        CHECK(pos.set_fen(test.fen, cl::RuleSet::classic()) == cl::FenError::NONE);
        char before[128];
        pos.write_fen(before, sizeof(before));
        cl::MoveList list;
        cl::generate_legal(pos, list);

        for(int i = 0; i < list.count; ++i)
        {
            cl::Undo undo;
            pos.make(list.moves[i], undo);
            CHECK(pos.validate());
            pos.unmake(list.moves[i], undo);
            CHECK(pos.validate());
        }

        char after[128];
        pos.write_fen(after, sizeof(after));
        CHECK(std::string_view(before) == std::string_view(after));
    }
}
