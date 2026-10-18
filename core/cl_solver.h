// CHESS LEGENDS: ARCANE WARS - exact puzzle solvers (host validation of challenges and scenario positions).
//
// These are exhaustive AND/OR searches over every legal action, including every magical ability of both sides,
// used to prove that a challenge has a forced solution (and that a themed ability is genuinely required).
#ifndef CL_SOLVER_H
#define CL_SOLVER_H

#include "cl_position.h"

namespace cl
{

// Optional memo for the boolean searches (host tools only): exact (position hash, moves left, query) entries, so
// transpositions inside a proof are searched once. Results of aborted searches are never stored.
struct SolverCache
{
    uint64_t* keys = nullptr;       // 0 = empty slot
    uint8_t* values = nullptr;
    uint32_t mask = 0;              // slot count - 1 (power of two)
};

struct SolverStats
{
    uint64_t nodes = 0;
    bool aborted = false;
    uint64_t node_limit = 200000000;
    SolverCache* cache = nullptr;
    Move root_only;                 // if set, the root considers only this action (proves that a given key works)
};

// True if the side to move can force checkmate within `moves` of its own moves against every defense.
[[nodiscard]] bool forced_mate(Position& pos, int moves, SolverStats& stats, Move* first_move = nullptr);

// Best material gain (centipawns, mover's point of view) the side to move can force after exactly `moves` of its own
// moves and the opponent's reply to the last one (minimax, both sides searching every action).
[[nodiscard]] int forced_material_gain(Position& pos, int moves, SolverStats& stats, Move* first_move = nullptr);

// True if the side to move can avoid being checkmated for `moves` of its own moves against every attack.
[[nodiscard]] bool can_survive(Position& pos, int moves, SolverStats& stats, Move* first_move = nullptr);

// True if the side to move can force a material lead of at least `gain` centipawns over the starting balance, measured
// at one of its own turns (after the opponent's reply) within `moves` moves, or checkmate - exactly the WIN_MATERIAL
// objective of ObjectiveTracker. A stalemate fails.
[[nodiscard]] bool forced_material_lead(Position& pos, int moves, int gain, SolverStats& stats,
                                        Move* first_move = nullptr);

// True if the side to move can force one of its pawns to promote within `moves` of its own moves against every
// defense. Strict: a game that ends first (either side mated or stalemated) does not count as a promotion.
[[nodiscard]] bool forced_promotion(Position& pos, int moves, SolverStats& stats, Move* first_move = nullptr);

}

#endif
