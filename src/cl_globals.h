// CHESS LEGENDS: ARCANE WARS - console-side global state (lives in EWRAM).
#ifndef CL_GLOBALS_H
#define CL_GLOBALS_H

#include "cl_ai.h"
#include "cl_game.h"
#include "cl_save.h"
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

struct MatchSetup
{
    MatchMode mode = MatchMode::ARCANE;
    bool arcane = true;
    uint8_t human_mask = 1;          // bit 0: White is human, bit 1: Black is human
    Difficulty difficulty = Difficulty::KNIGHT;
    Personality personality = Personality::BALANCED;
    uint8_t node_scale = 100;        // percent of the difficulty's node budget
    uint8_t board_theme = 0;
    uint8_t opponent_faction = 0;    // 0 = default dark army, 1.. kingdom factions
    int8_t encounter = -1;           // campaign encounter index
    int8_t challenge = -1;           // challenge index
    int8_t gauntlet_stage = -1;
    int8_t lesson = -1;              // tutorial lesson index
    bool resume_suspended = false;   // the game and scenario state were restored from a suspended match
    bool last_success = false;       // outcome of the last finished scenario match (story / challenge / gauntlet)
    bool result_ready = false;       // last_success belongs to the match just left (false after quitting it)
};

// What the story dialogue scene shows next (set by the story map, the match scene and the gauntlet).
enum class StoryStep : uint8_t
{
    PROLOGUE,           // opening narration, then the first world introduction
    WORLD_INTRO,
    BRIEFING,           // boss introduction (bosses), briefing card, intro dialogue, then the match
    VICTORY,
    DEFEAT,
    EPILOGUE            // after the final boss: epilogue, then the ending
};

struct StoryFlow
{
    StoryStep step = StoryStep::PROLOGUE;
    int8_t world = 0;
    int8_t encounter = -1;
};

struct Globals
{
    Game game;
    SearchMemory search_memory;
    TTEntry tt_entries[4096];
    Profile profile;                 // settings, progress and statistics (persisted)
    SuspendedMatch suspended;        // scratch for suspend / resume records
    MatchSetup setup;
    StoryFlow story;
    uint32_t seed = 0x5EED1234u;
    uint32_t frame_counter = 0;
    uint32_t play_frames = 0;        // frames not yet added to the play-time statistic
    uint64_t toasts = 0;             // unlocked achievements still to be announced
    bool save_failed = false;        // the last save could not be verified
    bool load_notice = false;        // the save was repaired or reset at boot (told once on the title screen)
};

Globals& globals();
cl_test_interface& test_interface();

// Deterministic cosmetic random numbers (never used by the rules or the AI).
uint32_t cosmetic_random();

[[nodiscard]] inline AnimationMode animation_mode()
{
    return AnimationMode(globals().profile.settings.animations & 3);
}

// Adds pending play time to the statistics and writes the profile. Returns false if the save failed.
bool save_profile_now();

// Unlocks achievements (and queues their announcement); returns the newly unlocked bits.
uint64_t grant_achievements(uint64_t earned);

}

#endif
