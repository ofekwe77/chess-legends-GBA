// CHESS LEGENDS: ARCANE WARS - action notation.
#include "cl_notation.h"

#include "cl_movegen.h"

namespace cl
{

namespace
{
    constexpr const char* ability_prefixes[ABILITIES_COUNT] = { "ss", "ps", "gw", "cs", "rc", "re" };
    constexpr char promotion_chars[7] = { 0, 0, 'n', 'b', 'r', 'q', 0 };
    constexpr char san_piece_chars[7] = { 0, 0, 'N', 'B', 'R', 'Q', 'K' };

    class Writer
    {
    public:
        Writer(char* output, int capacity) :
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

        void put_square(int sq)
        {
            put(char('a' + file_of(sq)));
            put(char('1' + rank_of(sq)));
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

    [[nodiscard]] bool text_equal(const char* a, const char* b)
    {
        while(*a && *b)
        {
            if(*a++ != *b++)
            {
                return false;
            }
        }

        return *a == *b;
    }
}

int square_name(int sq, char* output)
{
    output[0] = char('a' + file_of(sq));
    output[1] = char('1' + rank_of(sq));
    output[2] = 0;
    return 2;
}

const char* ability_name(Ability ability)
{
    switch(ability)
    {
    case ABILITY_SHADOW_STEP: return "Shadow Step";
    case ABILITY_PHASE_SHIFT: return "Phase Shift";
    case ABILITY_GHOST_WALK: return "Ghost Walk";
    case ABILITY_CASTLE_SWAP: return "Castle Swap";
    case ABILITY_ROYAL_COMMAND: return "Royal Command";
    case ABILITY_ROYAL_ESCAPE: return "Royal Escape";
    default: return "?";
    }
}

const char* ability_short_name(Ability ability)
{
    return ability < ABILITIES_COUNT ? ability_prefixes[ability] : "??";
}

const char* piece_type_name(PieceType type)
{
    switch(type)
    {
    case PAWN: return "Pawn";
    case KNIGHT: return "Knight";
    case BISHOP: return "Bishop";
    case ROOK: return "Rook";
    case QUEEN: return "Queen";
    case KING: return "King";
    default: return "";
    }
}

const char* piece_class_name(PieceType type)
{
    switch(type)
    {
    case PAWN: return "Soldier";
    case KNIGHT: return "Cavalry";
    case BISHOP: return "Mystic";
    case ROOK: return "Guardian";
    case QUEEN: return "Sorceress";
    case KING: return "Sovereign";
    default: return "";
    }
}

int move_to_coordinate(Move move, char* output, int capacity)
{
    Writer writer(output, capacity);

    if(move.is_magic())
    {
        writer.put(ability_prefixes[move.ability()]);
        writer.put(':');
    }

    writer.put_square(move.from());
    writer.put_square(move.to());

    if(move.kind() == MK_PROMOTION)
    {
        writer.put(promotion_chars[move.promotion()]);
    }
    else if(move.kind() == MK_ROYAL_COMMAND)
    {
        writer.put('@');
        writer.put_square(move.aux());
    }

    return writer.finish();
}

Move parse_coordinate(Position& pos, const char* text)
{
    MoveList list;
    generate_legal(pos, list);

    for(int i = 0; i < list.count; ++i)
    {
        char buffer[24];
        move_to_coordinate(list.moves[i], buffer, int(sizeof(buffer)));

        if(text_equal(buffer, text))
        {
            return list.moves[i];
        }
    }

    return Move();
}

int move_to_display(Position& pos, Move move, char* output, int capacity)
{
    Writer writer(output, capacity);
    const MoveKind kind = move.kind();
    const int from = move.from();
    const int to = move.to();

    if(move.is_magic())
    {
        writer.put(ability_name(move.ability()));
        writer.put(' ');

        if(kind == MK_CASTLE_SWAP)
        {
            writer.put_square(from);
            writer.put("<>");
            writer.put_square(to);
        }
        else
        {
            writer.put_square(from);
            writer.put('-');
            writer.put_square(to);
        }
    }
    else if(kind == MK_CASTLE_KING)
    {
        writer.put("O-O");
    }
    else if(kind == MK_CASTLE_QUEEN)
    {
        writer.put("O-O-O");
    }
    else
    {
        const PieceType type = type_of(pos.board[from]);

        if(type == PAWN)
        {
            if(move.is_capture())
            {
                writer.put(char('a' + file_of(from)));
                writer.put('x');
            }

            writer.put_square(to);

            if(kind == MK_PROMOTION)
            {
                writer.put('=');
                writer.put(san_piece_chars[move.promotion()]);
            }
        }
        else
        {
            writer.put(san_piece_chars[type]);

            // Disambiguation among legal normal moves of the same piece type to the same square.
            MoveList list;
            generate_legal(pos, list);
            bool ambiguous = false;
            bool same_file = false;
            bool same_rank = false;

            for(int i = 0; i < list.count; ++i)
            {
                Move other = list.moves[i];

                if(other == move || other.is_magic() || other.to() != to || other.from() == from)
                {
                    continue;
                }

                if(type_of(pos.board[other.from()]) != type)
                {
                    continue;
                }

                ambiguous = true;
                same_file |= file_of(other.from()) == file_of(from);
                same_rank |= rank_of(other.from()) == rank_of(from);
            }

            if(ambiguous)
            {
                if(! same_file)
                {
                    writer.put(char('a' + file_of(from)));
                }
                else if(! same_rank)
                {
                    writer.put(char('1' + rank_of(from)));
                }
                else
                {
                    writer.put_square(from);
                }
            }

            if(move.is_capture())
            {
                writer.put('x');
            }

            writer.put_square(to);
        }
    }

    // Check / checkmate suffix (checkmate considers magical defenses, like the rules engine).
    Undo undo;
    pos.make(move, undo);

    if(pos.in_check())
    {
        writer.put(has_legal_action(pos) ? '+' : '#');
    }

    pos.unmake(move, undo);
    return writer.finish();
}

}
