# Rules of CHESS LEGENDS: ARCANE WARS

This document specifies the game precisely enough to build an independent implementation that plays exactly the
same game. Where it states a number, the number comes from `core/cl_config.h`; where it states a behavior, the
behavior is the one implemented in `core/` and checked by the host tests (`tests/`). Section numbers in brackets
refer to the design document `MD.txt`.

Two rule sets exist:

* **Classic Chess**: standard chess (the rules of the FIDE Laws of Chess for movement, check, checkmate and draws,
  as detailed in section 1).
* **Arcane Chess**: Classic Chess plus a mana resource and six magical abilities (sections 2 and 3).

The campaign, the challenges, the tutorial and the boss gauntlet play Arcane Chess with announced modifiers
(section 4).

---

## 1. Classic Chess

### 1.1 Board, pieces and movement

The standard 8x8 board and starting position. White moves first. Pieces move and capture as in standard chess:

* King: one square in any direction.
* Queen: any distance along ranks, files and diagonals.
* Rook: any distance along ranks and files.
* Bishop: any distance along diagonals.
* Knight: the L-shaped jump (two squares in one direction, one square perpendicular), jumping over pieces.
* Pawn: one square forward to an empty square; two squares forward from its starting rank when both squares are
  empty; captures one square diagonally forward.

A move is legal only if the moving side's king is not attacked after the move.

### 1.2 Special moves

* **Castling** (king side and queen side): the king moves two squares toward a rook and that rook jumps to the
  square the king crossed. Allowed only if neither the king nor that rook has moved (castling right still held),
  every square between them is empty, the king is not in check, and neither the square the king crosses nor its
  destination is attacked.
* **En passant**: a pawn that has just made a two-square advance can be captured by an enemy pawn standing beside
  its destination, as if it had advanced one square. The right exists only on the very next action.
* **Promotion**: a pawn reaching the last rank by a normal move becomes a queen, rook, bishop or knight of the
  player's choice (underpromotion included).

### 1.3 End of the game

| Result | Condition |
|---|---|
| Checkmate (win) | The side to move is in check and has no legal action. |
| Stalemate (draw) | The side to move is not in check and has no legal action. |
| Resignation | A player resigns (in a game against the computer, the person; in local two-player mode, the player to move). |
| Draw by agreement | Local two-player mode only: the player to move offers, the other accepts. |
| Threefold repetition (claim) | The player to move may claim a draw when the current position has occurred three times. |
| Fifty-move rule (claim) | The player to move may claim a draw after 100 plies without a capture or a pawn move. |
| Fivefold repetition (automatic) | The game is drawn when the current position occurs for the fifth time. |
| Seventy-five-move rule (automatic) | The game is drawn after 150 plies without a capture or a pawn move (unless the last move mated). |
| Dead position (automatic) | See 1.4. |

Positions are compared for repetition with the side to move, the castling rights and the en passant right (the
latter only when an en passant capture is actually legal).

### 1.4 Dead positions

A position is declared dead, and drawn at once, only for these material sets (kings excluded):

* nothing at all (king against king);
* a single knight or a single bishop on the whole board;
* only bishops, of any number on either side, all standing on squares of one color.

The same list applies to Arcane Chess. It is safe there because magic never captures and never adds attacking
power; positions that are merely hard to win are never declared dead [29].

---

## 2. Arcane Chess: actions and mana

### 2.1 One action per turn [14]

On each turn the player to move performs exactly one action: either a legal chess move, or a legal magical
ability. A magical action uses the whole turn; moving and casting in the same turn is impossible.

The rules engine applies an action in one authoritative transition, in this order:

1. Verify that the action is legal in the current position (section 2.3).
2. Move the pieces; update castling rights, the en passant right and the movement history of pawns.
3. Update the halfmove counter (section 3.4) and spend the ability's mana.
4. Pass the turn to the opponent, then apply the opponent's turn-start mana regeneration.
5. Determine check, checkmate, stalemate and the automatic draws; append the position to the repetition history.

### 2.2 Mana (ARCANE ENERGY) [15]

* Each player has their own mana, from 0 to 6.
* At the beginning of each of a player's turns, that player gains 1 mana (never above 6; any excess is lost).
* At the initial position White has already received the first turn-start point: White starts with 1, Black with 0.
  Black receives its first point when its first turn begins.
