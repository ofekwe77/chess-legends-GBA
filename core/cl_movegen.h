// CHESS LEGENDS: ARCANE WARS - move generation.
//
// One generator serves the human interface, the AI search, the puzzle validator and the tests: normal chess
// moves and magical abilities are alternative actions produced by the same code and validated by the same king
// safety test (make the action, verify the mover's king is not attacked, unmake).
//
// Generation functions append to their output.
#ifndef CL_MOVEGEN_H
#define CL_MOVEGEN_H

#include "cl_position.h"

namespace cl
{

// Pseudo-legal generation (king safety not yet verified).
void generate_normal(const Position& pos, MoveBuffer& out);
void generate_tactical(const Position& pos, MoveBuffer& out);   // captures, en passant and queen promotions
void generate_arcane(const Position& pos, MoveBuffer& out);     // affordable, enabled magical abilities
void generate_pseudo(const Position& pos, MoveBuffer& out);     // normal + arcane

// Magical actions whose moved piece(s) would attack the enemy king from the destination on an empty board: the
// candidates for direct magical checks (quiescence search). They still need verification by making them.
void generate_arcane_checks(const Position& pos, MoveBuffer& out);

namespace movegen_detail
{
    template<void (*Generator)(const Position&, MoveBuffer&)>
    void append(const Position& pos, MoveList& list)
    {
        MoveBuffer buffer(list.moves + list.count, MAX_MOVES - list.count);
        Generator(pos, buffer);
        list.count += buffer.count;
        list.overflow |= buffer.overflow;
    }
}

inline void generate_normal(const Position& pos, MoveList& list)
{
    movegen_detail::append<static_cast<void (*)(const Position&, MoveBuffer&)>(generate_normal)>(pos, list);
}

inline void generate_tactical(const Position& pos, MoveList& list)
{
    movegen_detail::append<static_cast<void (*)(const Position&, MoveBuffer&)>(generate_tactical)>(pos, list);
}

inline void generate_arcane(const Position& pos, MoveList& list)
{
    movegen_detail::append<static_cast<void (*)(const Position&, MoveBuffer&)>(generate_arcane)>(pos, list);
}

inline void generate_pseudo(const Position& pos, MoveList& list)
{
    movegen_detail::append<static_cast<void (*)(const Position&, MoveBuffer&)>(generate_pseudo)>(pos, list);
}

// Legal generation (clears the list first).
void generate_legal(Position& pos, MoveList& list);

[[nodiscard]] inline bool is_castling(Move move)
{
    return move.kind() == MK_CASTLE_KING || move.kind() == MK_CASTLE_QUEEN;
}

// Castling conditions not checked by the generator: the king is not in check and does not pass through an attacked
// square (the destination square is covered by the post-move king safety test).
[[nodiscard]] bool castling_path_safe(const Position& pos, Move move);

// True if the pseudo-legal action keeps the mover's king safe (includes the castling path conditions).
[[nodiscard]] bool leaves_king_safe(Position& pos, Move move);

// Full validation of an arbitrary action (generated or not): it must be one of the legal actions.
[[nodiscard]] bool is_legal(Position& pos, Move move);

// Early-exit test used for checkmate / stalemate detection (includes magical defenses).
[[nodiscard]] bool has_legal_action(Position& pos);

// True if the action puts the opponent in check (normal attack rules after the action).
[[nodiscard]] bool gives_check(Position& pos, Move move);

// Perft node count over all legal actions (normal + Arcane when the rules enable it).
[[nodiscard]] uint64_t perft(Position& pos, int depth);

}

#endif
