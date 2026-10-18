// CHESS LEGENDS: ARCANE WARS - diagnostic mailbox of the test interface.
//
// The headless harness posts commands through cl_test_interface (cmd / args, answered with ack). They exist only
// for automated testing: no button combination reaches them, and normal play never depends on them.
#ifndef CL_TEST_COMMANDS_H
#define CL_TEST_COMMANDS_H

#include "bn_optional.h"

#include "cl_globals.h"

namespace cl::gba
{

// Handles a posted command; returns the scene to switch to when the command asks for one.
[[nodiscard]] bn::optional<SceneId> process_test_commands();

// A move posted with CL_TI_CMD_PLAY_MOVE (packed Move data), consumed by the match scene; 0 when none.
[[nodiscard]] uint32_t take_test_move();

}

#endif
