// CHESS LEGENDS: ARCANE WARS - runs the AI on the console without freezing the interface.
#ifndef CL_AI_RUNNER_H
#define CL_AI_RUNNER_H

#include "cl_ai.h"

namespace cl::gba
{

// Called about once per frame while the computer thinks. It must update the presentation and finish with
// bn::core::update(). Returning true cancels the search (for example when the player opens the pause menu).
using ThinkFrameFunction = bool (*)(void* user);

struct ThinkReport
{
    AiDecision decision;
    int frames = 0;          // frames rendered while thinking
    bool cancelled = false;
};

// Chooses an action for the side to move of `game`. The search runs on a dedicated EWRAM stack and yields to
// frame_function whenever most of a frame's time has been used.
ThinkReport ai_think(Game& game, const AiSettings& settings, bool standard_start, ThinkFrameFunction frame_function,
                     void* user);

}

#endif
