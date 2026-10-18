// CHESS LEGENDS: ARCANE WARS - title, main menu and match setup scenes.
#include "bn_keypad.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "cl_layers.h"
#include "cl_scene.h"
#include "cl_scene_match.h"
#include "cl_text.h"

namespace cl::gba
{

namespace
{

// ------------------------------------------------------------------------------------------------ title

class TitleScene final : public Scene
{
public:
    TitleScene()
    {
        set_current_scene(SceneId::TITLE);
        print_big(_text, 120, 40, "CHESS LEGENDS", TitleColor::GOLD, Align::CENTER);
        print_big(_text, 120, 62, "ARCANE WARS", TitleColor::ARCANE, Align::CENTER);
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        ++_frames;

        if((_frames & 31) == 0)
        {
            _press.clear();

            if((_frames & 32) == 0)
            {
                print(_press, 120, 120, "PRESS START", TextColor::WHITE, Align::CENTER);
            }
        }

        if(_frames > 20 && (bn::keypad::start_pressed() || bn::keypad::a_pressed()))
        {
            return SceneId::MAIN_MENU;
        }

        return bn::nullopt;
    }

private:
    bn::vector<bn::sprite_ptr, 16> _text;
    bn::vector<bn::sprite_ptr, 4> _press;
    int _frames = 0;
};

// ------------------------------------------------------------------------------------------------ main menu

class MainMenuScene final : public Scene
{
public:
    MainMenuScene()
    {
        set_current_scene(SceneId::MAIN_MENU);
        _panel.window(7, 2, 18, 17);
        print_big(_title, 120, 22, "MAIN MENU", TitleColor::GOLD, Align::CENTER);
        _draw();
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        _input.update();

        if(_input.up())
        {
            _index = (_index + count - 1) % count;
            _draw();
        }
        else if(_input.down())
        {
            _index = (_index + 1) % count;
            _draw();
        }

        if(bn::keypad::a_pressed())
        {
            MatchSetup& setup = globals().setup;

            switch(_index)
            {
            case 1:
                setup.mode = MatchMode::CLASSIC;
                setup.arcane = false;
                return SceneId::MATCH_SETUP;

            case 2:
                setup.mode = MatchMode::ARCANE;
                setup.arcane = true;
                return SceneId::MATCH_SETUP;

            default:
                break;
            }
        }

        if(bn::keypad::b_pressed())
        {
            return SceneId::TITLE;
        }

        _panel.commit();
        test_interface().menu_index = uint32_t(_index);
        test_interface().menu_count = count;
        return bn::nullopt;
    }

private:
    static constexpr int count = 9;
    static constexpr const char* items[count] = {
        "STORY MODE", "CLASSIC CHESS", "ARCANE CHESS", "CHALLENGES", "BOSS GAUNTLET", "GALLERY",
        "ACHIEVEMENTS", "OPTIONS", "CREDITS"
    };

    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 8> _title;
    bn::vector<bn::sprite_ptr, 40> _items;
    RepeatInput _input;
    int _index = 2;

    void _draw()
    {
        _items.clear();

        for(int i = 0; i < count; ++i)
        {
            print(_items, 120, 44 + i * 11, items[i], i == _index ? TextColor::GOLD : TextColor::WHITE, Align::CENTER);
        }

        print(_items, 74, 44 + _index * 11, "→", TextColor::GOLD);
    }
};

// ------------------------------------------------------------------------------------------------ match setup

class MatchSetupScene final : public Scene
{
public:
    MatchSetupScene()
    {
        set_current_scene(SceneId::MATCH_SETUP);
        _panel.window(3, 2, 24, 17);
        _draw();
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        _input.update();
        MatchSetup& setup = globals().setup;

        if(_input.up())
        {
            _row = (_row + rows - 1) % rows;
            _draw();
        }
        else if(_input.down())
        {
            _row = (_row + 1) % rows;
            _draw();
        }
        else if(_input.left() || _input.right())
        {
            const int delta = _input.right() ? 1 : -1;

            switch(_row)
            {
            case 0:
                _two_players = ! _two_players;
                break;

            case 1:
                _side = (_side + 3 + delta) % 3;
                break;

            case 2:
                setup.difficulty = Difficulty((int(setup.difficulty) + 4 + delta) % 4);
                break;

            case 3:
                setup.board_theme = uint8_t((setup.board_theme + 8 + delta) % 8);
                break;

            default:
                break;
            }

            _draw();
        }

        if(bn::keypad::a_pressed() && _row == rows - 1)
        {
            if(_two_players)
            {
                setup.human_mask = 3;
            }
            else
            {
                int side = _side;

                if(side == 2)
                {
                    side = int(cosmetic_random() & 1);
                }

                setup.human_mask = side == 0 ? 1 : 2;
            }

            setup.personality = Personality::BALANCED;
            setup.opponent_faction = 0;
            setup.resume_suspended = false;
            return SceneId::MATCH;
        }

        if(bn::keypad::b_pressed())
        {
            return SceneId::MAIN_MENU;
        }

        _panel.commit();
        test_interface().menu_index = uint32_t(_row);
        test_interface().menu_count = rows;
        return bn::nullopt;
    }

private:
    static constexpr int rows = 5;
    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 48> _text;
    RepeatInput _input;
    int _row = rows - 1;
    int _side = 0;
    bool _two_players = false;

    void _draw()
    {
        _text.clear();
        const MatchSetup& setup = globals().setup;
        print_big(_text, 120, 22, setup.arcane ? "ARCANE CHESS" : "CLASSIC CHESS", TitleColor::GOLD, Align::CENTER);

        constexpr const char* labels[rows] = { "OPPONENT", "YOUR SIDE", "DIFFICULTY", "BOARD", "" };
        constexpr const char* sides[] = { "WHITE", "BLACK", "RANDOM" };
        constexpr const char* themes[] = { "ROYAL", "VERDANT", "INFERNO", "FROST", "UMBRAL", "STORM", "NEXUS",
                                           "CONTRAST" };
        const char* values[rows] = {
            _two_players ? "TWO PLAYERS" : "COMPUTER",
            _two_players ? "-" : sides[_side],
            _two_players ? "-" : difficulty_name(setup.difficulty),
            themes[setup.board_theme % 8],
            ""
        };

        for(int i = 0; i < rows - 1; ++i)
        {
            const int y = 50 + i * 16;
            const bool current = i == _row;
            print(_text, 44, y, labels[i], current ? TextColor::GOLD : TextColor::GRAY);
            print(_text, 200, y, values[i], current ? TextColor::GOLD : TextColor::WHITE, Align::RIGHT);

            if(current)
            {
                print(_text, 32, y, "→", TextColor::GOLD);
            }
        }

        print_big(_text, 120, 118, "START", _row == rows - 1 ? TitleColor::GOLD : TitleColor::DIM, Align::CENTER);
        print(_text, 120, 140, "◆ Left / Right to change ◆", TextColor::GRAY, Align::CENTER);
    }
};

}

bn::unique_ptr<Scene> create_scene(SceneId id)
{
    switch(id)
    {
    case SceneId::TITLE:
        return bn::unique_ptr<Scene>(new TitleScene());

    case SceneId::MATCH_SETUP:
        return bn::unique_ptr<Scene>(new MatchSetupScene());

    case SceneId::MATCH:
        return bn::unique_ptr<Scene>(new MatchScene());

    default:
        return bn::unique_ptr<Scene>(new MainMenuScene());
    }
}

}
