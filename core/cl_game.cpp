// CHESS LEGENDS: ARCANE WARS - match state and the authoritative action transition.
#include "cl_game.h"

namespace cl
{

void Game::compact(Position& pos, CompactPosition& output)
{
    for(int i = 0; i < 32; ++i)
    {
        int a = sq88(i * 2);
        int b = sq88(i * 2 + 1);
        output.cells[i] = uint8_t(pos.board[a] | (pos.board[b] << 4));
    }

    output.side_castling = uint8_t((pos.side == BLACK ? 0x80 : 0) | pos.castling);

    // En passant counts for position identity only when an en passant capture is actually legal.
    output.ep = NO_SQUARE;

    if(pos.ep != NO_SQUARE)
    {
        MoveList pseudo;
        generate_tactical(pos, pseudo);

        for(int i = 0; i < pseudo.count; ++i)
        {
            if(pseudo.moves[i].kind() == MK_EN_PASSANT && leaves_king_safe(pos, pseudo.moves[i]))
            {
                output.ep = pos.ep;
                break;
            }
        }
    }

    output.mana = uint8_t(pos.mana[WHITE] | (pos.mana[BLACK] << 4));
    output.escapes = uint8_t(pos.escapes[WHITE] | (pos.escapes[BLACK] << 4));
    output.home[0] = pos.home[0];
    output.home[1] = pos.home[1];
}

void Game::start(const RuleSet& rules)
{
    FenError error = setup(start_fen, rules, true);
    CL_ASSERT(error == FenError::NONE);
}

FenError Game::setup(const char* fen, const RuleSet& rules, bool apply_turn_start_regeneration)
{
    FenError error = _pos.set_fen(fen, rules);

    if(error != FenError::NONE)
    {
        return error;
    }

    if(apply_turn_start_regeneration && rules.arcane)
    {
        int value = _pos.mana[_pos.side] + rules.regeneration;
        value = value > rules.max_mana ? rules.max_mana : value;
        _pos.mana[_pos.side] = uint8_t(value);
        _pos.hash = _pos.compute_hash();
    }

    _result = GameResult::ONGOING;
    _reason = ResultReason::NONE;
    _log_count = 0;
    _log_start = 0;
    _reset_window();
    _update_result();
    return FenError::NONE;
}

void Game::_reset_window()
{
    _base = _pos;
    _window_count = 0;
    compact(_pos, _window_positions[0]);
    _window_hashes[0] = _pos.hash;
}

void Game::_push_log(Move move)
{
    if(_log_count < log_capacity)
    {
        _log[(_log_start + _log_count) % log_capacity] = move;
        ++_log_count;
    }
    else
    {
        _log[_log_start] = move;
        _log_start = (_log_start + 1) % log_capacity;
    }
}

Move Game::log_move(int index) const
{
    CL_ASSERT(index >= 0 && index < _log_count);
    return _log[(_log_start + index) % log_capacity];
}

void Game::legal_actions(MoveList& list)
{
    if(over())
    {
        list.clear();
        return;
    }

    generate_legal(_pos, list);
}

bool Game::apply(Move move, ActionInfo* info)
{
    if(over() || ! is_legal(_pos, move))
    {
        return false;
    }

    const Color us = _pos.side;
    const MoveKind kind = move.kind();
    ActionInfo details;
    details.move = move;
    details.mover = us;
    details.piece = _pos.board[move.from()];

    if(kind == MK_CASTLE_SWAP)
    {
        details.partner = _pos.board[move.to()];
    }
    else if(kind == MK_ROYAL_COMMAND)
    {
        details.partner = _pos.board[move.aux()];
    }

    if(kind == MK_EN_PASSANT)
    {
        details.captured_sq = us == WHITE ? move.to() - 16 : move.to() + 16;
        details.captured = _pos.board[details.captured_sq];
    }
    else if(move.is_capture())
    {
        details.captured_sq = move.to();
        details.captured = _pos.board[move.to()];
    }

    if(move.is_magic())
    {
        details.mana_spent = _pos.rules.cost[move.ability()];
    }

    Undo undo;
    _pos.make(move, undo);
    CL_DEBUG_ASSERT(_pos.validate());
    details.check = _pos.in_check();
    _push_log(move);

    // Irreversible actions start a new repetition window: in Classic Chess every capture or pawn move (the
    // halfmove counter reset); in Arcane Chess only captures and promotions, because magical repositioning can
    // move pawns sideways and backwards, so earlier positions can recur after a pawn move.
    bool irreversible = _pos.rules.arcane ? (move.is_capture() || kind == MK_PROMOTION) : _pos.halfmove == 0;

    if(irreversible || _window_count >= max_window)
    {
        _reset_window();
    }
    else
    {
        _window_moves[_window_count] = move;
        ++_window_count;
        compact(_pos, _window_positions[_window_count]);
        _window_hashes[_window_count] = _pos.hash;
    }

    _update_result();

    if(info)
    {
        *info = details;
    }

    return true;
}

int Game::repetition_count() const
{
    const CompactPosition& current = _window_positions[_window_count];
    int count = 0;

    // Only positions with the same side to move can match: step back two plies at a time.
    for(int i = _window_count; i >= 0; i -= 2)
    {
        if(_window_hashes[i] == _window_hashes[_window_count] && _window_positions[i] == current)
        {
            ++count;
        }
    }

    return count;
}

bool Game::can_claim_threefold() const
{
    return ! over() && repetition_count() >= config::threefold_claim_count;
}

bool Game::can_claim_fifty() const
{
    return ! over() && _pos.halfmove >= config::fifty_move_claim_plies;
}

bool Game::claim_draw()
{
    if(can_claim_threefold())
    {
        _result = GameResult::DRAW;
        _reason = ResultReason::THREEFOLD_CLAIM;
        return true;
    }

    if(can_claim_fifty())
    {
        _result = GameResult::DRAW;
        _reason = ResultReason::FIFTY_MOVE_CLAIM;
        return true;
    }

    return false;
}

void Game::resign(Color loser)
{
    if(over())
    {
        return;
    }

    _result = loser == WHITE ? GameResult::BLACK_WINS : GameResult::WHITE_WINS;
    _reason = ResultReason::RESIGNATION;
}

void Game::agree_draw()
{
    if(over())
    {
        return;
    }

    _result = GameResult::DRAW;
    _reason = ResultReason::AGREEMENT;
}

void Game::finish_objective(bool success, Color player)
{
    if(success)
    {
        _result = player == WHITE ? GameResult::WHITE_WINS : GameResult::BLACK_WINS;
        _reason = ResultReason::OBJECTIVE_COMPLETE;
    }
    else
    {
        _result = player == WHITE ? GameResult::BLACK_WINS : GameResult::WHITE_WINS;
        _reason = ResultReason::OBJECTIVE_FAILED;
    }
}

bool Game::dead_position() const
{
    // Checkmate is impossible with these material sets regardless of how pieces are repositioned: magic never
    // captures and never adds attacking power, and no static mating configuration exists for them.
    //   K vs K, K + one minor piece vs K, any number of bishops (both sides) all on squares of one color.
    const uint8_t* counts = _pos.piece_count;

    if(counts[W_PAWN] || counts[B_PAWN] || counts[W_ROOK] || counts[B_ROOK] || counts[W_QUEEN] || counts[B_QUEEN])
    {
        return false;
    }

    int knights = counts[W_KNIGHT] + counts[B_KNIGHT];
    int bishops = counts[W_BISHOP] + counts[B_BISHOP];

    if(knights + bishops <= 1)
    {
        return true;
    }

    if(knights == 0)
    {
        bool light = false;
        bool dark = false;

        for(int c = 0; c < 2; ++c)
        {
            for(int i = 0; i < _pos.pcount[c]; ++i)
            {
                int sq = _pos.plist[c][i];

                if(type_of(_pos.board[sq]) == BISHOP)
                {
                    (is_light_square(sq) ? light : dark) = true;
                }
            }
        }

        return ! (light && dark);
    }

    return false;
}

void Game::_update_result()
{
    if(over())
    {
        return;
    }

    if(! has_legal_action(_pos))
    {
        if(_pos.in_check())
        {
            _result = _pos.side == WHITE ? GameResult::BLACK_WINS : GameResult::WHITE_WINS;
            _reason = ResultReason::CHECKMATE;
        }
        else
        {
            _result = GameResult::DRAW;
            _reason = ResultReason::STALEMATE;
        }

        return;
    }

    if(dead_position())
    {
        _result = GameResult::DRAW;
        _reason = ResultReason::DEAD_POSITION;
    }
    else if(repetition_count() >= config::fivefold_automatic_count)
    {
        _result = GameResult::DRAW;
        _reason = ResultReason::FIVEFOLD_REPETITION;
    }
    else if(_pos.halfmove >= config::seventy_five_move_plies)
    {
        _result = GameResult::DRAW;
        _reason = ResultReason::SEVENTY_FIVE_MOVES;
    }
    else if(_pos.rules.arcane && _pos.ply >= config::arcane_max_plies)
    {
        _result = GameResult::DRAW;
        _reason = ResultReason::ARCANE_MOVE_LIMIT;
    }
}

bool Game::restore(const Position& base, const Move* moves, int count, GameResult result, ResultReason reason)
{
    _pos = base;
    _result = GameResult::ONGOING;
    _reason = ResultReason::NONE;
    _log_count = 0;
    _log_start = 0;
    _reset_window();

    for(int i = 0; i < count; ++i)
    {
        if(! is_legal(_pos, moves[i]))
        {
            return false;
        }

        Undo undo;
        _pos.make(moves[i], undo);
        _push_log(moves[i]);

        if(_window_count >= max_window)
        {
            return false;
        }

        _window_moves[_window_count] = moves[i];
        ++_window_count;
        compact(_pos, _window_positions[_window_count]);
        _window_hashes[_window_count] = _pos.hash;
    }

    if(result != GameResult::ONGOING)
    {
        _result = result;
        _reason = reason;
    }
    else
    {
        _update_result();
    }

    return true;
}

const char* result_reason_name(ResultReason reason)
{
    switch(reason)
    {
    case ResultReason::NONE: return "";
    case ResultReason::CHECKMATE: return "CHECKMATE";
    case ResultReason::RESIGNATION: return "RESIGNATION";
    case ResultReason::STALEMATE: return "STALEMATE";
    case ResultReason::FIVEFOLD_REPETITION: return "FIVEFOLD REPETITION";
    case ResultReason::SEVENTY_FIVE_MOVES: return "75-MOVE RULE";
    case ResultReason::DEAD_POSITION: return "DEAD POSITION";
    case ResultReason::THREEFOLD_CLAIM: return "THREEFOLD REPETITION";
    case ResultReason::FIFTY_MOVE_CLAIM: return "50-MOVE RULE";
    case ResultReason::AGREEMENT: return "AGREEMENT";
    case ResultReason::ARCANE_MOVE_LIMIT: return "MOVE LIMIT";
    case ResultReason::OBJECTIVE_COMPLETE: return "OBJECTIVE COMPLETE";
    case ResultReason::OBJECTIVE_FAILED: return "OBJECTIVE FAILED";
    default: return "?";
    }
}

}
