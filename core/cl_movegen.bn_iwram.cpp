// CHESS LEGENDS: ARCANE WARS - hot move generation (ARM code in IWRAM on the console).
#include "cl_movegen.h"

#include "cl_tables.h"

namespace cl
{

namespace
{
    CL_IWRAM_CODE CL_NOINLINE void add_promotions(MoveBuffer& list, int from, int to, bool capture, bool queen_only)
    {
        list.add(Move::make(from, to, MK_PROMOTION, capture, QUEEN));

        if(! queen_only)
        {
            list.add(Move::make(from, to, MK_PROMOTION, capture, ROOK));
            list.add(Move::make(from, to, MK_PROMOTION, capture, BISHOP));
            list.add(Move::make(from, to, MK_PROMOTION, capture, KNIGHT));
        }
    }

    CL_IWRAM_CODE CL_NOINLINE void generate_pawn(const Position& pos, int sq, MoveBuffer& list, bool tactical_only)
    {
        const Color us = pos.side;
        const int forward = us == WHITE ? 16 : -16;
        const int promotion_rank = us == WHITE ? 7 : 0;
        const int to = sq + forward;

        if(on_board(to) && pos.board[to] == EMPTY)
        {
            if(rank_of(to) == promotion_rank)
            {
                add_promotions(list, sq, to, false, tactical_only);
            }
            else if(! tactical_only)
            {
                list.add(Move::make(sq, to, MK_QUIET));

                if(relative_rank(us, sq) == 1 && (pos.home[us] & (1 << file_of(sq))))
                {
                    int to2 = to + forward;

                    if(pos.board[to2] == EMPTY)
                    {
                        list.add(Move::make(sq, to2, MK_DOUBLE_PUSH));
                    }
                }
            }
        }

        for(int side_step = -1; side_step <= 1; side_step += 2)
        {
            int target = to + side_step;

            if(! on_board(target))
            {
                continue;
            }

            Piece victim = pos.board[target];

            if(victim != EMPTY)
            {
                if(color_of(victim) != us && type_of(victim) != KING)
                {
                    if(rank_of(target) == promotion_rank)
                    {
                        add_promotions(list, sq, target, true, tactical_only);
                    }
                    else
                    {
                        list.add(Move::make(sq, target, MK_CAPTURE, true));
                    }
                }
            }
            else if(target == pos.ep)
            {
                list.add(Move::make(sq, target, MK_EN_PASSANT, true));
            }
        }
    }

    CL_IWRAM_CODE CL_NOINLINE void generate_steps(const Position& pos, int sq, const int8_t* offsets, MoveBuffer& list,
                                      bool tactical_only)
    {
        const Color us = pos.side;

        for(int i = 0; i < 8; ++i)
        {
            int target = sq + offsets[i];

            if(! on_board(target))
            {
                continue;
            }

            Piece victim = pos.board[target];

            if(victim == EMPTY)
            {
                if(! tactical_only)
                {
                    list.add(Move::make(sq, target, MK_QUIET));
                }
            }
            else if(color_of(victim) != us && type_of(victim) != KING)
            {
                list.add(Move::make(sq, target, MK_CAPTURE, true));
            }
        }
    }

    CL_IWRAM_CODE CL_NOINLINE void generate_slides(const Position& pos, int sq, const int8_t* offsets, int count,
                                       MoveBuffer& list, bool tactical_only)
    {
        const Color us = pos.side;

        for(int i = 0; i < count; ++i)
        {
            int step = offsets[i];
            int target = sq + step;

            while(on_board(target))
            {
                Piece victim = pos.board[target];

                if(victim == EMPTY)
                {
                    if(! tactical_only)
                    {
                        list.add(Move::make(sq, target, MK_QUIET));
                    }
                }
                else
                {
                    if(color_of(victim) != us && type_of(victim) != KING)
                    {
                        list.add(Move::make(sq, target, MK_CAPTURE, true));
                    }

                    break;
                }

                target += step;
            }
        }
    }

