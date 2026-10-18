// CHESS LEGENDS: ARCANE WARS - console-side global state (lives in EWRAM).
#ifndef CL_GLOBALS_H
#define CL_GLOBALS_H

#include "cl_ai.h"
#include "cl_game.h"
#include "cl_test_interface.h"

namespace cl::gba
{

enum class SceneId : uint8_t
{
    BOOT,
    INTRO,
    TITLE,
    MAIN_MENU,
    MATCH_SETUP,
    MATCH,
    STORY_MAP,
    STORY_DIALOGUE,
    CHALLENGES,
    GAUNTLET,
    GALLERY,
    ACHIEVEMENTS,
    OPTIONS,
    CREDITS,
    TUTORIAL,
    ENDING,
    COUNT
};

enum class MatchMode : uint8_t
{
    CLASSIC,
    ARCANE,
    STORY,
    CHALLENGE,
    GAUNTLET,
    TUTORIAL
};

enum class AnimationMode : uint8_t
{
    FULL,       // board animation + cinematic battle scenes for captures
    FAST,       // shorter animations, battle scenes only for major captures
    MINIMAL,    // quick board animations only
    OFF         // instant
};

struct Settings
{
    uint8_t music_volume = 8;       // 0-10
    uint8_t sfx_volume = 8;         // 0-10
    bool music_on = true;
    bool sfx_on = true;
    AnimationMode animations = AnimationMode::FULL;
    uint8_t text_speed = 1;         // 0 slow, 1 normal, 2 fast, 3 instant
    uint8_t board_theme = 0;
    bool high_contrast = false;
    bool reduced_effects = false;
    bool show_coordinates = true;
    uint8_t preferred_difficulty = 1;
};

struct MatchSetup
{
    MatchMode mode = MatchMode::ARCANE;
    bool arcane = true;
    uint8_t human_mask = 1;          // bit 0: White is human, bit 1: Black is human
    Difficulty difficulty = Difficulty::KNIGHT;
    Personality personality = Personality::BALANCED;
    uint8_t board_theme = 0;
    uint8_t opponent_faction = 0;    // 0 = default dark army, 1.. kingdom factions
    int8_t encounter = -1;           // campaign encounter index
    int8_t challenge = -1;           // challenge index
    int8_t gauntlet_stage = -1;
    bool resume_suspended = false;
};

struct Globals
{
    Game game;
    SearchMemory search_memory;
    TTEntry tt_entries[4096];
    Settings settings;
    MatchSetup setup;
    uint32_t seed = 0x5EED1234u;
    uint32_t frame_counter = 0;
};

Globals& globals();
cl_test_interface& test_interface();

// Deterministic cosmetic random numbers (never used by the rules or the AI).
uint32_t cosmetic_random();

}

#endif
