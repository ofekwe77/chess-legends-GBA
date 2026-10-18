// CHESS LEGENDS: ARCANE WARS - position setup, FEN input / output and consistency checks.
#include "cl_position.h"

#include "cl_psqt.h"
#include "cl_tables.h"
#include "cl_zobrist.h"

namespace cl
{

namespace
{
    // ---- small text helpers (no standard library available on the GBA build) ------------------------------
    [[nodiscard]] bool is_space(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r';
    }

    [[nodiscard]] const char* skip_spaces(const char* text)
    {
        while(*text && is_space(*text))
        {
            ++text;
        }

        return text;
    }

    // Copies the next space-separated token into buffer; returns the position after it.
    const char* next_token(const char* text, char* buffer, int capacity)
    {
        text = skip_spaces(text);
        int length = 0;

        while(*text && ! is_space(*text))
        {
            if(length < capacity - 1)
            {
                buffer[length++] = *text;
            }

            ++text;
        }

        buffer[length] = 0;
        return text;
    }

    [[nodiscard]] bool parse_int(const char* text, int& value)
    {
        if(! *text)
        {
            return false;
        }

        int result = 0;

        for(; *text; ++text)
        {
            if(*text < '0' || *text > '9')
            {
                return false;
            }

            result = result * 10 + (*text - '0');

            if(result > 100000)
            {
                return false;
            }
        }

        value = result;
        return true;
    }

    [[nodiscard]] int hex_digit(char c)
    {
        if(c >= '0' && c <= '9')
        {
            return c - '0';
        }

        if(c >= 'a' && c <= 'f')
        {
            return c - 'a' + 10;
        }

        if(c >= 'A' && c <= 'F')
        {
            return c - 'A' + 10;
        }

        return -1;
    }

    [[nodiscard]] bool starts_with(const char* text, const char* prefix)
    {
        while(*prefix)
        {
            if(*text++ != *prefix++)
            {
                return false;
            }
        }

        return true;
    }

    // Parses "A/B" with A, B in [0, max_value]; base 10 or 16.
    [[nodiscard]] bool parse_pair(const char* text, int base, int max_value, int& first, int& second)
    {
        int values[2] = { 0, 0 };
        int index = 0;
        bool digits = false;

        for(; *text; ++text)
        {
            if(*text == '/')
            {
                if(index == 1 || ! digits)
                {
                    return false;
                }

                index = 1;
                digits = false;
                continue;
            }

            int digit = base == 16 ? hex_digit(*text) : (*text >= '0' && *text <= '9' ? *text - '0' : -1);

            if(digit < 0)
            {
                return false;
            }

            values[index] = values[index] * base + digit;
            digits = true;

            if(values[index] > max_value)
            {
                return false;
            }
        }

        if(index != 1 || ! digits)
        {
            return false;
        }

        first = values[0];
        second = values[1];
        return true;
    }

    [[nodiscard]] Piece piece_from_char(char c)
    {
        switch(c)
        {
        case 'P': return W_PAWN;
        case 'N': return W_KNIGHT;
        case 'B': return W_BISHOP;
        case 'R': return W_ROOK;
        case 'Q': return W_QUEEN;
        case 'K': return W_KING;
        case 'p': return B_PAWN;
        case 'n': return B_KNIGHT;
        case 'b': return B_BISHOP;
        case 'r': return B_ROOK;
        case 'q': return B_QUEEN;
        case 'k': return B_KING;
        default: return EMPTY;
        }
    }

    constexpr char piece_chars[16] = { '.', 'P', 'N', 'B', 'R', 'Q', 'K', '?', '?', 'p', 'n', 'b', 'r', 'q', 'k', '?' };

    class TextWriter
    {
    public:
        TextWriter(char* output, int capacity) :
            _output(output),
            _capacity(capacity)
        {
        }

        void put(char c)
        {
            if(_length < _capacity - 1)
            {
                _output[_length] = c;
            }

            ++_length;
        }

        void put(const char* text)
        {
            while(*text)
            {
                put(*text++);
            }
        }

        void put_int(int value)
        {
            char digits[12];
            int count = 0;

            if(value < 0)
            {
                put('-');
                value = -value;
            }

            do
            {
                digits[count++] = char('0' + value % 10);
                value /= 10;
            }
            while(value);

            while(count)
            {
                put(digits[--count]);
            }
        }

        void put_hex2(int value)
        {
            constexpr const char* hex = "0123456789abcdef";
            put(hex[(value >> 4) & 15]);
            put(hex[value & 15]);
        }

        int finish()
        {
            if(_capacity > 0)
            {
                _output[_length < _capacity ? _length : _capacity - 1] = 0;
            }

            return _length;
        }

