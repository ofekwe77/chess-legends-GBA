// CHESS LEGENDS: ARCANE WARS - node search: negamax, quiescence, move ordering, transposition table access.
#include "cl_search.h"

#include "cl_tables.h"

// Placement of the node search: ROM (Thumb). IWRAM holds move generation, make / unmake and the evaluation, which
// dominate the cost per node (see docs/AI_DESIGN.md, performance section).
#define CL_SEARCH_CODE

namespace cl
{

namespace
{
    constexpr int32_t ORDER_TT = 2000000000;
    constexpr int32_t ORDER_CAPTURE = 1000000;
    constexpr int32_t ORDER_PROMOTION = 900000;
    constexpr int32_t ORDER_MAGIC_CHECK = 850000;   // quiescence only
    constexpr int32_t ORDER_KILLER_1 = 800000;
    constexpr int32_t ORDER_KILLER_2 = 790000;

    // Best-first selection is used for the first moves of a node (where cut-offs happen); later moves are tried in
    // their current order, which avoids quadratic selection at nodes that search everything.
    constexpr int ordered_moves = 8;

    // Own pieces pinned against the own king (absolute pins along ranks, files and diagonals).
    struct Pins
    {
        uint8_t square[8];
        int count = -1;   // -1: not computed yet

        [[nodiscard]] bool contains(int sq) const
        {
            for(int i = 0; i < count; ++i)
            {
                if(square[i] == sq)
                {
                    return true;
                }
            }

            return false;
        }
    };

    void compute_pins(const Position& pos, Color us, Pins& pins)
    {
        pins.count = 0;
        const int king = pos.king_square(us);

        for(int d = 0; d < 8; ++d)
        {
            const int step = king_offsets[d];
            const bool diagonal = d >= 4;   // king_offsets: 4 orthogonal steps, then 4 diagonal steps
            int s = king + step;
            int candidate = NO_SQUARE;

            while(on_board(s))
            {
                const Piece p = pos.board[s];

                if(p != EMPTY)
                {
                    if(color_of(p) == us)
                    {
                        if(candidate != NO_SQUARE)
                        {
                            break;
                        }

                        candidate = s;
                    }
                    else
                    {
                        const PieceType t = type_of(p);

                        if(candidate != NO_SQUARE && (t == QUEEN || (diagonal ? t == BISHOP : t == ROOK)))
                        {
                            pins.square[pins.count++] = uint8_t(candidate);
                        }

                        break;
                    }
                }

                s += step;
            }
        }
    }

    // When not in check, a move of an unpinned non-king piece cannot expose the own king, with two exceptions:
    // en passant (two pieces leave the rank) and castling (verified separately). A Castle Swap keeps both squares
    // occupied, so it is legal whenever the king is not in check.
    [[nodiscard]] bool needs_safety_test(const Position& pos, Move move, bool in_check, Pins& pins)
    {
        if(in_check)
        {
            return true;
        }

        switch(move.kind())
        {
        case MK_CASTLE_SWAP:
            return false;

        case MK_EN_PASSANT:
        case MK_CASTLE_KING:
        case MK_CASTLE_QUEEN:
        case MK_ROYAL_ESCAPE:
            return true;

        default:
            break;
        }

        const int from = move.from();

        if(from == pos.king_square(pos.side))
        {
            return true;
        }

        if(pins.count < 0)
        {
            compute_pins(pos, pos.side, pins);
        }

        return pins.contains(from);
    }

    [[nodiscard]] int score_to_tt(int score, int ply)
    {
        if(score >= SCORE_MATE_BOUND)
        {
            return score + ply;
        }

        if(score <= -SCORE_MATE_BOUND)
        {
            return score - ply;
        }

        return score;
    }

    [[nodiscard]] int score_from_tt(int score, int ply)
    {
        if(score >= SCORE_MATE_BOUND)
        {
            return score - ply;
        }

        if(score <= -SCORE_MATE_BOUND)
        {
            return score + ply;
        }

        return score;
    }

