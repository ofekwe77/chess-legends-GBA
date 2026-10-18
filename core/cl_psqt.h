// CHESS LEGENDS: ARCANE WARS - material and piece-square tables (tapered middlegame / endgame).
//
// Values are written in board order as seen from White (first row = rank 8). The Position keeps the sum of
// material + table values incrementally, so the evaluation never rescans the board for these terms.
#ifndef CL_PSQT_H
#define CL_PSQT_H

#include <cstdint>

#include "cl_types.h"

namespace cl
{

namespace psqt_detail
{
    // clang-format off
    constexpr int16_t pawn_mg[64] = {
          0,   0,   0,   0,   0,   0,   0,   0,
         45,  50,  50,  55,  55,  50,  50,  45,
         14,  18,  24,  32,  32,  24,  18,  14,
          6,   8,  12,  24,  24,  12,   8,   6,
          2,   4,   8,  20,  20,   6,   4,   2,
          4,   2,   0,   6,   6,  -4,   2,   4,
          4,   8,   8, -16, -16,   8,   8,   4,
          0,   0,   0,   0,   0,   0,   0,   0 };

    constexpr int16_t pawn_eg[64] = {
          0,   0,   0,   0,   0,   0,   0,   0,
         80,  80,  75,  70,  70,  75,  80,  80,
         50,  50,  45,  40,  40,  45,  50,  50,
         30,  28,  24,  20,  20,  24,  28,  30,
         16,  14,  12,  10,  10,  12,  14,  16,
          6,   6,   4,   4,   4,   4,   6,   6,
          2,   2,   2,   0,   0,   2,   2,   2,
          0,   0,   0,   0,   0,   0,   0,   0 };

    constexpr int16_t knight_mg[64] = {
        -50, -40, -30, -30, -30, -30, -40, -50,
        -40, -20,   0,   5,   5,   0, -20, -40,
        -30,   5,  10,  15,  15,  10,   5, -30,
        -30,   0,  15,  20,  20,  15,   0, -30,
        -30,   5,  15,  20,  20,  15,   5, -30,
        -30,   0,  12,  15,  15,  12,   0, -30,
        -40, -20,   0,   5,   5,   0, -20, -40,
        -50, -35, -30, -30, -30, -30, -35, -50 };

    constexpr int16_t knight_eg[64] = {
        -50, -40, -30, -30, -30, -30, -40, -50,
        -40, -20,   0,   0,   0,   0, -20, -40,
        -30,   0,  10,  15,  15,  10,   0, -30,
        -30,   5,  15,  20,  20,  15,   5, -30,
        -30,   0,  15,  20,  20,  15,   0, -30,
        -30,   5,  10,  15,  15,  10,   5, -30,
        -40, -20,   0,   5,   5,   0, -20, -40,
        -50, -40, -30, -30, -30, -30, -40, -50 };

    constexpr int16_t bishop_mg[64] = {
        -20, -10, -10, -10, -10, -10, -10, -20,
        -10,   0,   0,   0,   0,   0,   0, -10,
        -10,   0,   5,  10,  10,   5,   0, -10,
        -10,   5,   5,  10,  10,   5,   5, -10,
        -10,   0,  10,  10,  10,  10,   0, -10,
        -10,  10,  10,  10,  10,  10,  10, -10,
        -10,  12,   0,   0,   0,   0,  12, -10,
        -20, -10, -12, -10, -10, -12, -10, -20 };

    constexpr int16_t bishop_eg[64] = {
        -20, -10, -10, -10, -10, -10, -10, -20,
        -10,   0,   0,   0,   0,   0,   0, -10,
        -10,   0,   5,  10,  10,   5,   0, -10,
        -10,   5,  10,  12,  12,  10,   5, -10,
        -10,   5,  10,  12,  12,  10,   5, -10,
        -10,   0,  10,  10,  10,  10,   0, -10,
        -10,   0,   0,   0,   0,   0,   0, -10,
        -20, -10, -10, -10, -10, -10, -10, -20 };

    constexpr int16_t rook_mg[64] = {
          0,   0,   0,   0,   0,   0,   0,   0,
         10,  15,  15,  15,  15,  15,  15,  10,
         -5,   0,   0,   0,   0,   0,   0,  -5,
         -5,   0,   0,   0,   0,   0,   0,  -5,
         -5,   0,   0,   0,   0,   0,   0,  -5,
         -5,   0,   0,   0,   0,   0,   0,  -5,
         -5,   0,   0,   0,   0,   0,   0,  -5,
         -2,   0,   4,   8,   8,   4,   0,  -2 };

