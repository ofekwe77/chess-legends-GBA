// CHESS LEGENDS: ARCANE WARS - tutorial lesson list.
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "cl_art.h"
#include "cl_layers.h"
#include "cl_scenes.h"
#include "cl_text.h"
#include "cl_tutorial.h"

namespace cl::gba
{

namespace
{

class TutorialScene final : public Scene
{
public:
    TutorialScene() :
        _backdrop(create_backdrop(Backdrop::VERDANT))
    {
        set_current_scene(SceneId::TUTORIAL);
        const Globals& g = globals();
        const uint16_t done = g.profile.progress.tutorial_lessons;

        // Back from a lesson: stay on it; otherwise start on the first lesson not completed yet.
        if(g.setup.lesson >= 0 && g.setup.lesson < lessons_count)
        {
            _index = g.setup.lesson;
        }
        else
        {
            for(int i = 0; i < lessons_count; ++i)
            {
                if(! ((done >> i) & 1))
                {
                    _index = i;
                    break;
                }
            }
        }

        _panel.window(1, 1, 28, 18);
        _draw();
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        _input.update();
        _toast.update();

        if(_input.up())
        {
            _index = (_index + lessons_count - 1) % lessons_count;
            _draw();
        }
        else if(_input.down())
        {
            _index = (_index + 1) % lessons_count;
            _draw();
        }

        if(bn::keypad::a_pressed())
        {
            MatchSetup& setup = globals().setup;
            setup.mode = MatchMode::TUTORIAL;
            setup.lesson = int8_t(_index);
            setup.encounter = -1;
            setup.challenge = -1;
            setup.gauntlet_stage = -1;
            setup.resume_suspended = false;
            return SceneId::MATCH;
        }

        if(bn::keypad::b_pressed())
        {
            globals().setup.lesson = -1;
            return SceneId::MAIN_MENU;
        }

        _panel.commit();
        test_interface().menu_index = uint32_t(_index);
        test_interface().menu_count = lessons_count;
        return bn::nullopt;
    }

private:
    bn::regular_bg_ptr _backdrop;
    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 80> _text;
    RepeatInput _input;
    ToastBanner _toast;
    int _index = 0;

    void _draw()
    {
        const uint16_t done = globals().profile.progress.tutorial_lessons;
        int completed = 0;

        for(int i = 0; i < lessons_count; ++i)
        {
            completed += (done >> i) & 1;
        }

        _text.clear();
        print_big(_text, 120, 14, "TUTORIAL", TitleColor::GOLD, Align::CENTER);
        bn::string<24> count;
        bn::ostringstream stream(count);
        stream << completed << " / " << lessons_count << " lessons";
        print(_text, 120, 32, count, TextColor::CYAN, Align::CENTER);

        for(int i = 0; i < lessons_count; ++i)
        {
            const int y = 46 + i * 9;
            const bool current = i == _index;
            const bool finished = (done >> i) & 1;
            bn::string<32> line;
            bn::ostringstream line_stream(line);
            line_stream << (i + 1) << ". " << lesson(i).title;
            print(_text, 40, y, line, current ? TextColor::GOLD : TextColor::WHITE);
            print(_text, 200, y, finished ? "★" : "", TextColor::GOLD, Align::RIGHT);

            if(current)
            {
                print(_text, 28, y, "→", TextColor::GOLD);
            }
        }

        print(_text, 120, 140, "Ⓐ Start   Ⓑ Back", TextColor::GRAY, Align::CENTER);
    }
};

}

bn::unique_ptr<Scene> make_tutorial_scene()
{
    return bn::unique_ptr<Scene>(new TutorialScene());
}

}
