// CHESS LEGENDS: ARCANE WARS - exact puzzle solvers.
#include "cl_solver.h"

#include "cl_campaign.h"
#include "cl_movegen.h"

namespace cl
{

namespace
{
    // Legal actions of the side to move, checks first (they decide most mating searches).
    int legal_ordered(Position& pos, Move* out, bool checks_only)
    {
        MoveList list;
        generate_legal(pos, list);
        int count = 0;

        for(int pass = 0; pass < 2; ++pass)
        {
            for(int i = 0; i < list.count; ++i)
            {
                const bool check = gives_check(pos, list.moves[i]);

                if((pass == 0) == check)
                {
                    if(! check && checks_only)
                    {
                        continue;
                    }

                    out[count++] = list.moves[i];
                }
            }
        }

        return count;
    }

    enum Query : int
    {
        QUERY_MATE = 1,
        QUERY_PROMOTION = 2,
        QUERY_MATERIAL = 3
    };

    // The rule set (ability masks, mana cap) is not part of the position hash, so it is folded into the key: one cache
    // may serve queries under different masks.
    [[nodiscard]] uint64_t cache_key(const Position& pos, int moves, Query query)
    {
        const uint64_t rules = uint64_t(pos.rules.ability_mask[WHITE]) | (uint64_t(pos.rules.ability_mask[BLACK]) << 8) |
                               (uint64_t(pos.rules.max_mana) << 16) | (uint64_t(pos.rules.arcane) << 24);
        return (pos.hash ^ (0x9E3779B97F4A7C15ull * uint64_t(moves * 4 + int(query))) ^
                (0xD1B54A32D192ED03ull * (rules + 1))) | 1u;
    }

    bool cache_probe(const SolverStats& stats, uint64_t key, bool& value)
    {
        if(! stats.cache)
        {
            return false;
        }

        const uint32_t index = uint32_t(key) & stats.cache->mask;

        if(stats.cache->keys[index] != key)
        {
            return false;
        }

        value = stats.cache->values[index] != 0;
        return true;
    }

    // True at a root whose actions are restricted to stats.root_only.
    bool restricted(const SolverStats& stats, const Move* first_move)
    {
        return first_move && ! stats.root_only.is_null();
    }

    bool skipped(const SolverStats& stats, const Move* first_move, Move move)
    {
        return restricted(stats, first_move) && move != stats.root_only;
    }

    void cache_store(SolverStats& stats, uint64_t key, bool value, const Move* first_move)
    {
        if(stats.cache && ! stats.aborted && ! restricted(stats, first_move))
        {
            const uint32_t index = uint32_t(key) & stats.cache->mask;
            stats.cache->keys[index] = key;
            stats.cache->values[index] = value;
        }
    }

    bool defender_mated_within(Position& pos, int moves, SolverStats& stats);

    bool attacker_mates(Position& pos, int moves, SolverStats& stats, Move* first_move)
    {
        if(++stats.nodes > stats.node_limit)
        {
            stats.aborted = true;
            return false;
        }

        const uint64_t key = cache_key(pos, moves, QUERY_MATE);
        bool cached;

        if(! first_move && cache_probe(stats, key, cached))
        {
            return cached;
        }

        Move actions[MAX_MOVES];
        // A mating move is always a check: with one move left only checks are examined.
        const int count = legal_ordered(pos, actions, moves == 1);

        for(int i = 0; i < count; ++i)
        {
            if(skipped(stats, first_move, actions[i]))
            {
                continue;
            }

            Undo undo;
            pos.make(actions[i], undo);
            bool mates;

            if(pos.in_check() && ! has_legal_action(pos))
            {
                mates = true;
            }
            else
            {
                mates = moves > 1 && defender_mated_within(pos, moves - 1, stats);
            }

            pos.unmake(actions[i], undo);

            if(mates)
            {
                if(first_move)
                {
                    *first_move = actions[i];
                }

                cache_store(stats, key, true, first_move);
                return true;
            }

            if(stats.aborted)
            {
                return false;
            }
        }

        cache_store(stats, key, false, first_move);
        return false;
    }

    bool defender_mated_within(Position& pos, int moves, SolverStats& stats)
    {
        if(++stats.nodes > stats.node_limit)
        {
            stats.aborted = true;
            return false;
        }

        Move actions[MAX_MOVES];
        const int count = legal_ordered(pos, actions, false);

        if(count == 0)
        {
            return pos.in_check();   // stalemate is not a mate
        }

        for(int i = 0; i < count; ++i)
        {
            Undo undo;
            pos.make(actions[i], undo);
            const bool lost = attacker_mates(pos, moves, stats, nullptr);
            pos.unmake(actions[i], undo);

            if(! lost)
            {
                return false;
            }
        }

        return true;
    }

