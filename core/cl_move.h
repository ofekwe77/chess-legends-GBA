// CHESS LEGENDS: ARCANE WARS - actions (normal chess moves and magical abilities share one representation).
#ifndef CL_MOVE_H
#define CL_MOVE_H

#include "cl_types.h"
#include "cl_config.h"

namespace cl
{

enum MoveKind : uint8_t
{
    MK_QUIET = 0,           // normal non-capturing move
    MK_CAPTURE = 1,         // normal capture
    MK_DOUBLE_PUSH = 2,     // pawn two-square advance (may create en passant rights)
    MK_EN_PASSANT = 3,      // en passant capture
    MK_CASTLE_KING = 4,     // O-O
    MK_CASTLE_QUEEN = 5,    // O-O-O
    MK_PROMOTION = 6,       // promotion (capture flag tells whether it also captures)
    MK_SHADOW_STEP = 7,     // pawn sideways one square                         (from = pawn, to = destination)
    MK_PHASE_SHIFT = 8,     // knight to an adjacent square                      (from = knight, to = destination)
    MK_GHOST_WALK = 9,      // bishop through one friendly piece                 (from = bishop, to = destination)
    MK_CASTLE_SWAP = 10,    // rook swaps with a friendly bishop / knight        (from = rook, to = partner)
    MK_ROYAL_COMMAND = 11,  // queen moves an adjacent ally                       (from = ally, to = dest, aux = queen)
    MK_ROYAL_ESCAPE = 12,   // king leaps two squares while in check             (from = king, to = destination)
    MOVE_KINDS_COUNT = 13
};

// 32-bit packed action:
//   bits  0- 6 from square (0x88)
//   bits  7-13 to square (0x88)
//   bits 14-20 auxiliary square (Royal Command: commanding queen), 0 otherwise
//   bits 21-24 kind
//   bit  25    capture flag
//   bits 26-28 promotion piece type
class Move
{
public:
    constexpr Move() = default;

    constexpr explicit Move(uint32_t data) :
        _data(data)
    {
    }

    [[nodiscard]] static constexpr Move make(int from, int to, MoveKind kind, bool capture = false,
                                             PieceType promotion = NO_TYPE, int aux = 0)
    {
        return Move(uint32_t(from) | (uint32_t(to) << 7) | (uint32_t(aux) << 14) | (uint32_t(kind) << 21) |
                    (uint32_t(capture) << 25) | (uint32_t(promotion) << 26));
    }

    [[nodiscard]] constexpr int from() const
    {
        return int(_data & 0x7F);
    }

    [[nodiscard]] constexpr int to() const
    {
        return int((_data >> 7) & 0x7F);
    }

    [[nodiscard]] constexpr int aux() const
    {
        return int((_data >> 14) & 0x7F);
    }

    [[nodiscard]] constexpr MoveKind kind() const
    {
        return MoveKind((_data >> 21) & 0xF);
    }

    [[nodiscard]] constexpr bool is_capture() const
    {
        return (_data >> 25) & 1;
    }

    [[nodiscard]] constexpr PieceType promotion() const
    {
        return PieceType((_data >> 26) & 7);
    }

    [[nodiscard]] constexpr bool is_magic() const
    {
        return kind() >= MK_SHADOW_STEP;
    }

    [[nodiscard]] constexpr Ability ability() const
    {
        return Ability(kind() - MK_SHADOW_STEP);
    }

    [[nodiscard]] constexpr bool is_null() const
    {
        return _data == 0;
    }

    [[nodiscard]] constexpr uint32_t data() const
    {
        return _data;
    }

    [[nodiscard]] constexpr bool operator==(const Move& other) const = default;

private:
    uint32_t _data = 0;
};

[[nodiscard]] constexpr MoveKind ability_move_kind(Ability ability)
{
    return MoveKind(MK_SHADOW_STEP + int(ability));
}

// Capacity of one generated action list. Normal chess never exceeds 218 moves; Arcane actions add at most a
// few dozen more in realistic positions. Generation stops safely (and reports overflow) instead of writing past
// the end in pathological positions.
constexpr int MAX_MOVES = 320;

// Non-owning output buffer (the search generates straight into its shared move stack).
struct MoveBuffer
{
    Move* moves;
    int capacity;
    int count = 0;
    bool overflow = false;

    MoveBuffer(Move* storage, int storage_capacity) :
        moves(storage),
        capacity(storage_capacity)
    {
    }

    void add(Move m)
    {
        if(count < capacity) [[likely]]
        {
            moves[count++] = m;
        }
        else
        {
            overflow = true;
        }
    }
};

struct MoveList
{
    Move moves[MAX_MOVES];
    int count = 0;
    bool overflow = false;

    void clear()
    {
        count = 0;
        overflow = false;
    }

    void add(Move m)
    {
        if(count < MAX_MOVES) [[likely]]
        {
            moves[count++] = m;
        }
        else
        {
            overflow = true;
        }
    }

    [[nodiscard]] bool contains(Move m) const
    {
        for(int i = 0; i < count; ++i)
        {
            if(moves[i] == m)
            {
                return true;
            }
        }

        return false;
    }
};

}

#endif
