// CHESS LEGENDS: ARCANE WARS - hot position code: attack detection and make / unmake.
//
// On the console this file is compiled as ARM code and its functions run from IWRAM (see cl_platform.h).
#include "cl_position.h"

#include "cl_psqt.h"
#include "cl_tables.h"
#include "cl_zobrist.h"

namespace cl
{

namespace
{
    constexpr AttackTables build_tables()
    {
        AttackTables result{};

        for(int8_t offset : knight_offsets)
        {
            result.mask[offset + 119] |= ATK_KNIGHT;
        }

        for(int8_t offset : king_offsets)
        {
            result.mask[offset + 119] |= ATK_KING;
        }

        result.mask[15 + 119] |= ATK_WPAWN;
        result.mask[17 + 119] |= ATK_WPAWN;
        result.mask[-15 + 119] |= ATK_BPAWN;
        result.mask[-17 + 119] |= ATK_BPAWN;

        for(int8_t offset : bishop_offsets)
        {
            for(int n = 1; n < 8; ++n)
            {
                result.mask[offset * n + 119] |= ATK_BISHOP | ATK_QUEEN;
                result.step[offset * n + 119] = offset;
            }
        }

        for(int8_t offset : rook_offsets)
        {
            for(int n = 1; n < 8; ++n)
            {
                result.mask[offset * n + 119] |= ATK_ROOK | ATK_QUEEN;
                result.step[offset * n + 119] = offset;
            }
        }

        constexpr uint8_t bits[16] = {
            0, ATK_WPAWN, ATK_KNIGHT, ATK_BISHOP, ATK_ROOK, ATK_QUEEN, ATK_KING, 0,
            0, ATK_BPAWN, ATK_KNIGHT, ATK_BISHOP, ATK_ROOK, ATK_QUEEN, ATK_KING, 0
        };

        for(int p = 0; p < 16; ++p)
        {
            result.attack_bit[p] = bits[p];
        }

        for(int sq = 0; sq < 128; ++sq)
        {
            result.castle_mask[sq] = CASTLE_ALL;
        }

        result.castle_mask[E1] = uint8_t(CASTLE_ALL & ~(CASTLE_WK | CASTLE_WQ));
        result.castle_mask[H1] = uint8_t(CASTLE_ALL & ~CASTLE_WK);
        result.castle_mask[A1] = uint8_t(CASTLE_ALL & ~CASTLE_WQ);
        result.castle_mask[E8] = uint8_t(CASTLE_ALL & ~(CASTLE_BK | CASTLE_BQ));
        result.castle_mask[H8] = uint8_t(CASTLE_ALL & ~CASTLE_BK);
        result.castle_mask[A8] = uint8_t(CASTLE_ALL & ~CASTLE_BQ);
        return result;
    }

