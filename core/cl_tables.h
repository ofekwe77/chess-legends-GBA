// CHESS LEGENDS: ARCANE WARS - shared 0x88 lookup tables (defined in cl_position.bn_iwram.cpp).
#ifndef CL_TABLES_H
#define CL_TABLES_H

#include <cstdint>

#include "cl_platform.h"
#include "cl_types.h"

namespace cl
{

constexpr uint8_t ATK_WPAWN = 1;
constexpr uint8_t ATK_BPAWN = 2;
constexpr uint8_t ATK_KNIGHT = 4;
constexpr uint8_t ATK_BISHOP = 8;
constexpr uint8_t ATK_ROOK = 16;
constexpr uint8_t ATK_QUEEN = 32;
constexpr uint8_t ATK_KING = 64;

// Indexed by (target - from + 119): which piece kinds can attack along that vector, and the unit step.
struct AttackTables
{
    uint8_t mask[240];
    int8_t step[240];
    uint8_t attack_bit[16];     // piece code -> ATK_* bit
    uint8_t castle_mask[128];   // castling rights kept when a piece leaves / arrives on a square
};

extern AttackTables tables;

// True when a piece standing on `from` would attack `target` on an empty board (blockers ignored).
[[nodiscard]] inline bool attacks_ignoring_blockers(Piece piece, int from, int target)
{
    return (tables.mask[target - from + 119] & tables.attack_bit[piece]) != 0;
}

}

#endif
