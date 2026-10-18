/*
 * CHESS LEGENDS: ARCANE WARS - test interface shared by the ROM and the headless emulator harness.
 *
 * The ROM keeps one instance of this structure in EWRAM, refreshed every frame. The harness finds it through the
 * magic words, reads fields by name and posts test commands through the mailbox (cmd / ack / args). The
 * structure only mirrors state and accepts diagnostic commands; normal play never depends on it.
 *
 * C-compatible on purpose (the harness is written in C).
 */
#ifndef CL_TEST_INTERFACE_H
#define CL_TEST_INTERFACE_H

#include <stdint.h>

#define CL_TI_MAGIC0 0x45544C43u /* "CLTE" */
#define CL_TI_MAGIC1 0x46495453u /* "STIF" */

/* X-macro list of every 32-bit field (after the two magic words). */
#define CL_TI_FIELDS(X) \
    X(frame) X(scene) X(cmd) X(ack) X(arg0) X(arg1) X(arg2) X(arg3) \
    X(match_active) X(mode) X(rules_arcane) X(human_mask) X(difficulty) X(side_to_move) X(mana_white) \
    X(mana_black) X(escapes_white) X(escapes_black) X(result) X(reason) X(plies) X(halfmove) X(in_check) \
    X(hash_lo) X(hash_hi) X(last_move) X(moves_applied) X(cursor_file) X(cursor_rank) X(selected_square) \
    X(ui_state) X(ai_thinking) X(ai_nodes) X(ai_depth) X(ai_frames) X(ai_moves) X(captures) X(spells) \
    X(board_theme) X(flipped) X(menu_index) X(menu_count) X(dialogue_active) X(dialogue_line) X(animating) \
    X(music_id) X(music_playing) X(sfx_count) X(last_sfx) X(cpu_last) X(cpu_max) X(save_writes) \
    X(save_status) X(campaign_cleared) X(achievements_lo) X(achievements_hi) X(encounter) X(objective) \
    X(error_count) X(stack_free) X(ewram_free) X(fps_drops) X(battle_scenes) X(cinematic) X(unlocked_worlds) \
    X(challenge_medals) X(text_speed) X(anim_mode) X(volume_music) X(volume_sfx) X(gauntlet_stage) \
    X(tutorial_step) X(gallery_unlocked) X(promotion_menu) X(pause_menu) X(load_status) X(toast) \
    X(moves_left) X(story_step) X(suspend_present) X(objective_status) X(challenge)

#define CL_TI_DECLARE(name) uint32_t name;

struct cl_test_interface
{
    uint32_t magic0;
    uint32_t magic1;
    CL_TI_FIELDS(CL_TI_DECLARE)
};

#undef CL_TI_DECLARE

/* Mailbox commands (diagnostic only, unreachable from the game's own controls). */
enum cl_ti_command
{
    CL_TI_CMD_NONE = 0,
    CL_TI_CMD_SET_SEED = 1,          /* arg0: AI / cosmetic random seed */
    CL_TI_CMD_START_MATCH = 2,       /* arg0: mode, arg1: human mask, arg2: difficulty, arg3: theme */
    CL_TI_CMD_PLAY_MOVE = 3,         /* arg0: packed move data (must be legal) */
    CL_TI_CMD_SET_ANIMATIONS = 4,    /* arg0: animation mode */
    CL_TI_CMD_GOTO_SCENE = 5,        /* arg0: scene id */
    CL_TI_CMD_SETUP_POSITION = 6,    /* arg0: built-in test position index */
    CL_TI_CMD_UNLOCK_ALL = 7,        /* unlock every campaign / gallery item (test only) */
    CL_TI_CMD_RESET_STATS = 8,       /* reset CPU statistics */
    CL_TI_CMD_START_ENCOUNTER = 9,   /* arg0: encounter index */
    CL_TI_CMD_START_CHALLENGE = 10,  /* arg0: challenge index */
    CL_TI_CMD_WIPE_SAVE = 11         /* format the save memory (test only) */
};

#endif
