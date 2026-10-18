// CHESS LEGENDS: ARCANE WARS - computer opponents: difficulty levels, personalities, opening choices.
//
// Every level and personality plays by exactly the same rules through the same move generator: they differ only
// in search budget, evaluation weights (bounded to 80-150 %), opening preferences and - for the two lowest levels
// - a controlled, seeded chance of choosing a slightly weaker but still sound move.
#ifndef CL_AI_H
#define CL_AI_H

#include "cl_game.h"
#include "cl_search.h"

namespace cl
{

enum class Difficulty : uint8_t
{
    APPRENTICE,
    KNIGHT,
    SORCERER,
    GRANDMASTER,
    COUNT
};

enum class Personality : uint8_t
{
    BALANCED,
    AGGRESSIVE,
    DEFENSIVE,
    TACTICAL,
    POSITIONAL,
    TRICKSTER,
    MASTER,
    COUNT
};

struct AiSettings
{
    Difficulty difficulty = Difficulty::KNIGHT;
    Personality personality = Personality::BALANCED;
    uint32_t seed = 1;              // per-match seed: opening variety and controlled imperfection
    int node_scale_percent = 100;   // optional per-encounter budget scaling (announced difficulty, never rules)
    uint32_t poll_mask = 255;       // search poll interval (the console polls every 8 nodes to keep animating)
};

struct AiProfile
{
    SearchLimits limits;
    EvalProfile eval;
    int imperfection_percent = 0;   // chance of choosing among near-best moves
    int imperfection_margin = 0;    // centipawns below the best quick score that are acceptable
    int claim_draw_below = -60;     // claim an available draw when the position is at least this bad
};

struct AiDecision
{
    Move move;
    int score = 0;
    int depth = 0;
    int seldepth = 0;
    uint32_t nodes = 0;
    bool from_book = false;
    bool imperfect = false;
    bool aborted = false;
};

[[nodiscard]] AiProfile make_ai_profile(const AiSettings& settings);
[[nodiscard]] const char* difficulty_name(Difficulty difficulty);
[[nodiscard]] const char* personality_name(Personality personality);

class Ai
{
public:
    Ai(SearchMemory& memory, TranspositionTable& tt) :
        _searcher(memory, tt)
    {
    }

    // Chooses the action for the side to move. standard_start: the game began from the normal start position
    // (enables the opening book while all moves are still in the game log).
    AiDecision choose(Game& game, const AiSettings& settings, bool standard_start, PollFunction poll = nullptr,
                      void* poll_user = nullptr);

    // Draw claim policy for the computer player (threefold / fifty-move claims when it is worse).
    [[nodiscard]] static bool should_claim_draw(const Game& game, const AiSettings& settings, int last_score);

    // See Searcher::set_stack_limit.
    void set_stack_limit(uintptr_t limit)
    {
        _searcher.set_stack_limit(limit);
    }

private:
    Searcher _searcher;

    Move _book_move(Game& game, const AiSettings& settings) const;
};

}

#endif
