// CHESS LEGENDS: ARCANE WARS - per-frame services and input helpers.
#include "cl_scene.h"

#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_memory.h"
#include "bn_sprite_palettes.h"
#include "bn_sprites.h"
#include "bn_string.h"
#include "bn_timers.h"

#include "cl_audio.h"
#include "cl_progress.h"
#include "cl_text.h"

#include "bn_sprite_items_banner.h"

// Linker symbols (Butano): end of the static IWRAM data and top of the IWRAM stack.
extern unsigned BN_IWRAM_END;
extern unsigned BN_IWRAM_TOP;

namespace cl::gba
{

namespace
{
    SceneId current_scene = SceneId::BOOT;
    int cpu_max_percent = 0;
    int sprites_max = 0;
    int missed_frames = 0;
    int profile_worst[int(ProfileSlot::COUNT)] = {};

    constexpr uint32_t stack_paint = 0xC0DEFACEu;
    uint32_t* stack_untouched_end = nullptr;     // first word above the never-touched part of the stack

    uint32_t* stack_bottom()
    {
        const uintptr_t end = reinterpret_cast<uintptr_t>(&BN_IWRAM_END);
        return reinterpret_cast<uint32_t*>((end + 3) & ~uintptr_t(3));
    }

    // Bytes of stack never used since boot: the painted words still intact above the static data. The boundary
    // only moves down, so each measurement scans just the untouched part.
    int stack_free_bytes()
    {
        if(! stack_untouched_end)
        {
            return 0;
        }

        uint32_t* bottom = stack_bottom();
        uint32_t* word = bottom;

        while(word < stack_untouched_end && *word == stack_paint)
        {
            ++word;
        }

        stack_untouched_end = word;
        return reinterpret_cast<uint8_t*>(word) - reinterpret_cast<uint8_t*>(bottom);
    }
}

void paint_stack()
{
    // Everything from the static data up to 256 bytes below the current stack pointer is free stack.
    uintptr_t stack_pointer = 0;
    asm volatile("mov %0, sp" : "=r" (stack_pointer));
    uint32_t* limit = reinterpret_cast<uint32_t*>((stack_pointer - 256) & ~uintptr_t(3));

    for(uint32_t* word = stack_bottom(); word < limit; ++word)
    {
        *word = stack_paint;
    }

    stack_untouched_end = limit;
}

void set_current_scene(SceneId id)
{
    current_scene = id;
}

void reset_cpu_statistics()
{
    cpu_max_percent = 0;
    sprites_max = 0;
    missed_frames = 0;

    for(int& worst : profile_worst)
    {
        worst = 0;
    }
}

void profile_note(ProfileSlot slot, int timer_ticks)
{
    const int percent = timer_ticks * 100 / bn::timers::ticks_per_frame();
    int& worst = profile_worst[int(slot)];
    worst = percent > worst ? percent : worst;
}

void frame_end()
{
    Globals& g = globals();
    cl_test_interface& ti = test_interface();
    ++g.frame_counter;
    ++g.play_frames;
    ti.magic0 = CL_TI_MAGIC0;
    ti.magic1 = CL_TI_MAGIC1;
    ti.frame = g.frame_counter;
    ti.scene = uint32_t(current_scene);

    int cpu = (bn::core::last_cpu_usage() * 100).right_shift_integer();
    ti.cpu_last = uint32_t(cpu);

    if(cpu > cpu_max_percent)
    {
        cpu_max_percent = cpu;
    }

    ti.cpu_max = uint32_t(cpu_max_percent);
    missed_frames += bn::core::last_missed_frames();
    ti.fps_drops = uint32_t(missed_frames);
    ti.prof_panel = uint32_t(profile_worst[int(ProfileSlot::PANEL)]);
    ti.prof_pieces = uint32_t(profile_worst[int(ProfileSlot::PIECES)]);
    ti.prof_execute = uint32_t(profile_worst[int(ProfileSlot::EXECUTE)]);
    ti.prof_search_start = uint32_t(profile_worst[int(ProfileSlot::SEARCH_START)]);
    ti.prof_apply = uint32_t(profile_worst[int(ProfileSlot::APPLY)]);
    ti.prof_after_action = uint32_t(profile_worst[int(ProfileSlot::AFTER_ACTION)]);
    ti.prof_begin_turn = uint32_t(profile_worst[int(ProfileSlot::BEGIN_TURN)]);
    ti.prof_status = uint32_t(profile_worst[int(ProfileSlot::STATUS)]);
    ti.prof_board_commit = uint32_t(profile_worst[int(ProfileSlot::BOARD_COMMIT)]);

    if((g.frame_counter & 15) == 0)
    {
        ti.stack_free = uint32_t(stack_free_bytes());
    }

    ti.ewram_free = uint32_t(bn::memory::available_alloc_ewram());
    const int sprites = bn::sprites::used_items_count();
    sprites_max = sprites > sprites_max ? sprites : sprites_max;
    ti.sprites = uint32_t(sprites);
    ti.sprites_max = uint32_t(sprites_max);

    // Profile facts, whatever the scene.
    const SaveProgress& progress = g.profile.progress;
    ti.achievements_lo = uint32_t(progress.achievements);
    ti.achievements_hi = uint32_t(progress.achievements >> 32);
    ti.challenge_medals = progress.challenge_medals;
    ti.campaign_cleared = progress.encounters_cleared;
    ti.unlocked_worlds = uint32_t(progress.story_world + 1);
    ti.tutorial_step = progress.tutorial_lessons;
    ti.text_speed = g.profile.settings.text_speed;
    ti.anim_mode = g.profile.settings.animations;
    ti.volume_music = g.profile.settings.music_volume;
    ti.volume_sfx = g.profile.settings.sfx_volume;
    audio_frame();
    bn::core::update();
}

void RepeatInput::update()
{
    const bool held[4] = { bn::keypad::left_held(), bn::keypad::right_held(), bn::keypad::up_held(),
                           bn::keypad::down_held() };
    _fired = 0;

    for(int i = 0; i < 4; ++i)
    {
        if(! held[i])
        {
            _held_frames[i] = 0;
            continue;
        }

        int frames = _held_frames[i]++;

        // Fire on press, then after a 14-frame delay every 4 frames.
        if(frames == 0 || (frames >= 14 && (frames - 14) % 4 == 0))
        {
            _fired |= 1 << i;
        }
    }
}

namespace
{
    constexpr int toast_frames = 150;
    constexpr int toast_slide = 10;
    constexpr int toast_rest_y = 2;
    constexpr int banner_sprites = 8;     // 32 px frames from x = 4 to x = 236 (the last two overlap)
}

bool ToastBanner::update(bool allowed)
{
    Globals& g = globals();

    if(! allowed)
    {
        if(_frames > 0)
        {
            g.toasts |= uint64_t(1) << _id;
            _frames = 0;
            _banner.clear();
            _text.clear();
            test_interface().toast = 0;
        }

        return false;
    }

    if(_frames > 0)
    {
        --_frames;
        const int elapsed = toast_frames - _frames;
        int y = toast_rest_y;

        if(elapsed < toast_slide)
        {
            y = toast_rest_y - (toast_slide - elapsed) * 2;
        }
        else if(_frames < toast_slide)
        {
            y = toast_rest_y - (toast_slide - _frames) * 2;
        }

        _place(y);

        if(_frames == 0)
        {
            _banner.clear();
            _text.clear();
            test_interface().toast = 0;
        }

        return true;
    }

    if(! g.toasts)
    {
        return false;
    }

    int id = 0;

    while(! ((g.toasts >> id) & 1))
    {
        ++id;
    }

    g.toasts &= ~(uint64_t(1) << id);
    _id = id;
    _banner.clear();
    _text.clear();

    for(int i = 0; i < banner_sprites; ++i)
    {
        const int frame = i == 0 ? 0 : (i == banner_sprites - 1 ? 2 : 1);
        bn::sprite_ptr sprite = bn::sprite_items::banner.create_sprite(0, 0, frame);
        sprite.set_bg_priority(0);
        sprite.set_z_order(-20);
        _banner.push_back(bn::move(sprite));
    }

    bn::string<48> text;
    bn::ostringstream stream(text);
    stream << "★ " << achievement_info(Achievement(id)).title;
    print(_text, 120, 0, text, TextColor::GOLD, Align::CENTER, 0);

    for(bn::sprite_ptr& sprite : _text)
    {
        sprite.set_z_order(-30);
    }

    _frames = toast_frames;
    _place(toast_rest_y - toast_slide * 2);
    play_sound(Sound::ACHIEVEMENT);
    test_interface().toast = uint32_t(id + 1);
    return true;
}

void ToastBanner::_place(int y)
{
    for(int i = 0; i < _banner.size(); ++i)
    {
        const int x = i == banner_sprites - 1 ? 236 - 32 : 4 + i * 32;
        _banner[i].set_position(x + 16 - 120, y + 8 - 80);
    }

    // Text rows are 8 px high, vertically centred in the 16 px banner.
    for(bn::sprite_ptr& sprite : _text)
    {
        sprite.set_y(y + 4 + 4 - 80);
    }
}

void fade_out(int frames)
{
    for(int i = 1; i <= frames; ++i)
    {
        bn::fixed intensity = bn::fixed(i) / frames;
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), intensity);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), intensity);
        frame_end();
    }
}

void fade_in(int frames)
{
    for(int i = frames - 1; i >= 0; --i)
    {
        bn::fixed intensity = bn::fixed(i) / frames;
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), intensity);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), intensity);
        frame_end();
    }
}

}