* Mana is gained in no other way: not for captures, not for checks, never at random. Unused mana carries over.
* An ability costs its mana when it is executed. An illegal action costs nothing.

| Ability | Piece | Cost | Limit |
|---|---|---|---|
| Shadow Step | Pawn | 2 | |
| Phase Shift | Knight | 3 | |
| Ghost Walk | Bishop | 3 | |
| Castle Swap | Rook | 4 | |
| Royal Command | Queen | 5 | |
| Royal Escape | King | 5 | once per player per game, only while in check |

### 2.3 Legality of an ability [23]

An ability is legal only if all of these hold:

1. The acting player owns the piece that uses it (for Royal Command, the queen and the commanded piece).
2. The player has at least the ability's cost in mana (after this turn's regeneration).
3. The ability is enabled for that player (always in normal Arcane matches; see 4.2 for scenarios).
4. All of the ability's own conditions (section 3) are met.
5. After the action, the acting player's king is not attacked. This applies also when the player was in check
   before the action: an ability may resolve a check by moving the king, by blocking the attack or by moving a
   piece out of an attacking line, exactly like a normal move.

No ability captures. A magical action can still give check, because after every action check is determined
from the normal attack patterns of the pieces in their new positions [22].

---

## 3. The six abilities

Directions use the board as White sees it; "adjacent" means one of the eight squares around a square (king-move
adjacency).

### 3.1 Shadow Step (pawn, 2 mana) [16]

* The pawn moves exactly one square left or right along its rank.
* The destination must be on the board and empty. Shadow Step never captures.
* The pawn keeps its color and its forward direction.
* The pawn counts as moved: it loses the right to a two-square advance, even if it stands on its starting rank
  afterwards (see the movement history in 3.4).
* Shadow Step never promotes (a pawn stays on its rank).

### 3.2 Phase Shift (knight, 3 mana) [17]

* The knight moves to any adjacent empty square (all eight neighbors), instead of its usual jump.
* Phase Shift never captures.

### 3.3 Ghost Walk (bishop, 3 mana) [18]

* The bishop moves along one of its diagonals as usual, except that it may pass through exactly one square occupied
  by a friendly piece.
* Every other square crossed must be empty; it may never pass an enemy piece, nor two occupied squares.
* The destination must be empty and different from the starting square. Ghost Walk never captures.
* Only moves that actually pass through a friendly piece are Ghost Walks; an ordinary unobstructed bishop move is a
  normal move.

### 3.4 Castle Swap (rook, 4 mana) [19]

* The rook and a friendly bishop or knight exchange squares simultaneously.
* Both pieces must stand on the same rank or the same file, at most 3 squares apart, with every square strictly
  between them empty.
* Neither piece captures. The rook counts as moved: if it still held a castling right, that right is lost. A swap
  never creates or restores a castling right.

### 3.5 Royal Command (queen, 5 mana) [20]

* The queen stays where she is. A friendly pawn, knight, bishop or rook (never the king, another queen or an enemy
  piece) standing on a square adjacent to the queen moves to a new square.
* The destination must be empty, adjacent to the queen, adjacent to the commanded piece's square, and different
  from it.
* A pawn moved this way counts as moved (no later two-square advance) and may not be placed on its promotion rank;
  a pawn never promotes through Royal Command.
* A rook moved this way loses any castling right it still held. Royal Command never captures.

### 3.6 Royal Escape (king, 5 mana, once per game) [21]

* Allowed only while the player's king is in check, and only if the player has not used Royal Escape yet in this
  game.
* The king leaps exactly two squares in one of the eight directions (two horizontally, two vertically or two
  diagonally). The square in between may be occupied (the king jumps over it).
* The destination must be empty and must not be attacked by any enemy piece (normal attack rules).
* Royal Escape never captures. The king loses both castling rights for the rest of the game.
* **Checkmate and Royal Escape.** A king in check is mated only if no legal action exists, Royal Escape included:
  when the defender has at least 5 mana at the start of its turn and has not used Royal Escape, every escape
  destination is considered before a checkmate is declared.

---

## 4. Interactions and edge cases [24-31]

### 4.1 Castling, en passant, promotion

* Castling remains available in Arcane Chess. A rook moved by Castle Swap or Royal Command, and a king moved by
  Royal Escape, lose their castling rights. Returning a piece to its original square never restores a right.
