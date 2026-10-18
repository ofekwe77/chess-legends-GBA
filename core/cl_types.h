// CHESS LEGENDS: ARCANE WARS - core chess types.
//
// Platform independent: this header (and everything in core/) never includes Butano or any GBA header, so the
// same rules engine runs inside the ROM and inside the host-side test / simulation programs.
//
// Board representation: 0x88 mailbox. Square index = rank * 16 + file, rank 0 is White's back rank, file 0 is
// the a-file. A square is on the board when (square & 0x88) == 0.
#ifndef CL_TYPES_H
#define CL_TYPES_H

#include <cstdint>

#include "cl_assert.h"
#include "cl_platform.h"

namespace cl
{

enum Color : uint8_t
{
    WHITE = 0,
    BLACK = 1
};

[[nodiscard]] constexpr Color opposite(Color c)
{
    return Color(c ^ 1);
}

enum PieceType : uint8_t
{
    NO_TYPE = 0,
    PAWN = 1,
    KNIGHT = 2,
    BISHOP = 3,
    ROOK = 4,
    QUEEN = 5,
    KING = 6
};

constexpr int piece_types_count = 7;

// Piece code: type in bits 0-2, color in bit 3. Zero means an empty square.
using Piece = uint8_t;

constexpr Piece EMPTY = 0;
constexpr Piece W_PAWN = PAWN;
constexpr Piece W_KNIGHT = KNIGHT;
constexpr Piece W_BISHOP = BISHOP;
constexpr Piece W_ROOK = ROOK;
constexpr Piece W_QUEEN = QUEEN;
constexpr Piece W_KING = KING;
constexpr Piece B_PAWN = PAWN | 8;
constexpr Piece B_KNIGHT = KNIGHT | 8;
constexpr Piece B_BISHOP = BISHOP | 8;
constexpr Piece B_ROOK = ROOK | 8;
constexpr Piece B_QUEEN = QUEEN | 8;
constexpr Piece B_KING = KING | 8;

[[nodiscard]] constexpr Piece make_piece(Color c, PieceType t)
{
    return Piece(t | (c << 3));
}

[[nodiscard]] constexpr PieceType type_of(Piece p)
{
    return PieceType(p & 7);
}

[[nodiscard]] constexpr Color color_of(Piece p)
{
    return Color(p >> 3);
}

// ---------------------------------------------------------------------------------------------------------------
// Squares
// ---------------------------------------------------------------------------------------------------------------

constexpr int NO_SQUARE = 0x7F;   // never a valid 0x88 square (0x7F & 0x88 != 0)

[[nodiscard]] constexpr int make_square(int file, int rank)
{
    return rank * 16 + file;
}

[[nodiscard]] constexpr int file_of(int sq)
{
    return sq & 7;
}

[[nodiscard]] constexpr int rank_of(int sq)
{
    return sq >> 4;
}

[[nodiscard]] constexpr bool on_board(int sq)
{
    return (sq & 0x88) == 0;
}

// 0..63 index (a1 = 0, h8 = 63) for compact tables.
[[nodiscard]] constexpr int sq64(int sq)
{
    return (sq >> 4) * 8 + (sq & 7);
}

[[nodiscard]] constexpr int sq88(int index64)
{
    return (index64 >> 3) * 16 + (index64 & 7);
}

// Rank seen from a side: 0 = that side's back rank, 7 = its promotion rank.
[[nodiscard]] constexpr int relative_rank(Color c, int sq)
{
    return c == WHITE ? rank_of(sq) : 7 - rank_of(sq);
}

// Chebyshev (king-move) distance.
[[nodiscard]] constexpr int square_distance(int a, int b)
{
    int df = file_of(a) - file_of(b);
    int dr = rank_of(a) - rank_of(b);
    df = df < 0 ? -df : df;
    dr = dr < 0 ? -dr : dr;
    return df > dr ? df : dr;
}

[[nodiscard]] constexpr bool is_light_square(int sq)
{
    return ((file_of(sq) + rank_of(sq)) & 1) != 0;
}

constexpr int A1 = 0x00, B1 = 0x01, C1 = 0x02, D1 = 0x03, E1 = 0x04, F1 = 0x05, G1 = 0x06, H1 = 0x07;
constexpr int A8 = 0x70, B8 = 0x71, C8 = 0x72, D8 = 0x73, E8 = 0x74, F8 = 0x75, G8 = 0x76, H8 = 0x77;

// ---------------------------------------------------------------------------------------------------------------
// Castling rights
// ---------------------------------------------------------------------------------------------------------------

constexpr uint8_t CASTLE_WK = 1;
constexpr uint8_t CASTLE_WQ = 2;
constexpr uint8_t CASTLE_BK = 4;
constexpr uint8_t CASTLE_BQ = 8;
constexpr uint8_t CASTLE_ALL = 15;

// ---------------------------------------------------------------------------------------------------------------
// Direction offsets (0x88)
// ---------------------------------------------------------------------------------------------------------------

constexpr int8_t knight_offsets[8] = { 33, 31, 18, 14, -14, -18, -31, -33 };
constexpr int8_t king_offsets[8] = { 1, -1, 16, -16, 17, 15, -15, -17 };
constexpr int8_t bishop_offsets[4] = { 17, 15, -15, -17 };
constexpr int8_t rook_offsets[4] = { 1, -1, 16, -16 };

// Material values from the design document (centipawns), used for move ordering and the evaluation.
constexpr int16_t piece_value[piece_types_count] = { 0, 100, 320, 330, 500, 900, 20000 };

}

#endif
