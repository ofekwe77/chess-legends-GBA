// CHESS LEGENDS: ARCANE WARS - music and sound effects (maxmod through Butano).
//
// Every request respects the persisted audio settings (music / sound on-off and volumes). The wanted track is
// remembered while music is off, so turning it back on resumes the right theme. Sound effects are rate-limited
// (the same effect at most once every few frames, a few starts per frame) so bursts of events never distort.
#ifndef CL_AUDIO_H
#define CL_AUDIO_H

#include "cl_config.h"

namespace cl::gba
{

// Same order as the gallery soundtrack (cl::music_title).
enum class Music : uint8_t
{
    TITLE,
    WORLD_MAP,
    VERDANT,
    INFERNO,
    FROZEN,
    UMBRAL,
    STORM,
    NEXUS,
    BATTLE,
    BOSS,
    VICTORY,
    DEFEAT,
    ENDING,
    COUNT
};

enum class Sound : uint8_t
{
    CURSOR,
    MENU_MOVE,
    MENU_SELECT,
    MENU_CANCEL,
    PIECE_SELECT,
    PIECE_MOVE,
    CAPTURE,
    INVALID,
    CHECK,
    CHECKMATE,
    STALEMATE,
    PROMOTION,
    SHADOW_STEP,
    PHASE_SHIFT,
    GHOST_WALK,
    CASTLE_SWAP,
    ROYAL_COMMAND,
    ROYAL_ESCAPE,
    ACHIEVEMENT,
    UNLOCK,
    BOSS_APPEAR,
    VICTORY,
    DEFEAT,
    COUNT
};

// Plays a track (looping unless asked otherwise); keeps playing if it is already the current track.
void play_music(Music music, bool loop = true);
void stop_music();

// The theme of a kingdom (0-5).
[[nodiscard]] Music world_music(int world);

void play_sound(Sound sound);

// Sound identity of each ability.
[[nodiscard]] Sound spell_sound(Ability ability);

// Applies changed audio settings at once (volumes, music on / off).
void apply_audio_settings();

// Called once per frame (sound rate limiting).
void audio_frame();

}

#endif
