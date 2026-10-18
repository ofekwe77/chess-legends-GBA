// CHESS LEGENDS: ARCANE WARS - search setup and root iteration (cold code; the node search lives in
// cl_search.bn_iwram.cpp).
#include "cl_search.h"

namespace cl
{

void TranspositionTable::init(TTEntry* storage, int entries_power_of_two)
{
    _entries = storage;
    _mask = entries_power_of_two - 1;
    CL_ASSERT((entries_power_of_two & _mask) == 0);
    clear();
}

void TranspositionTable::clear()
{
    for(int i = 0; i <= _mask; ++i)
    {
        _entries[i] = TTEntry{ 0, 0, 0, 0, BOUND_NONE };
    }
}

SearchResult Searcher::search(const Position& root, const uint64_t* history, int history_count,
                              const SearchLimits& limits, const EvalProfile& profile, PollFunction poll,
                              void* poll_user)
{
    SearchResult result;
    _pos = root;
    _limits = limits;
    _profile = profile;
    _poll = poll;
    _poll_user = poll_user;
    _nodes = 0;
    _seldepth = 0;
    _stack_top = 0;
    _aborted = false;
    _budget_active = false;
    _root_side = root.side;

    // Deterministic start: identical inputs always produce identical searches.
    _tt.clear();

    for(int p = 0; p < SEARCH_MAX_PLY; ++p)
    {
        _mem.killers[p][0] = Move();
        _mem.killers[p][1] = Move();
    }

    for(int p = 0; p < 16; ++p)
    {
        for(int s = 0; s < 64; ++s)
        {
            _mem.history[p][s] = 0;
        }
    }

    for(int i = 0; i < 256; ++i)
    {
        _mem.repetition_filter[i] = 0;
    }

    for(EvalCacheEntry& entry : _mem.eval_cache)
    {
        entry.valid = 0;
    }

    // Game history (positions before the root).
    _path_count = 0;
    const int keep = SearchMemory::path_capacity - SEARCH_MAX_PLY - 8;
    int first = history_count > keep ? history_count - keep : 0;

    for(int i = first; i < history_count; ++i)
    {
        _push_path(history[i]);
    }

    _history_floor = 0;

    // Root moves, ordered by the move-ordering heuristics.
    Move* root_moves = _mem.root_moves;
    int32_t* root_order = _mem.root_order;
    int32_t* iteration_scores = _mem.iteration_scores;
    int root_count = 0;
    {
        MoveBuffer buffer(_mem.moves, SearchMemory::move_stack_capacity);
        generate_pseudo(_pos, buffer);

        for(int i = 0; i < buffer.count && root_count < MAX_MOVES; ++i)
        {
            if(leaves_king_safe(_pos, buffer.moves[i]))
            {
                root_moves[root_count++] = buffer.moves[i];
            }
        }
    }

    result.root_count = root_count;

    if(root_count == 0)
    {
        return result;
    }

    for(int i = 0; i < root_count; ++i)
    {
        _mem.moves[i] = root_moves[i];
    }

    _score_moves(0, root_count, Move(), 0);

    for(int i = 0; i < root_count; ++i)
    {
        root_order[i] = _mem.scores[i];
    }

    for(int i = 1; i < root_count; ++i)
    {
        Move m = root_moves[i];
        int32_t s = root_order[i];
        int j = i - 1;

        while(j >= 0 && root_order[j] < s)
        {
            root_moves[j + 1] = root_moves[j];
            root_order[j + 1] = root_order[j];
            --j;
        }

        root_moves[j + 1] = m;
        root_order[j + 1] = s;
    }

    result.best = root_moves[0];

    if(root_count == 1)
    {
        result.score = evaluate(_pos, _profile);
        result.pv[0] = result.best;
        result.pv_length = 1;
        return result;
    }

    for(int depth = 1; depth <= limits.max_depth && depth < SEARCH_MAX_PLY - 2; ++depth)
    {
        _budget_active = depth > 1;
        int alpha = -SCORE_INFINITE;
        const int beta = SCORE_INFINITE;
        int best_score = -SCORE_INFINITE;
        Move best_move;
        _stack_top = 0;

        for(int i = 0; i < root_count; ++i)
        {
            const Move move = root_moves[i];
            Undo undo;
            _pos.make(move, undo);
            const bool gives_check = _pos.in_check();
            _push_path(undo.hash);
            _mem.pv_length[1] = 1;
            int new_depth = depth - 1 + (gives_check ? 1 : 0);
            int score;

            if(i == 0)
            {
                score = -_negamax(new_depth, 1, -beta, -alpha, true, gives_check, true);
            }
            else
            {
                score = -_negamax(new_depth, 1, -alpha - 1, -alpha, false, gives_check, true);

                if(score > alpha && ! _aborted)
                {
                    score = -_negamax(new_depth, 1, -beta, -alpha, true, gives_check, true);
                }
            }

            _pop_path();
            _pos.unmake(move, undo);

            if(_aborted)
            {
                break;
            }

            iteration_scores[i] = score;

            if(score > best_score)
            {
                best_score = score;
                best_move = move;

                if(score > alpha)
                {
                    alpha = score;
                    result.pv[0] = move;
                    int length = 1;

                    for(int j = 1; j < _mem.pv_length[1] && length < SEARCH_MAX_PLY; ++j)
                    {
                        result.pv[length++] = _mem.pv_table[1][j];
                    }

                    result.pv_length = length;
                }
            }
        }

        if(_aborted)
        {
            // A move that beat the previous best with a full-window search in the unfinished iteration is a
            // reliable improvement; otherwise keep the last completed iteration's choice.
            if(! best_move.is_null() && best_move != result.best && best_score > result.score && result.depth > 0)
            {
                result.best = best_move;
                result.score = best_score;
            }

            break;
        }

        result.best = best_move;
        result.score = best_score;
        result.depth = depth;

        // Reorder: best first, then by this iteration's scores (stable).
        for(int i = 1; i < root_count; ++i)
        {
            Move m = root_moves[i];
            int32_t s = iteration_scores[i];
            int j = i - 1;

            while(j >= 0 && iteration_scores[j] < s)
            {
                root_moves[j + 1] = root_moves[j];
                iteration_scores[j + 1] = iteration_scores[j];
                --j;
            }

            root_moves[j + 1] = m;
            iteration_scores[j + 1] = s;
        }

        if(best_score >= SCORE_MATE_BOUND || best_score <= -SCORE_MATE_BOUND)
        {
            break;
        }

        // The next iteration would most likely not finish within the budget.
        if(depth > 1 && _nodes * 2 > limits.max_nodes)
        {
            break;
        }
    }

    result.nodes = _nodes;
    result.seldepth = _seldepth;
    result.aborted = _aborted && ! (_budget_active && _nodes >= _limits.max_nodes);
    return result;
}

int Searcher::score_root_moves(const Position& root, const SearchLimits& limits, const EvalProfile& profile,
                               Move* moves, int* scores, int capacity)
{
    _pos = root;
    _limits = limits;
    _profile = profile;
    _poll = nullptr;
    _nodes = 0;
    _stack_top = 0;
    _aborted = false;
    _budget_active = false;
    _root_side = root.side;
    _path_count = 0;
    _history_floor = 0;

    for(int i = 0; i < 256; ++i)
    {
        _mem.repetition_filter[i] = 0;
    }

    for(EvalCacheEntry& entry : _mem.eval_cache)
    {
        entry.valid = 0;
    }

    int count = 0;
    MoveBuffer buffer(_mem.root_moves, MAX_MOVES);
    generate_pseudo(_pos, buffer);
    _stack_top = 0;

    for(int i = 0; i < buffer.count && count < capacity; ++i)
    {
        Move move = buffer.moves[i];

        if(! leaves_king_safe(_pos, move))
        {
            continue;
        }

        Undo undo;
        _pos.make(move, undo);
        bool check = _pos.in_check();
        _push_path(undo.hash);
        int score = -_quiesce(-SCORE_INFINITE, SCORE_INFINITE, 1, 0, check);
        _pop_path();
        _pos.unmake(move, undo);
        moves[count] = move;
        scores[count] = score;
        ++count;
    }

    return count;
}

}
