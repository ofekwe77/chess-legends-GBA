// CHESS LEGENDS: ARCANE WARS - action notation.
//
// Two formats:
//   * Coordinate notation for data files and tests: "e2e4", "e7e8q", and Arcane actions with an ability prefix:
//     "ss:d2c2" Shadow Step, "ps:f3e4" Phase Shift, "gw:c1e3" Ghost Walk, "cs:a1b1" Castle Swap (rook square,
//     partner square), "rc:e2f2@d1" Royal Command (ally from, ally to, queen square), "re:e1e3" Royal Escape.
//   * Display notation (SAN for chess moves, "Shadow Step d2-c2"-style text for magic) for the in-game log.
#ifndef CL_NOTATION_H
#define CL_NOTATION_H

#include "cl_position.h"

namespace cl
{

// Writes coordinate notation; returns the length.
int move_to_coordinate(Move move, char* output, int capacity);

// Finds the legal action matching coordinate notation; returns a null move if none matches.
[[nodiscard]] Move parse_coordinate(Position& pos, const char* text);

// SAN (with +/# suffix) for normal moves; compact ability text for magical actions. Requires the position
// before the move.
int move_to_display(Position& pos, Move move, char* output, int capacity);

[[nodiscard]] const char* ability_name(Ability ability);
[[nodiscard]] const char* ability_short_name(Ability ability);
[[nodiscard]] const char* piece_type_name(PieceType type);

// Fantasy class of a piece type: Soldier, Cavalry, Mystic, Guardian, Sorceress, Sovereign.
[[nodiscard]] const char* piece_class_name(PieceType type);

int square_name(int sq, char* output);

}

#endif