    [[nodiscard]] bool is_quiet(Move move)
    {
        return ! move.is_capture() && move.kind() != MK_PROMOTION;
    }

    [[nodiscard]] int filter_bucket(uint64_t hash)
    {
        return int((hash >> 24) & 255);
    }

    [[nodiscard]] int capture_order(const Position& pos, Move move)
    {
        Piece victim = move.kind() == MK_EN_PASSANT ? Piece(PAWN) : pos.board[move.to()];
        int score = ORDER_CAPTURE + piece_value[type_of(victim)] * 64 - type_of(pos.board[move.from()]);

        if(move.kind() == MK_PROMOTION)
        {
            score += piece_value[move.promotion()];
        }

        return score;
    }

    // Null-move pruning is unsafe without pieces (zugzwang is common in pawn endgames).
    [[nodiscard]] bool has_pieces(const Position& pos, Color c)
    {
        return pos.piece_count[make_piece(c, KNIGHT)] + pos.piece_count[make_piece(c, BISHOP)] +
               pos.piece_count[make_piece(c, ROOK)] + pos.piece_count[make_piece(c, QUEEN)] > 0;
    }
}

// ---------------------------------------------------------------------------------------------------------------
// Transposition table (hot part)
// ---------------------------------------------------------------------------------------------------------------

CL_SEARCH_CODE const TTEntry* TranspositionTable::probe(uint64_t hash) const
{
    const TTEntry& entry = _entries[uint32_t(hash) & uint32_t(_mask)];

    if(_current(entry) && entry.key == uint32_t(hash >> 32))
    {
        return &entry;
    }

    return nullptr;
}

CL_SEARCH_CODE void TranspositionTable::store(uint64_t hash, Move move, int score, int depth, uint8_t bound)
{
    TTEntry& entry = _entries[uint32_t(hash) & uint32_t(_mask)];
    uint32_t key = uint32_t(hash >> 32);

    // Depth-preferred replacement, but always keep fresher information for the same position. Entries of earlier
    // searches are empty.
    const bool current = _current(entry);

    if(current && entry.key != key && entry.depth > depth + 2)
    {
        return;
    }

    if(current && entry.key == key && move.is_null())
    {
        move = Move(entry.move);
    }

    entry.key = key;
    entry.move = move.data();
    entry.score = int16_t(score);
    entry.depth = int8_t(depth);
    entry.bound = uint8_t(bound | (_generation << 2));
}

// ---------------------------------------------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------------------------------------------

CL_SEARCH_CODE bool Searcher::_check_abort()
{
    if(_aborted)
    {
        return true;
    }

    if(_budget_active && _nodes >= _limits.max_nodes)
    {
        _aborted = true;
        return true;
    }

    if(_poll && (_nodes & _limits.poll_mask) == 0 && _poll(_poll_user, _nodes))
    {
        _aborted = true;
    }

    return _aborted;
}

CL_SEARCH_CODE void Searcher::_push_path(uint64_t hash)
{
    CL_DEBUG_ASSERT(_path_count < SearchMemory::path_capacity);
    _mem.path[_path_count++] = hash;
    ++_mem.repetition_filter[filter_bucket(hash)];
}

CL_SEARCH_CODE void Searcher::_pop_path()
{
    uint64_t hash = _mem.path[--_path_count];
    --_mem.repetition_filter[filter_bucket(hash)];
}

CL_SEARCH_CODE bool Searcher::_is_repetition() const
{
    const uint64_t hash = _pos.hash;

    if(! _mem.repetition_filter[filter_bucket(hash)])
    {
        return false;
    }

    // The path holds the positions before the current one; positions with the same side to move are an even
    // number of plies back. Classic chess positions cannot repeat across an irreversible move (halfmove reset);
    // Arcane positions can (magical pawn moves are reversible), so the whole window is examined. Positions before
    // a null move never count (_history_floor).
    int floor = _history_floor;

    if(! _pos.rules.arcane)
    {
        int limit = _path_count - _pos.halfmove;

        if(limit > floor)
        {
            floor = limit;
        }
    }

    for(int i = _path_count - 2; i >= floor; i -= 2)
    {
        if(_mem.path[i] == hash)
        {
            return true;
        }
    }

    return false;
}

CL_SEARCH_CODE bool Searcher::_insufficient_material() const
{
    const uint8_t* counts = _pos.piece_count;

    if(counts[W_PAWN] || counts[B_PAWN] || counts[W_ROOK] || counts[B_ROOK] || counts[W_QUEEN] || counts[B_QUEEN])
    {
        return false;
    }

    return counts[W_KNIGHT] + counts[B_KNIGHT] + counts[W_BISHOP] + counts[B_BISHOP] <= 1;
}

CL_SEARCH_CODE int Searcher::_draw_score(int ply) const
{
    // Contempt is defined for the root side; negamax scores are for the side to move.
    (void) ply;
    return _pos.side == _root_side ? _limits.draw_score : -_limits.draw_score;
}

CL_SEARCH_CODE bool Searcher::_stack_exhausted() const
{
    if(! _stack_limit)
    {
        return false;
    }

    volatile char probe = 0;
    return reinterpret_cast<uintptr_t>(&probe) < _stack_limit;
}

CL_SEARCH_CODE int Searcher::_evaluate()
{
    // The hash covers the side to move, so a cached score is already from the right point of view.
    EvalCacheEntry& entry = _mem.eval_cache[uint32_t(_pos.hash) & (SearchMemory::eval_cache_size - 1)];
    const uint32_t key = uint32_t(_pos.hash >> 32);

    if(entry.valid == _mem.eval_generation && entry.key == key)
    {
        return entry.score;
    }

    const int score = evaluate(_pos, _profile);
    entry.key = key;
    entry.score = int16_t(score);
    entry.valid = _mem.eval_generation;
    return score;
}

CL_SEARCH_CODE int Searcher::_evaluate_lazy(int alpha, int beta)
{
    const int quick = evaluate_quick(_pos);
    const int phase = _pos.phase > 24 ? 24 : _pos.phase;
    const int margin = 260 + (24 - phase) * 10;

    if(quick - margin >= beta || quick + margin <= alpha)
    {
        return quick;
    }

    return _evaluate();
}

CL_SEARCH_CODE void Searcher::_score_moves(int start, int end, Move tt_move, int ply)
{
    const Move killer1 = _mem.killers[ply][0];
    const Move killer2 = _mem.killers[ply][1];

    for(int i = start; i < end; ++i)
    {
        const Move move = _mem.moves[i];
        int32_t score;

        if(move == tt_move)
        {
            score = ORDER_TT;
        }
        else if(move.is_capture())
        {
            score = capture_order(_pos, move);
        }
        else if(move.kind() == MK_PROMOTION)
        {
            score = ORDER_PROMOTION + piece_value[move.promotion()];
        }
        else if(move == killer1)
        {
            score = ORDER_KILLER_1;
        }
        else if(move == killer2)
        {
            score = ORDER_KILLER_2;
        }
        else
        {
            Piece piece = _pos.board[move.from()];
            score = _mem.history[piece][sq64(move.to())];

            // Magic costs mana: try it after equally promising normal moves.
            if(move.is_magic())
            {
                score -= 64 + _pos.rules.cost[move.ability()] * 8;
            }
        }

        _mem.scores[i] = score;
    }
}

CL_SEARCH_CODE Move Searcher::_pick(int index, int end)
{
    int best = index;
    int32_t best_score = _mem.scores[index];

    for(int i = index + 1; i < end; ++i)
    {
        if(_mem.scores[i] > best_score)
        {
            best = i;
            best_score = _mem.scores[i];
        }
    }

    if(best != index)
    {
        Move m = _mem.moves[index];
        _mem.moves[index] = _mem.moves[best];
        _mem.moves[best] = m;
        _mem.scores[best] = _mem.scores[index];
        _mem.scores[index] = best_score;
    }

    return _mem.moves[index];
}

// ---------------------------------------------------------------------------------------------------------------
// Quiescence
// ---------------------------------------------------------------------------------------------------------------

CL_SEARCH_CODE int Searcher::_quiesce(int alpha, int beta, int ply, int qdepth, bool in_check)
{
    if(_check_abort())
    {
        return 0;
    }

    ++_nodes;

    if(ply > _seldepth)
    {
        _seldepth = ply;
    }

    if(ply >= SEARCH_MAX_PLY - 1 || _stack_top + MAX_MOVES >= SearchMemory::move_stack_capacity || _stack_exhausted())
    {
        return _evaluate();
    }

    if(ply > 0 && (_is_repetition() || _insufficient_material()))
    {
        return _draw_score(ply);
    }

    const Color us = _pos.side;
    int best;
    const int start = _stack_top;
    MoveBuffer buffer(_mem.moves + start, SearchMemory::move_stack_capacity - start);

    if(in_check)
    {
        // Never stand pat in check: every evasion (including magical defenses) is searched.
        if(qdepth >= _limits.max_quiescence_depth + 4)
        {
            return _evaluate();
        }

        generate_pseudo(_pos, buffer);
        best = -SCORE_MATE + ply;
    }
    else
    {
        int stand_pat = _evaluate_lazy(alpha, beta);

        if(stand_pat >= beta)
        {
            return stand_pat;
        }

        if(stand_pat > alpha)
        {
            alpha = stand_pat;
        }

        best = stand_pat;

        if(qdepth >= _limits.max_quiescence_depth)
        {
            return stand_pat;
        }

        generate_tactical(_pos, buffer);

        // A bounded selection of forcing magical actions (direct checks) at the first quiescence ply: a quiet
        // repositioning can create a decisive check that pure capture search would miss.
        if(qdepth == 0 && _pos.rules.arcane && _limits.quiescence_magic_checks > 0 && _pos.mana[us] >= 2)
        {
            const int magic_start = buffer.count;
            generate_arcane_checks(_pos, buffer);
            int write = magic_start;
            int kept = 0;

            for(int i = magic_start; i < buffer.count && kept < _limits.quiescence_magic_checks; ++i)
            {
                Move m = buffer.moves[i];
                Undo undo;
                _pos.make(m, undo);
                bool legal = ! _pos.in_check(us);
                bool check = legal && _pos.in_check();
                _pos.unmake(m, undo);

                if(check)
                {
                    buffer.moves[write++] = m;
                    ++kept;
                }
            }

            buffer.count = write;
        }
    }

    const int end = start + buffer.count;
    _stack_top = end;

    for(int i = start; i < end; ++i)
    {
        const Move move = _mem.moves[i];
        int32_t score;

        if(move.is_capture())
        {
            score = capture_order(_pos, move);
        }
        else if(move.kind() == MK_PROMOTION)
        {
            score = ORDER_PROMOTION + piece_value[move.promotion()];
        }
        else if(move.is_magic())
        {
            score = in_check ? 1000 : ORDER_MAGIC_CHECK;
        }
        else
        {
            score = _mem.history[_pos.board[move.from()]][sq64(move.to())];
        }

        _mem.scores[i] = score;
    }

    int legal = 0;
    const int stand_for_delta = best;
    Pins pins;

    for(int i = start; i < end; ++i)
    {
        const Move move = _pick(i, end);

        // Delta pruning: a capture that cannot raise alpha even with a margin is skipped (not in check).
        if(! in_check && move.is_capture() && move.kind() != MK_PROMOTION)
        {
            Piece victim = move.kind() == MK_EN_PASSANT ? Piece(PAWN) : _pos.board[move.to()];

            if(stand_for_delta + piece_value[type_of(victim)] + 180 < alpha)
            {
                continue;
            }
        }

        if(is_castling(move) && ! castling_path_safe(_pos, move))
        {
            continue;
        }

        const bool test = needs_safety_test(_pos, move, in_check, pins);
        Undo undo;
        _pos.make(move, undo);

        CL_DEBUG_ASSERT(test || ! _pos.in_check(us));   // host builds verify every skipped safety test

        if(test && _pos.in_check(us))
        {
            _pos.unmake(move, undo);
            continue;
        }

        ++legal;
        bool gives_check = _pos.in_check();
        _push_path(undo.hash);
        int score = -_quiesce(-beta, -alpha, ply + 1, qdepth + 1, gives_check);
        _pop_path();
        _pos.unmake(move, undo);

        if(_aborted)
        {
            _stack_top = start;
            return 0;
        }

        if(score > best)
        {
            best = score;

            if(score > alpha)
            {
                alpha = score;

                if(score >= beta)
                {
                    break;
                }
            }
        }
    }

    _stack_top = start;

    if(in_check && legal == 0)
    {
        return -SCORE_MATE + ply;
    }

    return best;
}

// ---------------------------------------------------------------------------------------------------------------
// Main search
// ---------------------------------------------------------------------------------------------------------------

CL_SEARCH_CODE int Searcher::_negamax(int depth, int ply, int alpha, int beta, bool pv_node, bool in_check,
                                      bool allow_null)
{
    _mem.pv_length[ply] = int8_t(ply);

    if(_check_abort())
    {
        return 0;
    }

    if(ply > 0)
    {
        if(_is_repetition() || _insufficient_material())
        {
            return _draw_score(ply);
        }

        if(_pos.halfmove >= config::seventy_five_move_plies && ! in_check)
        {
            return _draw_score(ply);
        }

        if(_pos.rules.arcane && _pos.ply >= config::arcane_max_plies)
        {
            return _draw_score(ply);
        }

        // Mate distance pruning.
        int mated = -SCORE_MATE + ply;
        int mating = SCORE_MATE - ply - 1;

        if(alpha < mated)
        {
            alpha = mated;
        }

        if(beta > mating)
        {
            beta = mating;
        }

        if(alpha >= beta)
        {
            return alpha;
        }
    }

    if(depth <= 0)
    {
        return _quiesce(alpha, beta, ply, 0, in_check);
    }

    ++_nodes;

    if(ply >= SEARCH_MAX_PLY - 2 || _stack_top + MAX_MOVES >= SearchMemory::move_stack_capacity || _stack_exhausted())
    {
        return _evaluate();
    }

    Move tt_move;

    if(const TTEntry* entry = _tt.probe(_pos.hash))
    {
        tt_move = Move(entry->move);

        if(! pv_node && entry->depth >= depth)
        {
            int score = score_from_tt(entry->score, ply);

            const uint8_t bound = TranspositionTable::bound_of(*entry);

            if(bound == TranspositionTable::BOUND_EXACT ||
               (bound == TranspositionTable::BOUND_LOWER && score >= beta) ||
               (bound == TranspositionTable::BOUND_UPPER && score <= alpha))
            {
                return score;
            }
        }
    }

    const Color us = _pos.side;

    // Null-move pruning: if passing the turn (the opponent regenerates mana as on a real turn) still fails high
    // with a reduced search, the position is good enough to cut. Not in check, not at PV nodes, not twice in a row,
    // not without pieces (zugzwang), and positions before the pass never count as repetitions.
    if(_limits.use_null_move && allow_null && ! pv_node && ! in_check && depth >= 3 && beta < SCORE_MATE_BOUND &&
       has_pieces(_pos, us) && evaluate_quick(_pos) >= beta)
    {
        const int reduction = depth >= 6 ? 3 : 2;
        Undo undo;
        _pos.make_null(undo);
        _push_path(undo.hash);
        const int saved_floor = _history_floor;
        _history_floor = _path_count;
        int score = -_negamax(depth - 1 - reduction, ply + 1, -beta, -beta + 1, false, false, false);
        _history_floor = saved_floor;
        _pop_path();
        _pos.unmake_null(undo);

        if(_aborted)
        {
            return 0;
        }

        if(score >= beta)
        {
            return score >= SCORE_MATE_BOUND ? beta : score;
        }
    }

    const int start = _stack_top;
    MoveBuffer buffer(_mem.moves + start, SearchMemory::move_stack_capacity - start);
    generate_pseudo(_pos, buffer);
    const int end = start + buffer.count;
    _stack_top = end;
    _score_moves(start, end, tt_move, ply);

    const int original_alpha = alpha;
    int best = -SCORE_INFINITE;
    Move best_move;
    int legal = 0;
    Pins pins;

    for(int i = start; i < end; ++i)
    {
        const Move move = i - start < ordered_moves ? _pick(i, end) : _mem.moves[i];

        if(is_castling(move) && ! castling_path_safe(_pos, move))
        {
            continue;
        }

        const bool test = needs_safety_test(_pos, move, in_check, pins);
        Undo undo;
        _pos.make(move, undo);

        CL_DEBUG_ASSERT(test || ! _pos.in_check(us));   // host builds verify every skipped safety test

        if(test && _pos.in_check(us))
        {
            _pos.unmake(move, undo);
            continue;
        }

        ++legal;
        const bool gives_check = _pos.in_check();
        const bool quiet = is_quiet(move);

        // Check extension (bounded by the ply limit).
        int new_depth = depth - 1;

        if(gives_check && ply < SEARCH_MAX_PLY / 2)
        {
            ++new_depth;
        }

        _push_path(undo.hash);
        int score;

        if(legal == 1)
        {
            score = -_negamax(new_depth, ply + 1, -beta, -alpha, pv_node, gives_check, true);
        }
        else
        {
            int reduction = 0;

            if(_limits.use_lmr && depth >= 3 && legal > 3 && ! in_check && ! gives_check && quiet &&
               move != _mem.killers[ply][0] && move != _mem.killers[ply][1])
            {
                reduction = (legal > 10 && depth >= 5) ? 2 : 1;
            }

            score = -_negamax(new_depth - reduction, ply + 1, -alpha - 1, -alpha, false, gives_check, true);

            if(score > alpha && reduction && ! _aborted)
            {
                score = -_negamax(new_depth, ply + 1, -alpha - 1, -alpha, false, gives_check, true);
            }

            if(score > alpha && score < beta && ! _aborted)
            {
                score = -_negamax(new_depth, ply + 1, -beta, -alpha, true, gives_check, true);
            }
        }

        _pop_path();
        _pos.unmake(move, undo);

        if(_aborted)
        {
            _stack_top = start;
            return 0;
        }

        if(score > best)
        {
            best = score;
            best_move = move;

            if(score > alpha)
            {
                alpha = score;

                // Triangular principal variation.
                _mem.pv_table[ply][ply] = move;

                for(int j = ply + 1; j < _mem.pv_length[ply + 1]; ++j)
                {
                    _mem.pv_table[ply][j] = _mem.pv_table[ply + 1][j];
                }

                _mem.pv_length[ply] = _mem.pv_length[ply + 1];

                if(score >= beta)
                {
                    if(quiet)
                    {
                        if(_mem.killers[ply][0] != move)
                        {
                            _mem.killers[ply][1] = _mem.killers[ply][0];
                            _mem.killers[ply][0] = move;
                        }

                        Piece piece = _pos.board[move.from()];
                        int16_t& h = _mem.history[piece][sq64(move.to())];
                        int bonus = depth * depth;

                        if(h + bonus > 20000)
                        {
                            for(int p = 0; p < 16; ++p)
                            {
                                for(int s = 0; s < 64; ++s)
                                {
                                    _mem.history[p][s] = int16_t(_mem.history[p][s] / 2);
                                }
                            }
                        }

                        h = int16_t(h + bonus);
                    }

                    break;
                }
            }
        }
    }

    _stack_top = start;

    if(legal == 0)
    {
        return in_check ? -SCORE_MATE + ply : _draw_score(ply);
    }

    uint8_t bound = best <= original_alpha ? TranspositionTable::BOUND_UPPER :
                    best >= beta ? TranspositionTable::BOUND_LOWER : TranspositionTable::BOUND_EXACT;
    _tt.store(_pos.hash, best_move, score_to_tt(best, ply), depth, bound);
    return best;
}

}
