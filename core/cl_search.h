// CHESS LEGENDS: ARCANE WARS - game tree search.
//
// Negamax alpha-beta with iterative deepening, principal variation search, a transposition table (Zobrist
// keys), killer and history move ordering, check extensions, conservative late move reductions and a
// check-aware quiescence search. Normal moves and magical abilities are alternatives in the same tree.
//
// The search is bounded by a deterministic node budget (identical results on the GBA and on the host) and calls
// an optional poll function every 256 nodes so the console can keep animating, playing music and reading input
// while the computer thinks; the poll function may cancel the search.
#ifndef CL_SEARCH_H
#define CL_SEARCH_H

#include "cl_eval.h"
#include "cl_movegen.h"

namespace cl
{

constexpr int SCORE_INFINITE = 32000;
constexpr int SCORE_MATE = 30000;
constexpr int SCORE_MATE_BOUND = SCORE_MATE - 512;
constexpr int SEARCH_MAX_PLY = 40;

struct TTEntry
{
    uint32_t key;
    uint32_t move;
    int16_t score;
    int8_t depth;
    uint8_t bound;      // bits 0-1: bound type; bits 2-7: search generation (other generations count as empty)
};

class TranspositionTable
{
public:
    static constexpr uint8_t BOUND_NONE = 0;
    static constexpr uint8_t BOUND_UPPER = 1;
    static constexpr uint8_t BOUND_LOWER = 2;
    static constexpr uint8_t BOUND_EXACT = 3;

    void init(TTEntry* storage, int entries_power_of_two);
    void clear();

    // Starts a new search. Entries stored by earlier searches then behave exactly like empty ones (probe misses,
    // store replaces), so every search sees the same table as after clear() without paying for the clear; the
    // table is really cleared when the 6-bit generation wraps (every 63 searches).
    void new_search();

    [[nodiscard]] const TTEntry* probe(uint64_t hash) const;
    void store(uint64_t hash, Move move, int score, int depth, uint8_t bound);

    [[nodiscard]] static uint8_t bound_of(const TTEntry& entry)
    {
        return entry.bound & 3;
    }

    [[nodiscard]] int size() const
    {
        return _mask + 1;
    }

private:
    TTEntry* _entries = nullptr;
    int _mask = 0;
    uint8_t _generation = 1;

    [[nodiscard]] bool _current(const TTEntry& entry) const
    {
        return (entry.bound & 3) != BOUND_NONE && (entry.bound >> 2) == _generation;
    }
};

struct SearchLimits
{
    uint32_t max_nodes = 50000;     // deterministic budget (depth 1 always completes)
    int max_depth = 32;
    int max_quiescence_depth = 8;
    int quiescence_magic_checks = 6;  // forcing magical actions examined at the first quiescence ply
    int draw_score = 0;             // contempt: score of a draw for the side to move at the root
    bool use_lmr = true;
    bool use_null_move = true;      // null-move pruning (never in check, never without pieces, never twice in a row)
    uint32_t poll_mask = 255;       // poll every (mask + 1) nodes; the console uses 7 to keep 60 fps while thinking
};

struct SearchResult
{
    Move best;
    int score = 0;
    int depth = 0;                  // last fully completed iteration
    int seldepth = 0;
    uint32_t nodes = 0;
    bool aborted = false;           // cancelled through the poll function
    Move pv[SEARCH_MAX_PLY];
    int pv_length = 0;
    int root_count = 0;
};

// Return true to cancel the search.
using PollFunction = bool (*)(void* user, uint32_t nodes);

struct EvalCacheEntry
{
    uint32_t key;
    int16_t score;
    int16_t valid;      // generation of the search that stored it (SearchMemory::eval_generation)
};

// Large working storage; the caller decides where it lives (EWRAM on the console).
struct SearchMemory
{
    static constexpr int move_stack_capacity = 3072;
    static constexpr int path_capacity = 704;    // game window (<= 608) + search path
    static constexpr int eval_cache_size = 2048;

    EvalCacheEntry eval_cache[eval_cache_size];
    int16_t eval_generation = 0;    // entries of other generations are empty (the cache depends on the profile)

    Move moves[move_stack_capacity];
    int32_t scores[move_stack_capacity];
    Move root_moves[MAX_MOVES];
    int32_t root_order[MAX_MOVES];
    int32_t iteration_scores[MAX_MOVES];
    uint64_t path[path_capacity];
    Move pv_table[SEARCH_MAX_PLY][SEARCH_MAX_PLY];
    int8_t pv_length[SEARCH_MAX_PLY];
    Move killers[SEARCH_MAX_PLY][2];
    int16_t history[16][64];
    uint8_t repetition_filter[256];
};

class Searcher
{
public:
    Searcher(SearchMemory& memory, TranspositionTable& tt) :
        _mem(memory),
        _tt(tt)
    {
    }

    // history: Zobrist hashes of the game positions before the root (oldest first), used for repetition draws.
    SearchResult search(const Position& root, const uint64_t* history, int history_count, const SearchLimits& limits,
                        const EvalProfile& profile, PollFunction poll = nullptr, void* poll_user = nullptr);

    // Controlled imperfection at the low levels: writes the legal root moves whose quick (quiescence) score is at
    // most `margin` below the quick score of `anchor` (the move the search chose; listed first). Zero-window
    // tests keep it cheap; it polls like the search and never spends more than `limits.max_nodes` nodes. Moves
    // not tested when the budget runs out are simply not offered. Returns the count (0 when nothing applies).
    int acceptable_root_moves(const Position& root, Move anchor, int margin, const SearchLimits& limits,
                              const EvalProfile& profile, Move* moves, int capacity, PollFunction poll = nullptr,
                              void* poll_user = nullptr);

    // Stack guard: when a node's stack address falls below this limit, the node is evaluated statically instead of
    // being expanded (0 disables the guard; the console sets it to the bottom of the AI stack plus a margin).
    void set_stack_limit(uintptr_t limit)
    {
        _stack_limit = limit;
    }

    // Nodes visited by the last search or imperfection pass.
    [[nodiscard]] uint32_t last_nodes() const
    {
        return _nodes;
    }

private:
    uintptr_t _stack_limit = 0;
    SearchMemory& _mem;
    TranspositionTable& _tt;
    Position _pos;
    SearchLimits _limits;
    EvalProfile _profile;
    PollFunction _poll = nullptr;
    void* _poll_user = nullptr;
    uint32_t _nodes = 0;
    int _seldepth = 0;
    int _stack_top = 0;
    int _path_count = 0;
    int _history_floor = 0;
    Color _root_side = WHITE;
    bool _aborted = false;
    bool _budget_active = false;

    [[nodiscard]] bool _stack_exhausted() const;
    [[nodiscard]] int _evaluate();
    [[nodiscard]] int _evaluate_lazy(int alpha, int beta);
    int _negamax(int depth, int ply, int alpha, int beta, bool pv_node, bool in_check, bool allow_null);
    int _quiesce(int alpha, int beta, int ply, int qdepth, bool in_check);
    bool _check_abort();
    void _score_moves(int start, int end, Move tt_move, int ply);
    Move _pick(int index, int end);
    [[nodiscard]] bool _is_repetition() const;
    [[nodiscard]] bool _insufficient_material() const;
    void _push_path(uint64_t hash);
    void _pop_path();
    [[nodiscard]] int _draw_score(int ply) const;
};

}

#endif
