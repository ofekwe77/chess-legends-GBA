// CHESS LEGENDS: ARCANE WARS - legal move lists, legality tests and perft.
#include "cl_movegen.h"

#include "cl_tables.h"

namespace cl
{

// Rarely called relative to the full generators, so it lives in ROM to keep IWRAM for the hottest code.
void generate_arcane_checks(const Position& pos, MoveBuffer& list)
{
    if(! pos.rules.arcane)
    {
        return;
    }

    const Color us = pos.side;
    const bool shadow_step = pos.can_afford(us, ABILITY_SHADOW_STEP);
    const bool phase_shift = pos.can_afford(us, ABILITY_PHASE_SHIFT);
    const bool ghost_walk = pos.can_afford(us, ABILITY_GHOST_WALK);
    const bool castle_swap = pos.can_afford(us, ABILITY_CASTLE_SWAP);
    const bool royal_command = pos.can_afford(us, ABILITY_ROYAL_COMMAND);

    if(! (shadow_step || phase_shift || ghost_walk || castle_swap || royal_command))
    {
        return;
    }

    const int king = pos.king_square(opposite(us));
    const int count = pos.pcount[us];

    for(int i = 1; i < count; ++i)
    {
        const int sq = pos.plist[us][i];
        const Piece piece = pos.board[sq];

        switch(type_of(piece))
        {

        case PAWN:
            // A sideways step keeps the rank: only pawns one rank "behind" the king can check after stepping.
            if(shadow_step && rank_of(king) == rank_of(sq) + (us == WHITE ? 1 : -1))
            {
                for(int dir = -1; dir <= 1; dir += 2)
                {
                    const int target = sq + dir;

                    if(on_board(target) && pos.board[target] == EMPTY && attacks_ignoring_blockers(piece, target, king))
                    {
                        list.add(Move::make(sq, target, MK_SHADOW_STEP));
                    }
                }
            }
            break;

        case KNIGHT:
            if(phase_shift && square_distance(sq, king) <= 3)
            {
                for(int8_t offset : king_offsets)
                {
                    const int target = sq + offset;

                    if(on_board(target) && pos.board[target] == EMPTY && attacks_ignoring_blockers(piece, target, king))
                    {
                        list.add(Move::make(sq, target, MK_PHASE_SHIFT));
                    }
                }
            }
            break;

        case BISHOP:
            // A bishop stays on its square color: it can only ever check a king standing on that color.
            if(ghost_walk && is_light_square(sq) == is_light_square(king))
            {
                for(int8_t step : bishop_offsets)
                {
                    int target = sq + step;

                    while(on_board(target) && pos.board[target] == EMPTY)
                    {
                        target += step;
                    }

                    if(! on_board(target) || color_of(pos.board[target]) != us)
                    {
                        continue;
                    }

                    target += step;

                    while(on_board(target) && pos.board[target] == EMPTY)
                    {
                        if(attacks_ignoring_blockers(piece, target, king))
                        {
                            list.add(Move::make(sq, target, MK_GHOST_WALK));
                        }

                        target += step;
                    }
                }
            }
            break;

        case ROOK:
            if(castle_swap)
            {
                for(int8_t step : rook_offsets)
                {
                    int target = sq + step;

                    for(int distance = 1; distance <= config::castle_swap_max_distance && on_board(target);
                        ++distance, target += step)
                    {
                        const Piece partner = pos.board[target];

                        if(partner == EMPTY)
                        {
                            continue;
                        }

                        const PieceType partner_type = type_of(partner);

                        if(color_of(partner) == us && (partner_type == BISHOP || partner_type == KNIGHT) &&
                           (attacks_ignoring_blockers(piece, target, king) ||
                            attacks_ignoring_blockers(partner, sq, king)))
                        {
                            list.add(Move::make(sq, target, MK_CASTLE_SWAP));
                        }

                        break;
                    }
                }
            }
            break;

        case QUEEN:
            if(royal_command)
            {
                for(int8_t offset : king_offsets)
                {
                    const int ally_sq = sq + offset;

                    if(! on_board(ally_sq))
                    {
                        continue;
                    }

                    const Piece ally = pos.board[ally_sq];

                    if(ally == EMPTY || color_of(ally) != us)
                    {
                        continue;
                    }

                    const PieceType ally_type = type_of(ally);

                    if(ally_type == KING || ally_type == QUEEN)
                    {
                        continue;
                    }

                    for(int8_t dest_offset : king_offsets)
                    {
                        const int dest = sq + dest_offset;

                        if(! on_board(dest) || pos.board[dest] != EMPTY || square_distance(dest, ally_sq) != 1)
                        {
                            continue;
                        }

                        if(ally_type == PAWN && (rank_of(dest) == 0 || rank_of(dest) == 7))
                        {
                            continue;
                        }

                        if(attacks_ignoring_blockers(ally, dest, king))
                        {
                            list.add(Move::make(ally_sq, dest, MK_ROYAL_COMMAND, false, NO_TYPE, sq));
                        }
                    }
                }
            }
            break;

        default:
            break;
        }
    }
}

void generate_legal(Position& pos, MoveList& list)
{
    MoveList pseudo;
    generate_pseudo(pos, pseudo);
    list.clear();
    list.overflow = pseudo.overflow;

    for(int i = 0; i < pseudo.count; ++i)
    {
        if(leaves_king_safe(pos, pseudo.moves[i]))
        {
            list.add(pseudo.moves[i]);
        }
    }
}

bool is_legal(Position& pos, Move move)
{
    if(move.is_null())
    {
        return false;
    }

    MoveList pseudo;
    generate_pseudo(pos, pseudo);

    return pseudo.contains(move) && leaves_king_safe(pos, move);
}

bool has_legal_action(Position& pos)
{
    MoveList pseudo;
    generate_pseudo(pos, pseudo);

    for(int i = 0; i < pseudo.count; ++i)
    {
        if(leaves_king_safe(pos, pseudo.moves[i]))
        {
            return true;
        }
    }

    return false;
}

bool gives_check(Position& pos, Move move)
{
    Undo undo;
    pos.make(move, undo);
    bool check = pos.in_check();
    pos.unmake(move, undo);
    return check;
}

uint64_t perft(Position& pos, int depth)
{
    if(depth <= 0)
    {
        return 1;
    }

    MoveList pseudo;
    generate_pseudo(pos, pseudo);
    CL_ASSERT(! pseudo.overflow);
    const Color us = pos.side;
    uint64_t nodes = 0;

    for(int i = 0; i < pseudo.count; ++i)
    {
        Move move = pseudo.moves[i];

        if(is_castling(move) && ! castling_path_safe(pos, move))
        {
            continue;
        }

        Undo undo;
        pos.make(move, undo);

        if(! pos.in_check(us))
        {
            nodes += depth == 1 ? 1 : perft(pos, depth - 1);
        }

        pos.unmake(move, undo);
    }

    return nodes;
}

}