    int material_search(Position& pos, int plies, int alpha, int beta, Color root, SolverStats& stats, Move* best)
    {
        if(++stats.nodes > stats.node_limit)
        {
            stats.aborted = true;
            return 0;
        }

        const bool maximizing = pos.side == root;

        if(plies == 0)
        {
            return material_balance(pos, root);
        }

        Move actions[MAX_MOVES];
        const int count = legal_ordered(pos, actions, false);

        if(count == 0)
        {
            // Mate ends everything: a mated root loses everything, a mated opponent loses everything.
            if(pos.in_check())
            {
                return maximizing ? -100000 : 100000;
            }

            return material_balance(pos, root);
        }

        int value = maximizing ? -1000000 : 1000000;

        for(int i = 0; i < count && ! stats.aborted; ++i)
        {
            Undo undo;
            pos.make(actions[i], undo);
            int score = material_search(pos, plies - 1, alpha, beta, root, stats, nullptr);
            pos.unmake(actions[i], undo);

            if(maximizing)
            {
                if(score > value)
                {
                    value = score;

                    if(best)
                    {
                        *best = actions[i];
                    }
                }

                alpha = value > alpha ? value : alpha;
            }
            else
            {
                value = score < value ? score : value;
                beta = value < beta ? value : beta;
            }

            if(alpha >= beta)
            {
                break;
            }
        }

        return value;
    }

    bool survives(Position& pos, int moves, Color defender, SolverStats& stats, Move* first_move)
    {
        if(++stats.nodes > stats.node_limit)
        {
            stats.aborted = true;
            return false;
        }

        Move actions[MAX_MOVES];
        const int count = legal_ordered(pos, actions, false);

        if(pos.side == defender)
        {
            if(count == 0)
            {
                return ! pos.in_check();
            }

            if(moves == 0)
            {
                return true;
            }

            for(int i = 0; i < count; ++i)
            {
                if(skipped(stats, first_move, actions[i]))
                {
                    continue;
                }

                Undo undo;
                pos.make(actions[i], undo);
                const bool ok = survives(pos, moves - 1, defender, stats, nullptr);
                pos.unmake(actions[i], undo);

                if(ok)
                {
                    if(first_move)
                    {
                        *first_move = actions[i];
                    }

                    return true;
                }

                if(stats.aborted)
                {
                    return false;
                }
            }

            return false;
        }

        // Attacker to move: the defender survives only if it survives every reply.
        if(count == 0)
        {
            return true;
        }

        for(int i = 0; i < count; ++i)
        {
            Undo undo;
            pos.make(actions[i], undo);
            const bool ok = survives(pos, moves, defender, stats, nullptr);
            pos.unmake(actions[i], undo);

            if(! ok)
            {
                return false;
            }
        }

        return true;
    }

    bool defender_concedes_lead(Position& pos, int moves, int target, Color root, SolverStats& stats);

    // Attacker (root) to move: can it reach material_balance >= target at one of its turns within `moves` moves?
    bool attacker_leads(Position& pos, int moves, int target, Color root, SolverStats& stats, Move* first_move)
    {
        if(++stats.nodes > stats.node_limit)
        {
            stats.aborted = true;
            return false;
        }

        if(moves <= 0)
        {
            return false;
        }

        const uint64_t key = cache_key(pos, moves, QUERY_MATERIAL) ^ (0xA24BAED4963EE407ull * uint64_t(target + 65536));
        bool cached;

        if(! first_move && cache_probe(stats, key, cached))
        {
            return cached;
        }

        Move actions[MAX_MOVES];
        const int count = legal_ordered(pos, actions, false);

        for(int i = 0; i < count; ++i)
        {
            if(skipped(stats, first_move, actions[i]))
            {
                continue;
            }

            Undo undo;
            pos.make(actions[i], undo);
            bool success;

            if(! has_legal_action(pos))
            {
                success = pos.in_check();   // checkmate wins; stalemate is a draw (objective failed)
            }
            else
            {
                success = defender_concedes_lead(pos, moves - 1, target, root, stats);
            }

            pos.unmake(actions[i], undo);

            if(stats.aborted)
            {
                return false;
            }

            if(success)
            {
                if(first_move)
                {
                    *first_move = actions[i];
                }

                cache_store(stats, key, true, first_move);
                return true;
            }
        }

        cache_store(stats, key, false, first_move);
        return false;
    }

    // Defender to move (it has at least one action): true if every reply leaves the attacker leading by the target at
    // its turn, or still able to force it with the moves left.
    bool defender_concedes_lead(Position& pos, int moves, int target, Color root, SolverStats& stats)
    {
        if(++stats.nodes > stats.node_limit)
        {
            stats.aborted = true;
            return false;
        }

        Move actions[MAX_MOVES];
        const int count = legal_ordered(pos, actions, false);

        for(int i = 0; i < count; ++i)
        {
            Undo undo;
            pos.make(actions[i], undo);
            bool conceded = false;

            // A reply that mates or stalemates the attacker ends the game before its turn: never a success.
            if(has_legal_action(pos))
            {
                conceded = material_balance(pos, root) >= target ||
                           attacker_leads(pos, moves, target, root, stats, nullptr);
            }

            pos.unmake(actions[i], undo);

            if(! conceded || stats.aborted)
            {
                return false;
            }
        }

        return true;
    }