    // Slider types (bishop, rook, queen), indexed by piece type.
    constexpr bool slider_type[8] = { false, false, false, true, true, true, false, false };
}

CL_IWRAM_DATA AttackTables tables = build_tables();

// ---------------------------------------------------------------------------------------------------------------
// Piece placement helpers
// ---------------------------------------------------------------------------------------------------------------

CL_IWRAM_CODE CL_NOINLINE void Position::_raw_put(Piece piece, int sq)
{
    CL_DEBUG_ASSERT(on_board(sq) && board[sq] == EMPTY && piece != EMPTY);
    Color c = color_of(piece);
    board[sq] = piece;

    if(type_of(piece) == KING)
    {
        // The king always occupies index 0: move whatever is there to the end of the list.
        if(pcount[c])
        {
            int moved_sq = plist[c][0];
            plist[c][pcount[c]] = uint8_t(moved_sq);
            pindex[moved_sq] = int8_t(pcount[c]);
        }

        plist[c][0] = uint8_t(sq);
        pindex[sq] = 0;
    }
    else
    {
        plist[c][pcount[c]] = uint8_t(sq);
        pindex[sq] = int8_t(pcount[c]);
    }

    ++pcount[c];
    ++piece_count[piece];
}

CL_IWRAM_CODE CL_NOINLINE void Position::_raw_remove(int sq)
{
    Piece piece = board[sq];
    CL_DEBUG_ASSERT(piece != EMPTY && type_of(piece) != KING);
    Color c = color_of(piece);
    int index = pindex[sq];
    int last = --pcount[c];
    int last_sq = plist[c][last];
    plist[c][index] = uint8_t(last_sq);
    pindex[last_sq] = int8_t(index);
    board[sq] = EMPTY;
    --piece_count[piece];
}

CL_IWRAM_CODE void Position::_raw_move(int from, int to)
{
    CL_DEBUG_ASSERT(board[from] != EMPTY && board[to] == EMPTY);
    Piece piece = board[from];
    int index = pindex[from];
    board[to] = piece;
    board[from] = EMPTY;
    plist[color_of(piece)][index] = uint8_t(to);
    pindex[to] = int8_t(index);
}

CL_IWRAM_CODE CL_NOINLINE void Position::_put(Piece piece, int sq)
{
    _raw_put(piece, sq);
    int index = sq64(sq);
    hash ^= zobrist.piece[piece][index];

    if(color_of(piece) == WHITE)
    {
        score_mg += psqt.mg[piece][index];
        score_eg += psqt.eg[piece][index];
    }
    else
    {
        score_mg -= psqt.mg[piece][index];
        score_eg -= psqt.eg[piece][index];
    }

    phase += phase_weight[type_of(piece)];
}

CL_IWRAM_CODE CL_NOINLINE void Position::_remove(int sq)
{
    Piece piece = board[sq];
    int index = sq64(sq);
    hash ^= zobrist.piece[piece][index];

    if(color_of(piece) == WHITE)
    {
        score_mg -= psqt.mg[piece][index];
        score_eg -= psqt.eg[piece][index];
    }
    else
    {
        score_mg += psqt.mg[piece][index];
        score_eg += psqt.eg[piece][index];
    }

    phase -= phase_weight[type_of(piece)];
    _raw_remove(sq);
}

CL_IWRAM_CODE CL_NOINLINE void Position::_move(int from, int to)
{
    Piece piece = board[from];
    int from_index = sq64(from);
    int to_index = sq64(to);
    hash ^= zobrist.piece[piece][from_index] ^ zobrist.piece[piece][to_index];

    if(color_of(piece) == WHITE)
    {
        score_mg += psqt.mg[piece][to_index] - psqt.mg[piece][from_index];
        score_eg += psqt.eg[piece][to_index] - psqt.eg[piece][from_index];
    }
    else
    {
        score_mg -= psqt.mg[piece][to_index] - psqt.mg[piece][from_index];
        score_eg -= psqt.eg[piece][to_index] - psqt.eg[piece][from_index];
    }

    _raw_move(from, to);
}

// A pawn on its second rank keeps its double-step right only while nothing leaves or arrives on that square.
CL_IWRAM_CODE CL_NOINLINE void Position::_touch_home(int sq)
{
    int rank = rank_of(sq);

    if(rank == 1)
    {
        int bit = 1 << file_of(sq);

        if(home[WHITE] & bit)
        {
            home[WHITE] &= uint8_t(~bit);
            hash ^= zobrist.home[WHITE][file_of(sq)];
        }
    }
    else if(rank == 6)
    {
        int bit = 1 << file_of(sq);

        if(home[BLACK] & bit)
        {
            home[BLACK] &= uint8_t(~bit);
            hash ^= zobrist.home[BLACK][file_of(sq)];
        }
    }
}

CL_IWRAM_CODE CL_NOINLINE void Position::_set_mana(Color c, int value)
{
    if(value != mana[c])
    {
        hash ^= zobrist.mana[c][mana[c]] ^ zobrist.mana[c][value];
        mana[c] = uint8_t(value);
    }
}

// ---------------------------------------------------------------------------------------------------------------
// Attacks
// ---------------------------------------------------------------------------------------------------------------

CL_IWRAM_CODE bool Position::attacked(int sq, Color by) const
{
    const uint8_t* list = plist[by];
    const int count = pcount[by];

    for(int i = 0; i < count; ++i)
    {
        const int from = list[i];
        const Piece piece = board[from];
        const int index = sq - from + 119;

        if(! (tables.mask[index] & tables.attack_bit[piece]))
        {
            continue;
        }

        if(slider_type[type_of(piece)])
        {
            const int step = tables.step[index];
            int s = from + step;

            while(s != sq && board[s] == EMPTY)
            {
                s += step;
            }

            if(s != sq)
            {
                continue;
            }
        }

        return true;
    }

    return false;
}

CL_IWRAM_CODE int Position::attackers_count(int sq, Color by, int limit) const
{
    const uint8_t* list = plist[by];
    const int count = pcount[by];
    int result = 0;

    for(int i = 0; i < count; ++i)
    {
        const int from = list[i];
        const Piece piece = board[from];
        const int index = sq - from + 119;

        if(! (tables.mask[index] & tables.attack_bit[piece]))
        {
            continue;
        }

        if(slider_type[type_of(piece)])
        {
            const int step = tables.step[index];
            int s = from + step;

            while(s != sq && board[s] == EMPTY)
            {
                s += step;
            }

            if(s != sq)
            {
                continue;
            }
        }

        if(++result >= limit)
        {
            break;
        }
    }

    return result;
}

// ---------------------------------------------------------------------------------------------------------------
// Make / unmake: the single authoritative state transition
// ---------------------------------------------------------------------------------------------------------------

CL_IWRAM_CODE void Position::make(Move move, Undo& undo)
{
    undo.hash = hash;
    undo.score_mg = score_mg;
    undo.score_eg = score_eg;
    undo.phase = phase;
    undo.castling = castling;
    undo.ep = ep;
    undo.halfmove = halfmove;
    undo.mana[0] = mana[0];
    undo.mana[1] = mana[1];
    undo.escapes[0] = escapes[0];
    undo.escapes[1] = escapes[1];
    undo.home[0] = home[0];
    undo.home[1] = home[1];
    undo.captured = EMPTY;

    if(ep != NO_SQUARE)
    {
        hash ^= zobrist.ep_file[file_of(ep)];
        ep = NO_SQUARE;
    }

    const Color us = side;
    const Color them = opposite(us);
    const int from = move.from();
    const int to = move.to();
    const MoveKind kind = move.kind();
    uint8_t new_castling = castling;
    bool reset_clock = false;

    switch(kind)
    {

    case MK_QUIET:
        reset_clock = type_of(board[from]) == PAWN;
        _move(from, to);
        new_castling &= tables.castle_mask[from] & tables.castle_mask[to];
        _touch_home(from);
        _touch_home(to);
        break;

    case MK_CAPTURE:
        undo.captured = board[to];
        _remove(to);
        _move(from, to);
        reset_clock = true;
        new_castling &= tables.castle_mask[from] & tables.castle_mask[to];
        _touch_home(from);
        _touch_home(to);
        break;

    case MK_DOUBLE_PUSH:
        {
            _move(from, to);
            reset_clock = true;
            _touch_home(from);

            // En passant rights exist only when an enemy pawn stands next to the arriving pawn.
            Piece enemy_pawn = make_piece(them, PAWN);

            if((on_board(to - 1) && board[to - 1] == enemy_pawn) || (on_board(to + 1) && board[to + 1] == enemy_pawn))
            {
                ep = uint8_t((from + to) / 2);
                hash ^= zobrist.ep_file[file_of(ep)];
            }
        }
        break;

    case MK_EN_PASSANT:
        {
            int captured_sq = us == WHITE ? to - 16 : to + 16;
            undo.captured = board[captured_sq];
            _remove(captured_sq);
            _move(from, to);
            reset_clock = true;
        }
        break;

    case MK_CASTLE_KING:
        _move(from, to);
        _move(from + 3, from + 1);
        new_castling &= tables.castle_mask[from];
        break;

    case MK_CASTLE_QUEEN:
        _move(from, to);
        _move(from - 4, from - 1);
        new_castling &= tables.castle_mask[from];
        break;

    case MK_PROMOTION:
        if(move.is_capture())
        {
            undo.captured = board[to];
            _remove(to);
        }

        _remove(from);
        _put(make_piece(us, move.promotion()), to);
        reset_clock = true;
        new_castling &= tables.castle_mask[to];
        _touch_home(to);
        break;

    case MK_SHADOW_STEP:
        _move(from, to);
        reset_clock = true;
        _touch_home(from);
        _touch_home(to);
        break;

    case MK_PHASE_SHIFT:
    case MK_GHOST_WALK:
        _move(from, to);
        _touch_home(from);
        _touch_home(to);
        break;

    case MK_CASTLE_SWAP:
        {
            // Swap the rook (from) and its partner (to) through a temporary removal.
            Piece partner = board[to];
            _remove(to);
            _move(from, to);
            _put(partner, from);
            new_castling &= tables.castle_mask[from] & tables.castle_mask[to];
        }
        break;

    case MK_ROYAL_COMMAND:
        reset_clock = type_of(board[from]) == PAWN;
        _move(from, to);
        new_castling &= tables.castle_mask[from] & tables.castle_mask[to];
        _touch_home(from);
        _touch_home(to);
        break;

    case MK_ROYAL_ESCAPE:
        _move(from, to);
        hash ^= zobrist.escapes[us][escapes[us]];
        --escapes[us];
        hash ^= zobrist.escapes[us][escapes[us]];
        new_castling &= us == WHITE ? uint8_t(~(CASTLE_WK | CASTLE_WQ)) : uint8_t(~(CASTLE_BK | CASTLE_BQ));
        break;

    default:
        CL_ASSERT(false);
        break;
    }

    if(new_castling != castling)
    {
        hash ^= zobrist.castling[castling] ^ zobrist.castling[new_castling];
        castling = new_castling;
    }

    if(kind >= MK_SHADOW_STEP)
    {
        int cost = rules.cost[kind - MK_SHADOW_STEP];
        CL_DEBUG_ASSERT(mana[us] >= cost);
        _set_mana(us, mana[us] - cost);
    }

    halfmove = reset_clock ? 0 : uint16_t(halfmove + 1);

    if(us == BLACK)
    {
        ++fullmove;
    }

    ++ply;
    side = them;
    hash ^= zobrist.side;

    // Turn-start regeneration for the player who moves next (applied exactly once per turn, here only).
    if(rules.arcane)
    {
        int regenerated = mana[them] + rules.regeneration;
        _set_mana(them, regenerated > rules.max_mana ? rules.max_mana : regenerated);
    }
}

CL_IWRAM_CODE void Position::unmake(Move move, const Undo& undo)
{
    const Color us = opposite(side);
    const int from = move.from();
    const int to = move.to();
    side = us;
    --ply;

    if(us == BLACK)
    {
        --fullmove;
    }

    switch(move.kind())
    {

    case MK_QUIET:
    case MK_DOUBLE_PUSH:
    case MK_SHADOW_STEP:
    case MK_PHASE_SHIFT:
    case MK_GHOST_WALK:
    case MK_ROYAL_COMMAND:
    case MK_ROYAL_ESCAPE:
        _raw_move(to, from);
        break;

    case MK_CAPTURE:
        _raw_move(to, from);
        _raw_put(undo.captured, to);
        break;

    case MK_EN_PASSANT:
        _raw_move(to, from);
        _raw_put(undo.captured, us == WHITE ? to - 16 : to + 16);
        break;

    case MK_CASTLE_KING:
        _raw_move(to, from);
        _raw_move(from + 1, from + 3);
        break;

    case MK_CASTLE_QUEEN:
        _raw_move(to, from);
        _raw_move(from - 1, from - 4);
        break;

    case MK_PROMOTION:
        _raw_remove(to);
        _raw_put(make_piece(us, PAWN), from);

        if(undo.captured != EMPTY)
        {
            _raw_put(undo.captured, to);
        }
        break;

    case MK_CASTLE_SWAP:
        {
            Piece partner = board[from];
            _raw_remove(from);
            _raw_move(to, from);
            _raw_put(partner, to);
        }
        break;

    default:
        CL_ASSERT(false);
        break;
    }

    hash = undo.hash;
    score_mg = undo.score_mg;
    score_eg = undo.score_eg;
    phase = undo.phase;
    castling = undo.castling;
    ep = undo.ep;
    halfmove = undo.halfmove;
    mana[0] = undo.mana[0];
    mana[1] = undo.mana[1];
    escapes[0] = undo.escapes[0];
    escapes[1] = undo.escapes[1];
    home[0] = undo.home[0];
    home[1] = undo.home[1];
}

}