    // Castling is generated from rights and empty squares only; the attack conditions (king not in check, king
    // not passing through an attacked square) are verified by castling_path_safe() when the move is actually
    // tried, and the destination by the common post-move king safety test. Searches try castling far less
    // often than they generate it, so this saves several attack scans per node.
    CL_IWRAM_CODE CL_NOINLINE void generate_castling(const Position& pos, MoveBuffer& list)
    {
        const Color us = pos.side;
        const int king_sq = us == WHITE ? E1 : E8;
        const uint8_t king_side = us == WHITE ? CASTLE_WK : CASTLE_BK;
        const uint8_t queen_side = us == WHITE ? CASTLE_WQ : CASTLE_BQ;

        if(! (pos.castling & (king_side | queen_side)) || pos.board[king_sq] != make_piece(us, KING))
        {
            return;
        }

        const Piece rook = make_piece(us, ROOK);

        if((pos.castling & king_side) && pos.board[king_sq + 1] == EMPTY && pos.board[king_sq + 2] == EMPTY &&
           pos.board[king_sq + 3] == rook)
        {
            list.add(Move::make(king_sq, king_sq + 2, MK_CASTLE_KING));
        }

        if((pos.castling & queen_side) && pos.board[king_sq - 1] == EMPTY && pos.board[king_sq - 2] == EMPTY &&
           pos.board[king_sq - 3] == EMPTY && pos.board[king_sq - 4] == rook)
        {
            list.add(Move::make(king_sq, king_sq - 2, MK_CASTLE_QUEEN));
        }
    }

    CL_IWRAM_CODE void generate_moves(const Position& pos, MoveBuffer& list, bool tactical_only)
    {
        const Color us = pos.side;
        const int count = pos.pcount[us];

        for(int i = 0; i < count; ++i)
        {
            int sq = pos.plist[us][i];

            switch(type_of(pos.board[sq]))
            {
            case PAWN:
                generate_pawn(pos, sq, list, tactical_only);
                break;

            case KNIGHT:
                generate_steps(pos, sq, knight_offsets, list, tactical_only);
                break;

            case BISHOP:
                generate_slides(pos, sq, bishop_offsets, 4, list, tactical_only);
                break;

            case ROOK:
                generate_slides(pos, sq, rook_offsets, 4, list, tactical_only);
                break;

            case QUEEN:
                generate_slides(pos, sq, bishop_offsets, 4, list, tactical_only);
                generate_slides(pos, sq, rook_offsets, 4, list, tactical_only);
                break;

            case KING:
                generate_steps(pos, sq, king_offsets, list, tactical_only);
                break;

            default:
                CL_ASSERT(false);
                break;
            }
        }

        if(! tactical_only)
        {
            generate_castling(pos, list);
        }
    }
}

CL_IWRAM_CODE void generate_normal(const Position& pos, MoveBuffer& list)
{
    generate_moves(pos, list, false);
}

CL_IWRAM_CODE void generate_tactical(const Position& pos, MoveBuffer& list)
{
    generate_moves(pos, list, true);
}

CL_IWRAM_CODE void generate_arcane(const Position& pos, MoveBuffer& list)
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
    const bool royal_escape = pos.can_afford(us, ABILITY_ROYAL_ESCAPE) && pos.escapes[us] > 0;

    if(! (shadow_step || phase_shift || ghost_walk || castle_swap || royal_command || royal_escape))
    {
        return;
    }

    const int count = pos.pcount[us];