    // Fewest of its own moves any pawn of `side` needs to reach the last rank. Every action advances a pawn by at
    // most one rank (Royal Command moves one king step and never onto the last rank), except the double push.
    int promotion_distance(const Position& pos, Color side)
    {
        int best = 99;

        for(int i = 1; i < pos.pcount[side]; ++i)
        {
            const int sq = pos.plist[side][i];

            if(type_of(pos.board[sq]) == PAWN)
            {
                const int rank = relative_rank(side, sq);
                const int distance = rank == 1 ? 5 : 7 - rank;
                best = distance < best ? distance : best;
            }
        }

        return best;
    }

    bool defender_stops_promotion(Position& pos, int moves, SolverStats& stats);

    bool attacker_promotes(Position& pos, int moves, SolverStats& stats, Move* first_move)
    {
        if(++stats.nodes > stats.node_limit)
        {
            stats.aborted = true;
            return false;
        }

        if(moves <= 0 || promotion_distance(pos, pos.side) > moves)
        {
            return false;
        }

        const uint64_t key = cache_key(pos, moves, QUERY_PROMOTION);
        bool cached;

        if(! first_move && cache_probe(stats, key, cached))
        {
            return cached;
        }

        MoveList list;
        generate_legal(pos, list);

        // Promotions end the search at once; otherwise pawn actions first, since they decide most races.
        for(int i = 0; i < list.count; ++i)
        {
            if(list.moves[i].kind() == MK_PROMOTION && ! skipped(stats, first_move, list.moves[i]))
            {
                if(first_move)
                {
                    *first_move = list.moves[i];
                }

                cache_store(stats, key, true, first_move);
                return true;
            }
        }

        if(moves == 1)
        {
            cache_store(stats, key, false, first_move);
            return false;
        }

        for(int pass = 0; pass < 2; ++pass)
        {
            for(int i = 0; i < list.count; ++i)
            {
                const Move move = list.moves[i];
                const bool pawn_action = type_of(pos.board[move.from()]) == PAWN;

                if(pawn_action != (pass == 0) || skipped(stats, first_move, move))
                {
                    continue;
                }

                Undo undo;
                pos.make(move, undo);
                const bool promotes = defender_stops_promotion(pos, moves - 1, stats) == false;
                pos.unmake(move, undo);

                if(stats.aborted)
                {
                    return false;
                }

                if(promotes)
                {
                    if(first_move)
                    {
                        *first_move = move;
                    }

                    cache_store(stats, key, true, first_move);
                    return true;
                }
            }
        }

        cache_store(stats, key, false, first_move);
        return false;
    }

    // True if the side to move (the defender) has an action after which the attacker cannot force a promotion within
    // `moves`. A defender without actions has ended the game (mate or stalemate): no promotion follows.
    bool defender_stops_promotion(Position& pos, int moves, SolverStats& stats)
    {
        if(++stats.nodes > stats.node_limit)
        {
            stats.aborted = true;
            return true;
        }

        MoveList list;
        generate_legal(pos, list);

        if(list.count == 0)
        {
            return true;
        }

        for(int i = 0; i < list.count; ++i)
        {
            Undo undo;
            pos.make(list.moves[i], undo);
            const bool promotes = attacker_promotes(pos, moves, stats, nullptr);
            pos.unmake(list.moves[i], undo);

            if(! promotes || stats.aborted)
            {
                return true;
            }
        }

        return false;
    }
}

bool forced_mate(Position& pos, int moves, SolverStats& stats, Move* first_move)
{
    return attacker_mates(pos, moves, stats, first_move);
}

int forced_material_gain(Position& pos, int moves, SolverStats& stats, Move* first_move)
{
    const int start = material_balance(pos, pos.side);
    const int final_balance = material_search(pos, moves * 2, -1000000, 1000000, pos.side, stats, first_move);
    return final_balance - start;
}

bool can_survive(Position& pos, int moves, SolverStats& stats, Move* first_move)
{
    return survives(pos, moves, pos.side, stats, first_move);
}

bool forced_promotion(Position& pos, int moves, SolverStats& stats, Move* first_move)
{
    return attacker_promotes(pos, moves, stats, first_move);
}

bool forced_material_lead(Position& pos, int moves, int gain, SolverStats& stats, Move* first_move)
{
    const Color root = pos.side;
    return attacker_leads(pos, moves, material_balance(pos, root) + gain, root, stats, first_move);
}

}