    private:
        char* _output;
        int _capacity;
        int _length = 0;
    };
}

bool piece_is_slider(PieceType type)
{
    return type == BISHOP || type == ROOK || type == QUEEN;
}

uint8_t castle_mask(int sq)
{
    return tables.castle_mask[sq];
}

void Position::make_null(Undo& undo)
{
    undo.hash = hash;
    undo.ep = ep;
    undo.halfmove = halfmove;
    undo.mana[0] = mana[0];
    undo.mana[1] = mana[1];

    if(ep != NO_SQUARE)
    {
        hash ^= zobrist.ep_file[file_of(ep)];
        ep = NO_SQUARE;
    }

    const Color them = opposite(side);
    halfmove = uint16_t(halfmove + 1);
    ++ply;
    side = them;
    hash ^= zobrist.side;

    if(rules.arcane)
    {
        int regenerated = mana[them] + rules.regeneration;
        _set_mana(them, regenerated > rules.max_mana ? rules.max_mana : regenerated);
    }
}

void Position::unmake_null(const Undo& undo)
{
    side = opposite(side);
    --ply;
    hash = undo.hash;
    ep = undo.ep;
    halfmove = undo.halfmove;
    mana[0] = undo.mana[0];
    mana[1] = undo.mana[1];
}

void Position::clear(const RuleSet& rule_set)
{
    for(int sq = 0; sq < 128; ++sq)
    {
        board[sq] = EMPTY;
        pindex[sq] = -1;
    }

    for(int c = 0; c < 2; ++c)
    {
        for(int i = 0; i < 16; ++i)
        {
            plist[c][i] = 0;
        }

        pcount[c] = 0;
        mana[c] = 0;
        escapes[c] = config::royal_escape_uses;
        home[c] = 0;
    }

    for(int p = 0; p < 16; ++p)
    {
        piece_count[p] = 0;
    }

    side = WHITE;
    castling = 0;
    ep = NO_SQUARE;
    halfmove = 0;
    ply = 0;
    fullmove = 1;
    rules = rule_set;
    hash = 0;
    score_mg = 0;
    score_eg = 0;
    phase = 0;
}

void Position::set_start(const RuleSet& rule_set)
{
    FenError error = set_fen(start_fen, rule_set);
    CL_ASSERT(error == FenError::NONE);
}

FenError Position::set_fen(const char* fen, const RuleSet& rule_set)
{
    clear(rule_set);

    char token[96];
    const char* cursor = next_token(fen, token, int(sizeof(token)));

    // ---- piece placement ----
    int rank = 7;
    int file = 0;
    int kings[2] = { 0, 0 };

    for(const char* c = token; *c; ++c)
    {
        if(*c == '/')
        {
            if(file != 8 || rank == 0)
            {
                return FenError::SYNTAX;
            }

            --rank;
            file = 0;
        }
        else if(*c >= '1' && *c <= '8')
        {
            file += *c - '0';

            if(file > 8)
            {
                return FenError::SYNTAX;
            }
        }
        else
        {
            Piece piece = piece_from_char(*c);

            if(piece == EMPTY || file > 7)
            {
                return FenError::SYNTAX;
            }

            Color color = color_of(piece);

            if(type_of(piece) == KING)
            {
                if(++kings[color] > 1)
                {
                    return FenError::KING_COUNT;
                }
            }

            if(pcount[color] >= 16)
            {
                return FenError::TOO_MANY_PIECES;
            }

            if(type_of(piece) == PAWN && (rank == 0 || rank == 7))
            {
                return FenError::PAWN_ON_BACK_RANK;
            }

            _put(piece, make_square(file, rank));
            ++file;
        }
    }

    if(rank != 0 || file != 8)
    {
        return FenError::SYNTAX;
    }

    if(kings[WHITE] != 1 || kings[BLACK] != 1)
    {
        return FenError::KING_COUNT;
    }

    // ---- side to move ----
    cursor = next_token(cursor, token, int(sizeof(token)));

    if(token[0] == 'w' && token[1] == 0)
    {
        side = WHITE;
    }
    else if(token[0] == 'b' && token[1] == 0)
    {
        side = BLACK;
        hash ^= zobrist.side;
    }
    else
    {
        return FenError::SYNTAX;
    }

    // ---- castling ----
    cursor = next_token(cursor, token, int(sizeof(token)));
    uint8_t rights = 0;

    if(! (token[0] == '-' && token[1] == 0))
    {
        for(const char* c = token; *c; ++c)
        {
            switch(*c)
            {
            case 'K': rights |= CASTLE_WK; break;
            case 'Q': rights |= CASTLE_WQ; break;
            case 'k': rights |= CASTLE_BK; break;
            case 'q': rights |= CASTLE_BQ; break;
            default: return FenError::SYNTAX;
            }
        }
    }

    // Drop rights that the board cannot support (king and rook must stand on their original squares).
    if(board[E1] != W_KING)
    {
        rights &= uint8_t(~(CASTLE_WK | CASTLE_WQ));
    }

    if(board[H1] != W_ROOK)
    {
        rights &= uint8_t(~CASTLE_WK);
    }

    if(board[A1] != W_ROOK)
    {
        rights &= uint8_t(~CASTLE_WQ);
    }

    if(board[E8] != B_KING)
    {
        rights &= uint8_t(~(CASTLE_BK | CASTLE_BQ));
    }

    if(board[H8] != B_ROOK)
    {
        rights &= uint8_t(~CASTLE_BK);
    }

    if(board[A8] != B_ROOK)
    {
        rights &= uint8_t(~CASTLE_BQ);
    }

    castling = rights;
    hash ^= zobrist.castling[castling];

    // ---- en passant ----
    cursor = next_token(cursor, token, int(sizeof(token)));

    if(! (token[0] == '-' && token[1] == 0))
    {
        if(token[0] < 'a' || token[0] > 'h' || token[1] < '1' || token[1] > '8' || token[2] != 0)
        {
            return FenError::SYNTAX;
        }

        int ep_sq = make_square(token[0] - 'a', token[1] - '1');
        int expected_rank = side == WHITE ? 5 : 2;
        int pusher_sq = side == WHITE ? ep_sq - 16 : ep_sq + 16;
        int origin_sq = side == WHITE ? ep_sq + 16 : ep_sq - 16;

        if(rank_of(ep_sq) != expected_rank || board[ep_sq] != EMPTY || board[origin_sq] != EMPTY ||
           board[pusher_sq] != make_piece(opposite(side), PAWN))
        {
            return FenError::BAD_EN_PASSANT;
        }

        Piece capturer = make_piece(side, PAWN);

        if((on_board(pusher_sq - 1) && board[pusher_sq - 1] == capturer) ||
           (on_board(pusher_sq + 1) && board[pusher_sq + 1] == capturer))
        {
            ep = uint8_t(ep_sq);
            hash ^= zobrist.ep_file[file_of(ep)];
        }
    }

    // ---- counters (optional) ----
    const char* after_ep = cursor;
    cursor = next_token(cursor, token, int(sizeof(token)));
    int value = 0;

    if(token[0] && parse_int(token, value))
    {
        halfmove = uint16_t(value);
        const char* after_halfmove = cursor;
        cursor = next_token(cursor, token, int(sizeof(token)));

        if(token[0] && parse_int(token, value))
        {
            fullmove = uint16_t(value < 1 ? 1 : value);
        }
        else
        {
            cursor = after_halfmove;
        }
    }
    else
    {
        cursor = after_ep;
    }

    // ---- default pawn double-step eligibility: every pawn on its second rank ----
    for(int f = 0; f < 8; ++f)
    {
        if(board[make_square(f, 1)] == W_PAWN)
        {
            home[WHITE] |= uint8_t(1 << f);
        }

        if(board[make_square(f, 6)] == B_PAWN)
        {
            home[BLACK] |= uint8_t(1 << f);
        }
    }

    // ---- Arcane fields (optional): mana=W/B escape=W/B home=hh/hh ----
    while(true)
    {
        cursor = next_token(cursor, token, int(sizeof(token)));

        if(! token[0])
        {
            break;
        }

        int first = 0;
        int second = 0;

        if(starts_with(token, "mana="))
        {
            if(! parse_pair(token + 5, 10, 15, first, second) || first > rules.max_mana || second > rules.max_mana)
            {
                return FenError::BAD_ARCANE_FIELDS;
            }

            mana[WHITE] = uint8_t(first);
            mana[BLACK] = uint8_t(second);
        }
        else if(starts_with(token, "escape="))
        {
            if(! parse_pair(token + 7, 10, 3, first, second))
            {
                return FenError::BAD_ARCANE_FIELDS;
            }

            escapes[WHITE] = uint8_t(first);
            escapes[BLACK] = uint8_t(second);
        }
        else if(starts_with(token, "home="))
        {
            if(! parse_pair(token + 5, 16, 255, first, second))
            {
                return FenError::BAD_ARCANE_FIELDS;
            }

            // A home bit only means something when an own pawn stands on that square.
            home[WHITE] &= uint8_t(first);
            home[BLACK] &= uint8_t(second);
        }
        else
        {
            return FenError::SYNTAX;
        }
    }

    for(int c = 0; c < 2; ++c)
    {
        hash ^= zobrist.mana[c][mana[c]];
        hash ^= zobrist.escapes[c][escapes[c]];

        for(int f = 0; f < 8; ++f)
        {
            if(home[c] & (1 << f))
            {
                hash ^= zobrist.home[c][f];
            }
        }
    }

    if(in_check(opposite(side)))
    {
        return FenError::OPPONENT_IN_CHECK;
    }

    return FenError::NONE;
}

int Position::write_fen(char* output, int capacity) const
{
    TextWriter writer(output, capacity);

    for(int rank = 7; rank >= 0; --rank)
    {
        int empty = 0;

        for(int file = 0; file < 8; ++file)
        {
            Piece piece = board[make_square(file, rank)];

            if(piece == EMPTY)
            {
                ++empty;
                continue;
            }

            if(empty)
            {
                writer.put(char('0' + empty));
                empty = 0;
            }

            writer.put(piece_chars[piece]);
        }

        if(empty)
        {
            writer.put(char('0' + empty));
        }

        if(rank)
        {
            writer.put('/');
        }
    }

    writer.put(side == WHITE ? " w " : " b ");

    if(castling == 0)
    {
        writer.put('-');
    }
    else
    {
        if(castling & CASTLE_WK) writer.put('K');
        if(castling & CASTLE_WQ) writer.put('Q');
        if(castling & CASTLE_BK) writer.put('k');
        if(castling & CASTLE_BQ) writer.put('q');
    }

    writer.put(' ');

    if(ep == NO_SQUARE)
    {
        writer.put('-');
    }
    else
    {
        writer.put(char('a' + file_of(ep)));
        writer.put(char('1' + rank_of(ep)));
    }

    writer.put(' ');
    writer.put_int(halfmove);
    writer.put(' ');
    writer.put_int(fullmove);

    if(rules.arcane)
    {
        writer.put(" mana=");
        writer.put_int(mana[WHITE]);
        writer.put('/');
        writer.put_int(mana[BLACK]);
        writer.put(" escape=");
        writer.put_int(escapes[WHITE]);
        writer.put('/');
        writer.put_int(escapes[BLACK]);
        writer.put(" home=");
        writer.put_hex2(home[WHITE]);
        writer.put('/');
        writer.put_hex2(home[BLACK]);
    }

    return writer.finish();
}

uint64_t Position::compute_hash() const
{
    uint64_t result = 0;

    for(int sq = 0; sq < 128; ++sq)
    {
        if(on_board(sq) && board[sq] != EMPTY)
        {
            result ^= zobrist.piece[board[sq]][sq64(sq)];
        }
    }

    if(side == BLACK)
    {
        result ^= zobrist.side;
    }

    result ^= zobrist.castling[castling];

    if(ep != NO_SQUARE)
    {
        result ^= zobrist.ep_file[file_of(ep)];
    }

    for(int c = 0; c < 2; ++c)
    {
        result ^= zobrist.mana[c][mana[c]];
        result ^= zobrist.escapes[c][escapes[c]];

        for(int f = 0; f < 8; ++f)
        {
            if(home[c] & (1 << f))
            {
                result ^= zobrist.home[c][f];
            }
        }
    }

    return result;
}

bool Position::validate() const
{
    int counts[16] = {};
    int listed[2] = { 0, 0 };
    int mg = 0;
    int eg = 0;
    int phase_sum = 0;

    for(int sq = 0; sq < 128; ++sq)
    {
        if(! on_board(sq))
        {
            continue;
        }

        Piece piece = board[sq];

        if(piece == EMPTY)
        {
            continue;
        }

        if((piece & 7) == 0 || (piece & 7) == 7)
        {
            return false;
        }

        Color c = color_of(piece);
        int index = pindex[sq];

        if(index < 0 || index >= pcount[c] || plist[c][index] != sq)
        {
            return false;
        }

        ++counts[piece];
        ++listed[c];
        int sign = c == WHITE ? 1 : -1;
        mg += sign * psqt.mg[piece][sq64(sq)];
        eg += sign * psqt.eg[piece][sq64(sq)];
        phase_sum += phase_weight[type_of(piece)];

        if(type_of(piece) == PAWN && (rank_of(sq) == 0 || rank_of(sq) == 7))
        {
            return false;
        }
    }

    for(int c = 0; c < 2; ++c)
    {
        if(listed[c] != pcount[c] || board[plist[c][0]] != make_piece(Color(c), KING))
        {
            return false;
        }

        for(int f = 0; f < 8; ++f)
        {
            int sq = make_square(f, c == WHITE ? 1 : 6);

            if((home[c] & (1 << f)) && board[sq] != make_piece(Color(c), PAWN))
            {
                return false;
            }
        }
    }

    for(int p = 0; p < 16; ++p)
    {
        if(counts[p] != piece_count[p])
        {
            return false;
        }
    }

    if(mg != score_mg || eg != score_eg || phase_sum != phase)
    {
        return false;
    }

    if(compute_hash() != hash)
    {
        return false;
    }

    if(mana[0] > rules.max_mana || mana[1] > rules.max_mana)
    {
        return false;
    }

    return true;
}

}
