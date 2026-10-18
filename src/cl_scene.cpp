// CHESS LEGENDS: ARCANE WARS - per-frame services and input helpers.
#include "cl_scene.h"

#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_fixed.h"
#include "bn_keypad.h"
#include "bn_memory.h"
#include "bn_sprite_palettes.h"

namespace cl::gba
{

namespace
{
    SceneId current_scene = SceneId::BOOT;
    int cpu_max_percent = 0;
}

void set_current_scene(SceneId id)
{
    current_scene = id;
}

void frame_end()
{
    Globals& g = globals();
    cl_test_interface& ti = test_interface();
    ++g.frame_counter;
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
    ti.ewram_free = uint32_t(bn::memory::available_alloc_ewram());
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