* Only a normal two-square pawn advance creates an en passant right. No magical action creates one. An existing en
  passant right expires after the opponent's next action, even if that action is magical.
* Promotion happens only when a pawn reaches the last rank by a normal move (a normal advance or capture).

### 4.2 Halfmove counter (fifty and seventy-five move rules)

* The counter is reset by every capture and by every pawn movement, normal or magical (Shadow Step, a pawn moved by
  Royal Command).
* A magical action that moves only non-pawn pieces (Phase Shift, Ghost Walk, Castle Swap, Royal Escape, Royal
  Command of a piece) does not reset it.
* Because magical pawn moves reset the counter, the seventy-five-move rule alone does not bound the length of an
  Arcane game. An Arcane game that reaches **600 plies** is therefore drawn ("move limit"). Games played by the
  computer at every level average 95 to 137 plies (docs/BALANCE_REPORT.md).

### 4.3 Movement history of pawns

Each side records, for every file, whether the pawn that started on that file's second rank (seventh for Black) has
never moved. Only such a pawn may advance two squares. A pawn that reaches another file's starting square through
Shadow Step or Royal Command is not that file's original pawn and cannot advance two squares.

### 4.4 Repetition identity [28]

In Arcane Chess two positions are the same position only if all of the following are equal: piece placement, side
to move, castling rights, the en passant right (when a capture is legal), White's mana, Black's mana, whether each
player still has Royal Escape, and the pawn movement history (4.3). Boards with identical pieces but different
mana are different positions. The implementation compares a 64-bit Zobrist hash and then the full compact position,
so hash collisions cannot cause false repetitions.

### 4.5 Stalemate [30]

Stalemate occurs when the player to move is not in check and has neither a legal chess move nor a legal magical
action, counted with the player's actual mana after the turn-start regeneration. Royal Escape is never available
when the king is not in check.

### 4.6 Determinism [31]

Every action is deterministic: a given position and action always produce the same position. There are no hit
points, no random hits or misses, no random mana. Randomness is used only for cosmetic effects and, seeded and
reproducible, to vary the computer's choices at its lowest levels (docs/AI_DESIGN.md).

---

## 5. Scenario rules (campaign, challenges, tutorial, gauntlet)

### 5.1 Starting positions

Scenarios may start from a FEN position. Arcane FEN adds optional fields after the six standard ones:

* `mana=W/B`: current mana of White and Black (0 to the maximum);
* `escape=W/B`: Royal Escape uses left (1 = available, 0 = used);
* `home=hh/hh`: the pawn movement history (4.3) as two hexadecimal file masks.

A scenario states whether its FEN mana values are taken as they are or whether the side to move first receives its
turn-start regeneration.

### 5.2 Modifiers

An encounter may change, for the whole match and always announced in its briefing:

* the maximum mana (for example 4 instead of 6);
* which abilities each side may use (separately for the player and the opponent).

The six rules of section 3 never change.

### 5.3 Objectives

The player's moves are counted; the opponent's are not. "Within N moves" means the player's first N moves.

| Objective | Success | Failure |
|---|---|---|
| Win | The player wins the game (checkmate or the opponent resigns). | The player loses. |
| Checkmate within N | The player mates within N moves. | The N-th move did not mate. |
| Survive N moves | The player has made N moves without losing (a drawn game also counts as survived). | The player is mated or resigns. |
| Promote within N | A pawn of the player promotes within N moves. | N moves made without a promotion. |
| Win material (V pawns) within N | When it is the player's turn again (an immediate recapture counts), the player's material balance has improved by at least V x 100 centipawns (pawn 100, knight 320, bishop 330, rook 500, queen 900). | N moves made without it. |
| Capture all pawns within N | No enemy pawn remains. | N moves made with an enemy pawn left. |

A game won by the player always completes the scenario. When the objective is decided, the game ends with the
result "objective complete" or "objective failed".

---

## 6. Constants

| Constant | Value |
|---|---|
| Maximum mana | 6 |
| Regeneration per turn | 1 |
| Ability costs | 2, 3, 3, 4, 5, 5 |
| Royal Escape uses per game | 1 |
| Castle Swap maximum distance | 3 |
| Fifty-move claim | 100 plies |
| Seventy-five-move automatic draw | 150 plies |
| Threefold repetition claim / fivefold automatic draw | 3 / 5 occurrences |
| Arcane move limit | 600 plies |
