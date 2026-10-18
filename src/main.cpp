// CHESS LEGENDS: ARCANE WARS - Game Boy Advance entry point.
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_core.h"
#include "bn_sprite_palettes.h"

#include "cl_scene.h"
#include "cl_storage.h"
#include "cl_test_commands.h"

int main()
{
    bn::core::init();
    bn::bg_palettes::set_transparent_color(bn::color(3, 2, 8));

    using namespace cl::gba;
    Globals& g = globals();

    // The profile (settings, progress, statistics) is loaded once; a damaged copy falls back to the other slot.
    const LoadStatus status = load_profile(g.profile);
    test_interface().load_status = uint32_t(status);
    test_interface().suspend_present = has_suspended_match();
    g.load_notice = status == LoadStatus::RECOVERED || status == LoadStatus::RESET;
    g.setup.board_theme = g.profile.settings.board_theme;
    g.setup.difficulty = cl::Difficulty(g.profile.settings.preferred_difficulty & 3);

    SceneId current = g.profile.progress.intro_seen ? SceneId::TITLE : SceneId::INTRO;
    bn::unique_ptr<Scene> scene = create_scene(current);
    fade_in(12);

    while(true)
    {
        bn::optional<SceneId> next = process_test_commands();

        if(! next)
        {
            next = scene->update();
        }

        if(next)
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