    constexpr int16_t rook_eg[64] = {
          6,   6,   6,   6,   6,   6,   6,   6,
         12,  12,  12,  12,  12,  12,  12,  12,
          2,   2,   2,   2,   2,   2,   2,   2,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   0,   0,   0,   0,   0,
          0,   0,   0,   2,   2,   0,   0,   0 };

    constexpr int16_t queen_mg[64] = {
        -20, -10, -10,  -5,  -5, -10, -10, -20,
        -10,   0,   0,   0,   0,   0,   0, -10,
        -10,   0,   5,   5,   5,   5,   0, -10,
         -5,   0,   5,   5,   5,   5,   0,  -5,
         -5,   0,   5,   5,   5,   5,   0,  -5,
        -10,   2,   5,   5,   5,   5,   0, -10,
        -10,   0,   2,   0,   0,   0,   0, -10,
        -20, -10, -10,  -2,  -5, -10, -10, -20 };

    constexpr int16_t queen_eg[64] = {
        -20, -10, -10,  -5,  -5, -10, -10, -20,
        -10,   0,   5,   5,   5,   5,   0, -10,
        -10,   5,  10,  10,  10,  10,   5, -10,
         -5,   5,  10,  15,  15,  10,   5,  -5,
         -5,   5,  10,  15,  15,  10,   5,  -5,
        -10,   5,  10,  10,  10,  10,   5, -10,
        -10,   0,   5,   5,   5,   5,   0, -10,
        -20, -10, -10,  -5,  -5, -10, -10, -20 };

    constexpr int16_t king_mg[64] = {
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -30, -40, -40, -50, -50, -40, -40, -30,
        -20, -30, -30, -40, -40, -30, -30, -20,
        -10, -20, -20, -20, -20, -20, -20, -10,
         15,  15,   0, -10, -10,   0,  18,  15,
         20,  30,  10,   0,   0,  10,  30,  20 };

    constexpr int16_t king_eg[64] = {
        -50, -40, -30, -20, -20, -30, -40, -50,
        -30, -20, -10,   0,   0, -10, -20, -30,
        -30, -10,  20,  30,  30,  20, -10, -30,
        -30, -10,  30,  40,  40,  30, -10, -30,
        -30, -10,  30,  40,  40,  30, -10, -30,
        -30, -10,  20,  30,  30,  20, -10, -30,
        -30, -30,   0,   0,   0,   0, -30, -30,
        -50, -30, -30, -30, -30, -30, -30, -50 };
    // clang-format on

    struct Tables
    {
        // [piece code][sq64] material + table value from the piece owner's point of view.
        int16_t mg[16][64] = {};
        int16_t eg[16][64] = {};
    };

    constexpr Tables build()
    {
        const int16_t* mg_src[7] = { nullptr, pawn_mg, knight_mg, bishop_mg, rook_mg, queen_mg, king_mg };
        const int16_t* eg_src[7] = { nullptr, pawn_eg, knight_eg, bishop_eg, rook_eg, queen_eg, king_eg };
        constexpr int16_t mg_material[7] = { 0, 100, 320, 330, 500, 900, 0 };
        constexpr int16_t eg_material[7] = { 0, 110, 310, 330, 520, 920, 0 };
        Tables tables;

        for(int t = 1; t < 7; ++t)
        {
            for(int index = 0; index < 64; ++index)
            {
                int file = index & 7;
                int rank = index >> 3;
                int white_visual = (7 - rank) * 8 + file;   // board-order table row for White
                int black_visual = rank * 8 + file;         // mirrored for Black
                tables.mg[t][index] = int16_t(mg_material[t] + mg_src[t][white_visual]);
                tables.eg[t][index] = int16_t(eg_material[t] + eg_src[t][white_visual]);
                tables.mg[t | 8][index] = int16_t(mg_material[t] + mg_src[t][black_visual]);
                tables.eg[t | 8][index] = int16_t(eg_material[t] + eg_src[t][black_visual]);
            }
        }

        return tables;
    }
}

inline constexpr psqt_detail::Tables psqt = psqt_detail::build();

// Game phase weights (24 = full opening material, 0 = pawn endgame).
constexpr int8_t phase_weight[piece_types_count] = { 0, 0, 1, 1, 2, 4, 0 };
constexpr int max_phase = 24;

}

#endif