    for(int i = 0; i < count; ++i)
    {
        const int sq = pos.plist[us][i];

        switch(type_of(pos.board[sq]))
        {

        case PAWN:
            // SHADOW STEP: one square left or right onto an empty square, never a capture.
            if(shadow_step)
            {
                for(int dir = -1; dir <= 1; dir += 2)
                {
                    int target = sq + dir;

                    if(on_board(target) && pos.board[target] == EMPTY)
                    {
                        list.add(Move::make(sq, target, MK_SHADOW_STEP));
                    }
                }
            }
            break;

        case KNIGHT:
            // PHASE SHIFT: teleport to any of the eight adjacent squares if it is empty.
            if(phase_shift)
            {
                for(int8_t offset : king_offsets)
                {
                    int target = sq + offset;

                    if(on_board(target) && pos.board[target] == EMPTY)
                    {
                        list.add(Move::make(sq, target, MK_PHASE_SHIFT));
                    }
                }
            }
            break;

        case BISHOP:
            // GHOST WALK: continue along a diagonal through exactly one friendly piece; never through an enemy,
            // never through a second piece, always onto an empty square.
            if(ghost_walk)
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
                        list.add(Move::make(sq, target, MK_GHOST_WALK));
                        target += step;
                    }
                }
            }
            break;

        case ROOK:
            // CASTLE SWAP: exchange places with a friendly bishop or knight on the same rank / file, at most three
            // squares away, with every square in between empty.
            if(castle_swap)
            {
                for(int8_t step : rook_offsets)
                {
                    int target = sq + step;

                    for(int distance = 1; distance <= config::castle_swap_max_distance && on_board(target);
                        ++distance, target += step)
                    {
                        Piece partner = pos.board[target];

                        if(partner == EMPTY)
                        {
                            continue;
                        }

                        PieceType partner_type = type_of(partner);

                        if(color_of(partner) == us && (partner_type == BISHOP || partner_type == KNIGHT))
                        {
                            list.add(Move::make(sq, target, MK_CASTLE_SWAP));
                        }

                        break;
                    }
                }
            }
            break;

        case QUEEN:
            // ROYAL COMMAND: an adjacent friendly pawn / knight / bishop / rook moves one king step to an empty
            // square that is also adjacent to the queen. Pawns may never be placed on the first or last rank.
            if(royal_command)
            {
                for(int8_t offset : king_offsets)
                {
                    int ally_sq = sq + offset;

                    if(! on_board(ally_sq))
                    {
                        continue;
                    }

                    Piece ally = pos.board[ally_sq];

                    if(ally == EMPTY || color_of(ally) != us)
                    {
                        continue;
                    }

                    PieceType ally_type = type_of(ally);

                    if(ally_type == KING || ally_type == QUEEN)
                    {
                        continue;
                    }

                    for(int8_t dest_offset : king_offsets)
                    {
                        int dest = sq + dest_offset;

                        if(! on_board(dest) || pos.board[dest] != EMPTY || square_distance(dest, ally_sq) != 1)
                        {
                            continue;
                        }

                        if(ally_type == PAWN && (rank_of(dest) == 0 || rank_of(dest) == 7))
                        {
                            continue;
                        }

                        list.add(Move::make(ally_sq, dest, MK_ROYAL_COMMAND, false, NO_TYPE, sq));
                    }
                }
            }
            break;

        case KING:
            // ROYAL ESCAPE: only while in check, leap exactly two squares in any of the eight directions onto an
            // empty square (the landing square safety is verified by the common legality test).
            if(royal_escape && pos.attacked(sq, opposite(us)))
            {
                for(int8_t offset : king_offsets)
                {
                    int target = sq + 2 * offset;

                    if(on_board(target) && pos.board[target] == EMPTY)
                    {
                        list.add(Move::make(sq, target, MK_ROYAL_ESCAPE));
                    }
                }
            }
            break;

        default:
            CL_ASSERT(false);
            break;
        }
    }
}

CL_IWRAM_CODE void generate_pseudo(const Position& pos, MoveBuffer& list)
{
    generate_moves(pos, list, false);
    generate_arcane(pos, list);
}

CL_IWRAM_CODE bool castling_path_safe(const Position& pos, Move move)
{
    const Color them = opposite(pos.side);
    const int king_sq = move.from();
    const int passed_sq = move.kind() == MK_CASTLE_KING ? king_sq + 1 : king_sq - 1;
    return ! pos.attacked(king_sq, them) && ! pos.attacked(passed_sq, them);
}

CL_IWRAM_CODE bool leaves_king_safe(Position& pos, Move move)
{
    const Color us = pos.side;

    if(is_castling(move) && ! castling_path_safe(pos, move))
    {
        return false;
    }

    Undo undo;
    pos.make(move, undo);
    bool safe = ! pos.in_check(us);
    pos.unmake(move, undo);
    return safe;
}

}
