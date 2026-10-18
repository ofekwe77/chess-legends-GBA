// CHESS LEGENDS: ARCANE WARS - music and sound effects.
#include "cl_audio.h"

#include "bn_music.h"
#include "bn_music_item.h"
#include "bn_sound_item.h"

#include "bn_music_items.h"
#include "bn_sound_items.h"

#include "cl_globals.h"

namespace cl::gba
{

namespace
{
    constexpr int music_count = int(Music::COUNT);
    constexpr int sound_count = int(Sound::COUNT);

    const bn::music_item& music_item(Music music)
    {
        switch(music)
        {
        case Music::WORLD_MAP: return bn::music_items::world_map;
        case Music::VERDANT: return bn::music_items::verdant;
        case Music::INFERNO: return bn::music_items::inferno;
        case Music::FROZEN: return bn::music_items::frozen;
        case Music::UMBRAL: return bn::music_items::umbral;
        case Music::STORM: return bn::music_items::storm;
        case Music::NEXUS: return bn::music_items::nexus;
        case Music::BATTLE: return bn::music_items::battle;
        case Music::BOSS: return bn::music_items::boss;
        case Music::VICTORY: return bn::music_items::victory;
        case Music::DEFEAT: return bn::music_items::defeat;
        case Music::ENDING: return bn::music_items::ending;
        default: return bn::music_items::title;
        }
    }

    const bn::sound_item& sound_item(Sound sound)
    {
        switch(sound)
        {
        case Sound::MENU_MOVE: return bn::sound_items::sfx_menu_move;
        case Sound::MENU_SELECT: return bn::sound_items::sfx_menu_select;
        case Sound::MENU_CANCEL: return bn::sound_items::sfx_menu_cancel;
        case Sound::PIECE_SELECT: return bn::sound_items::sfx_piece_select;
        case Sound::PIECE_MOVE: return bn::sound_items::sfx_piece_move;
        case Sound::CAPTURE: return bn::sound_items::sfx_capture;
        case Sound::INVALID: return bn::sound_items::sfx_invalid;
        case Sound::CHECK: return bn::sound_items::sfx_check;
        case Sound::CHECKMATE: return bn::sound_items::sfx_checkmate;
        case Sound::STALEMATE: return bn::sound_items::sfx_stalemate;
        case Sound::PROMOTION: return bn::sound_items::sfx_promotion;
        case Sound::SHADOW_STEP: return bn::sound_items::sfx_shadow_step;
        case Sound::PHASE_SHIFT: return bn::sound_items::sfx_phase_shift;
        case Sound::GHOST_WALK: return bn::sound_items::sfx_ghost_walk;
        case Sound::CASTLE_SWAP: return bn::sound_items::sfx_castle_swap;
        case Sound::ROYAL_COMMAND: return bn::sound_items::sfx_royal_command;
        case Sound::ROYAL_ESCAPE: return bn::sound_items::sfx_royal_escape;
        case Sound::ACHIEVEMENT: return bn::sound_items::sfx_achievement;
        case Sound::UNLOCK: return bn::sound_items::sfx_unlock;
        case Sound::BOSS_APPEAR: return bn::sound_items::sfx_boss_appear;
        case Sound::VICTORY: return bn::sound_items::sfx_victory;
        case Sound::DEFEAT: return bn::sound_items::sfx_defeat;
        default: return bn::sound_items::sfx_cursor;
        }
    }

    int current_music = -1;         // track the game wants (also while music is off)
    bool current_loop = true;
    bool music_started = false;     // the wanted track was actually started
    uint32_t frame = 0;
    uint32_t last_played[sound_count] = {};
    int starts_this_frame = 0;

    bn::fixed music_volume()
    {
        const SaveSettings& s = globals().profile.settings;
        return bn::fixed(s.music_volume) * bn::fixed(0.075);    // 10 -> 0.75 (headroom for effects)
    }

    bn::fixed sound_volume()
    {
        const SaveSettings& s = globals().profile.settings;
        return bn::fixed(s.sfx_volume) / 10;
    }

    void start_current()
    {
        const SaveSettings& s = globals().profile.settings;

        if(current_music < 0 || ! s.music_on || s.music_volume == 0)
        {
            if(bn::music::playing())
            {
                bn::music::stop();
            }

            music_started = false;
            return;
        }

        music_item(Music(current_music)).play(music_volume(), current_loop);
        music_started = true;

        // Hearing a track unlocks it in the gallery (saved with the next profile save).
        globals().profile.progress.music_unlocked |= uint16_t(1u << current_music);
        test_interface().music_id = uint32_t(current_music);
    }
}

void play_music(Music music, bool loop)
{
    const int index = int(music);

    if(index == current_music && music_started && bn::music::playing())
    {
        return;
    }

    current_music = index;
    current_loop = loop;
    start_current();
}

void stop_music()
{
    current_music = -1;
    music_started = false;

    if(bn::music::playing())
    {
        bn::music::stop();
    }

    test_interface().music_id = 0xFFFFFFFFu;
}

Music world_music(int world)
{
    return world >= 0 && world < 6 ? Music(int(Music::VERDANT) + world) : Music::TITLE;
}

void play_sound(Sound sound)
{
    const SaveSettings& s = globals().profile.settings;

    if(! s.sfx_on || s.sfx_volume == 0)
    {
        return;
    }

    // The same effect at most once every 3 frames, and at most 3 new effects per frame.
    const int index = int(sound);

    if((last_played[index] && frame - last_played[index] < 3) || starts_this_frame >= 3)
    {
        return;
    }

    last_played[index] = frame;
    ++starts_this_frame;
    sound_item(sound).play(sound_volume());
    test_interface().sfx_count += 1;
    test_interface().last_sfx = uint32_t(index);
}

Sound spell_sound(Ability ability)
{
    switch(ability)
    {
    case ABILITY_SHADOW_STEP: return Sound::SHADOW_STEP;
    case ABILITY_PHASE_SHIFT: return Sound::PHASE_SHIFT;
    case ABILITY_GHOST_WALK: return Sound::GHOST_WALK;
    case ABILITY_CASTLE_SWAP: return Sound::CASTLE_SWAP;
    case ABILITY_ROYAL_COMMAND: return Sound::ROYAL_COMMAND;
    default: return Sound::ROYAL_ESCAPE;
    }
}

void apply_audio_settings()
{
    const SaveSettings& s = globals().profile.settings;

    if(! s.music_on || s.music_volume == 0)
    {
        if(bn::music::playing())
        {
            bn::music::stop();
        }

        music_started = false;
    }
    else if(! music_started || ! bn::music::playing())
    {
        start_current();
    }
    else
    {
        bn::music::set_volume(music_volume());
    }
}

void audio_frame()
{
    ++frame;
    starts_this_frame = 0;
    test_interface().music_playing = bn::music::playing();
}

}
