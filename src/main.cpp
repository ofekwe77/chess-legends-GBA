// CHESS LEGENDS: ARCANE WARS - Game Boy Advance entry point.
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_sprite_palettes.h"

#include "cl_scene.h"

int main()
{
    bn::core::init();
    bn::bg_palettes::set_transparent_color(bn::color(3, 2, 8));

    using namespace cl::gba;
    SceneId current = SceneId::TITLE;
    bn::unique_ptr<Scene> scene = create_scene(current);
    fade_in(12);

    while(true)
    {
        if(bn::optional<SceneId> next = scene->update())
        {
            fade_out(10);
            scene.reset();
            current = *next;
            scene = create_scene(current);
            frame_end();
            fade_in(10);
            continue;
        }

        frame_end();
    }
}
