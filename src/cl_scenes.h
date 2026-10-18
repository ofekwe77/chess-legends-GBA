// CHESS LEGENDS: ARCANE WARS - scene factories (one per scene file).
#ifndef CL_SCENES_H
#define CL_SCENES_H

#include "cl_scene.h"

namespace cl::gba
{

bn::unique_ptr<Scene> make_title_scene();
bn::unique_ptr<Scene> make_main_menu_scene();
bn::unique_ptr<Scene> make_match_setup_scene();
bn::unique_ptr<Scene> make_match_scene();
bn::unique_ptr<Scene> make_story_map_scene();
bn::unique_ptr<Scene> make_story_dialogue_scene();
bn::unique_ptr<Scene> make_challenges_scene();
bn::unique_ptr<Scene> make_gauntlet_scene();
bn::unique_ptr<Scene> make_gallery_scene();
bn::unique_ptr<Scene> make_achievements_scene();
bn::unique_ptr<Scene> make_options_scene();
bn::unique_ptr<Scene> make_credits_scene();
bn::unique_ptr<Scene> make_tutorial_scene();
bn::unique_ptr<Scene> make_intro_scene();
bn::unique_ptr<Scene> make_ending_scene();

}

#endif
