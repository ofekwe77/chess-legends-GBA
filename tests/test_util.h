// Shared helpers for the host tests.
#ifndef CL_TEST_UTIL_H
#define CL_TEST_UTIL_H

#include "cl_test.h"

#include <string_view>

#include "cl_game.h"
#include "cl_notation.h"

namespace test_util
{
    inline cl::Game& fresh_game()
    {
        static cl::Game game;   // large object: keep it off the stack
        return game;
    }

    inline bool setup(cl::Game& game, const char* fen, bool arcane)
    {
        cl::RuleSet rules = arcane ? cl::RuleSet::arcane_rules() : cl::RuleSet::classic();
        return game.setup(fen, rules, false) == cl::FenError::NONE;
    }

    inline cl::Move find(cl::Game& game, const char* coordinate)
    {
        cl::Position pos = game.position();
        return cl::parse_coordinate(pos, coordinate);
    }

    inline bool is_legal(cl::Game& game, const char* coordinate)
    {
        return ! find(game, coordinate).is_null();
    }

    inline bool play(cl::Game& game, const char* coordinate, cl::ActionInfo* info = nullptr)
    {
        cl::Move move = find(game, coordinate);
        return ! move.is_null() && game.apply(move, info);
    }

    inline int count_kind(cl::Game& game, cl::MoveKind kind)
    {
        cl::MoveList list;
        game.legal_actions(list);
        int count = 0;

        for(int i = 0; i < list.count; ++i)
        {
            count += list.moves[i].kind() == kind;
        }

        return count;
    }

    inline int count_legal(cl::Game& game)
    {
        cl::MoveList list;
        game.legal_actions(list);
        return list.count;
    }
}

#endif
