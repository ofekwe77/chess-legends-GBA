// CHESS LEGENDS: ARCANE WARS - combination puzzle finder (host tool).
//
// Searches random sparse positions for Arcane mates in N that need at least two different abilities: a forced mate
// in N exists, there is no faster mate, there is no mate in N without abilities, and masking either of two
// different abilities breaks the mate. Every reported position is a proof produced by the exact solver.
//
// Usage: generate [moves=2] [attempts=200000] [seed=1] [need=-1] [wanted=auto]
//   need: ability index that must be among the required ones (-1: any); wanted: minimum required abilities.
#include <cstdio>
#include <cstdlib>

#include "cl_game.h"
#include "cl_movegen.h"
#include "cl_notation.h"
#include "cl_solver.h"

namespace cl
{
    [[noreturn]] void assert_failed(const char* condition, const char* file, int line)
    {
        std::printf("ASSERTION FAILED %s:%d: %s\n", file, line, condition);
        std::abort();
    }
}

namespace
{
    uint32_t state = 1;

    uint32_t next()
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    int pick(int n)
    {
        return int((next() >> 8) % uint32_t(n));
    }

    bool mate_with(const cl::Position& base, uint8_t mask, int moves)
    {
        cl::Position pos = base;
        pos.rules.ability_mask[pos.side] = mask;
        cl::SolverStats stats;
        stats.node_limit = 3000000;
        return cl::forced_mate(pos, moves, stats, nullptr) && ! stats.aborted;
    }
}

int main(int argc, char** argv)
{
    const int moves = argc > 1 ? std::atoi(argv[1]) : 2;
    const int attempts = argc > 2 ? std::atoi(argv[2]) : 200000;
    state = argc > 3 ? uint32_t(std::atoi(argv[3])) : 1u;
    const int need = argc > 4 ? std::atoi(argv[4]) : -1;
    const int wanted = argc > 5 ? std::atoi(argv[5]) : (moves >= 3 ? 1 : 2);
    int found = 0;

    // White army pool and black defenders pool.
    const char white_pool[] = { 'Q', 'R', 'R', 'B', 'B', 'N', 'N', 'P', 'P', 'P' };
    const char black_pool[] = { 'p', 'p', 'p', 'r', 'b', 'n' };

    for(int attempt = 0; attempt < attempts; ++attempt)
    {
        char board[64];

        for(char& c : board)
        {
            c = 0;
        }

        // Black king near an edge (mating nets), white king somewhere safe.
        const int black_king = pick(2) ? 56 + pick(8) : 48 + pick(8) + (pick(2) ? 8 : 0);
        board[black_king < 64 ? black_king : 63] = 'k';
        int white_king;

        do
        {
            white_king = pick(64);
        }
        while(board[white_king] || cl::square_distance(cl::sq88(white_king), cl::sq88(black_king)) < 2);

        board[white_king] = 'K';
        const int white_count = 2 + pick(3);
        const int black_count = 1 + pick(3);

        for(int i = 0; i < white_count + black_count; ++i)
        {
            const char piece = i < white_count ? white_pool[pick(int(sizeof(white_pool)))] :
                                                 black_pool[pick(int(sizeof(black_pool)))];
            int sq;
            int guard = 0;

            do
            {
                sq = pick(64);
            }
            while((board[sq] || ((piece == 'P' || piece == 'p') && (sq < 8 || sq >= 56))) && ++guard < 100);

            if(! board[sq])
            {
                board[sq] = piece;
            }
        }

        // Realistic material only: at most one queen, two rooks, two knights, two bishops on opposite colors.
        int queens = 0, rooks = 0, knights = 0, light_bishops = 0, dark_bishops = 0;

        for(int sq = 0; sq < 64; ++sq)
        {
            switch(board[sq])
            {
            case 'Q': ++queens; break;
            case 'R': ++rooks; break;
            case 'N': ++knights; break;
            case 'B': ((sq / 8 + sq % 8) % 2 ? light_bishops : dark_bishops) += 1; break;
            default: break;
            }
        }

        if(queens > 1 || rooks > 2 || knights > 2 || light_bishops > 1 || dark_bishops > 1)
        {
            continue;
        }

        char fen[128];
        int length = 0;

        for(int rank = 7; rank >= 0; --rank)
        {
            int empty = 0;

            for(int file = 0; file < 8; ++file)
            {
                const char c = board[rank * 8 + file];

                if(! c)
                {
                    ++empty;
                    continue;
                }

                if(empty)
                {
                    fen[length++] = char('0' + empty);
                    empty = 0;
                }

                fen[length++] = c;
            }

            if(empty)
            {
                fen[length++] = char('0' + empty);
            }

            if(rank)
            {
                fen[length++] = '/';
            }
        }

        const char* tail = " w - - 0 1 mana=6/0";

        for(const char* t = tail; *t; ++t)
        {
            fen[length++] = *t;
        }

        fen[length] = 0;
        cl::Position pos;

        if(pos.set_fen(fen, cl::RuleSet::arcane_rules()) != cl::FenError::NONE || pos.in_check())
        {
            continue;
        }

        cl::Game game;

        if(game.setup(fen, cl::RuleSet::arcane_rules(), false) != cl::FenError::NONE || game.over())
        {
            continue;
        }

        if(! mate_with(pos, cl::ABILITY_MASK_ALL, moves))
        {
            continue;
        }

        if(moves > 1 && mate_with(pos, cl::ABILITY_MASK_ALL, moves - 1))
        {
            continue;
        }

        if(mate_with(pos, 0, moves))
        {
            continue;
        }

        int required = 0;
        int required_mask = 0;

        for(int a = 0; a < cl::ABILITIES_COUNT; ++a)
        {
            if(! mate_with(pos, uint8_t(cl::ABILITY_MASK_ALL & ~(1 << a)), moves))
            {
                ++required;
                required_mask |= 1 << a;
            }
        }

        if(required >= wanted && (need < 0 || (required_mask & (1 << need))))
        {
            ++found;
            std::printf("FOUND %s  requires:", fen);

            for(int a = 0; a < cl::ABILITIES_COUNT; ++a)
            {
                if(required_mask & (1 << a))
                {
                    std::printf(" %s", cl::ability_name(cl::Ability(a)));
                }
            }

            std::printf("\n");
            std::fflush(stdout);

            if(found >= 12)
            {
                break;
            }
        }
    }

    std::printf("done: %d found\n", found);
    return 0;
}
