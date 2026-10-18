// CHESS LEGENDS: ARCANE WARS - Zobrist hashing keys.
//
// Keys are generated at compile time from a fixed seed (splitmix64), so the ROM and the host programs produce
// identical hashes. An Arcane Chess position hash covers piece placement, side to move, castling rights, en
// passant rights, both mana values, both Royal Escape counters and the pawn double-step eligibility masks
// (design document section 28).
#ifndef CL_ZOBRIST_H
#define CL_ZOBRIST_H

#include <cstdint>

namespace cl
{

struct ZobristKeys
{
    uint64_t piece[16][64] = {};
    uint64_t side = 0;
    uint64_t castling[16] = {};
    uint64_t ep_file[8] = {};
    uint64_t mana[2][16] = {};
    uint64_t escapes[2][4] = {};
    uint64_t home[2][8] = {};
};

namespace zobrist_detail
{
    constexpr uint64_t splitmix64(uint64_t& state)
    {
        state += 0x9E3779B97F4A7C15ull;
        uint64_t z = state;
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
        return z ^ (z >> 31);
    }

    constexpr ZobristKeys generate(uint64_t seed)
    {
        ZobristKeys keys;
        uint64_t state = seed;

        for(int p = 0; p < 16; ++p)
        {
            for(int s = 0; s < 64; ++s)
            {
                keys.piece[p][s] = splitmix64(state);
            }
        }

        keys.side = splitmix64(state);

        // Castling keys are XOR combinations of four independent bits so that rights can be toggled one by one.
        uint64_t castle_bits[4] = { splitmix64(state), splitmix64(state), splitmix64(state), splitmix64(state) };

        for(int c = 0; c < 16; ++c)
        {
            uint64_t key = 0;

            for(int b = 0; b < 4; ++b)
            {
                if(c & (1 << b))
                {
                    key ^= castle_bits[b];
                }
            }

            keys.castling[c] = key;
        }

        for(int f = 0; f < 8; ++f)
        {
            keys.ep_file[f] = splitmix64(state);
        }

        for(int c = 0; c < 2; ++c)
        {
            for(int m = 0; m < 16; ++m)
            {
                keys.mana[c][m] = splitmix64(state);
            }

            for(int e = 0; e < 4; ++e)
            {
                keys.escapes[c][e] = splitmix64(state);
            }

            for(int f = 0; f < 8; ++f)
            {
                keys.home[c][f] = splitmix64(state);
            }
        }

        return keys;
    }
}

inline constexpr ZobristKeys zobrist = zobrist_detail::generate(0xC4E55A2CA1E5ull);

}

#endif
