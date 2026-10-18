// CHESS LEGENDS: ARCANE WARS - the interactive tutorial (ten guided lessons on real positions).
//
// Every lesson is a real position and a task checked by the rules engine: selecting a square, playing a given
// action, capturing a piece type, casting an ability, reaching an amount of mana, or meeting an objective with the
// computer defending. Nothing is scripted around the rules.
#ifndef CL_TUTORIAL_H
#define CL_TUTORIAL_H

#include "cl_campaign.h"

namespace cl
{

enum class LessonGoal : uint8_t
{
    SELECT_SQUARE,  // select the piece on `from` (move the cursor there and press A)
    MOVE,           // play an action from `from` to `to`
    CAPTURE,        // capture a piece of type `value`
    CAST,           // cast the ability `value`
    REACH_MANA,     // hold at least `value` mana when the turn comes back (the computer replies)
    OBJECTIVE       // meet `objective` with the computer defending
};

struct Lesson
{
    const char* title;
    const char* pages[3];           // short instruction pages shown first (unused entries are nullptr)
    const char* task;               // shown in the side panel while playing
    const char* success;            // shown when the task is done
    const char* fen;                // nullptr: standard start position
    bool arcane;
    LessonGoal goal;
    int8_t from;                    // 0x88 squares (NO_SQUARE when unused)
    int8_t to;
    uint8_t value;                  // piece type / ability / mana
    ObjectiveSpec objective;
    Difficulty defender;
};

constexpr int lessons_count = 10;

[[nodiscard]] const Lesson& lesson(int index);

// Sets up the lesson position. Returns false if the data is invalid.
bool setup_lesson(const Lesson& lesson, Game& game);

// True if the applied action (by the learner) completes an action-type goal (MOVE, CAPTURE, CAST).
[[nodiscard]] bool lesson_action_completes(const Lesson& lesson, const ActionInfo& info);

}

#endif
