// CHESS LEGENDS: ARCANE WARS - title, main menu and match setup scenes, and the scene dispatcher.
#include "bn_keypad.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "cl_layers.h"
#include "cl_progress.h"
#include "cl_scene_match.h"
#include "cl_scenes.h"
#include "cl_storage.h"
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

        if(globals().load_notice)
        {
            globals().load_notice = false;
            print(_text, 120, 96, "The save was damaged and has been repaired", TextColor::RED, Align::CENTER);
            print(_text, 120, 106, "from its backup copy where possible.", TextColor::RED, Align::CENTER);
        }

        print(_text, 120, 148, "An original Game Boy Advance chess fantasy", TextColor::GRAY, Align::CENTER);
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        ++_frames;

        if((_frames & 31) == 0)
        {
            _press.clear();

            if((_frames & 32) == 0)
            {
                print(_press, 120, 124, "PRESS START", TextColor::WHITE, Align::CENTER);
            }
        }

        _toast.update();

        if(_frames > 20 && (bn::keypad::start_pressed() || bn::keypad::a_pressed()))
        {
            return SceneId::MAIN_MENU;
        }

        return bn::nullopt;
    }

private:
    bn::vector<bn::sprite_ptr, 40> _text;
    bn::vector<bn::sprite_ptr, 4> _press;
    ToastBanner _toast;
    int _frames = 0;
};

// ------------------------------------------------------------------------------------------------ main menu

enum class MenuItem : uint8_t
{
    CONTINUE,
    STORY,
    CLASSIC,
    ARCANE,
    CHALLENGES,
    GAUNTLET,
    TUTORIAL,
    GALLERY,
    ACHIEVEMENTS,
    OPTIONS,
    CREDITS
};

struct MenuEntry
{
    MenuItem item;
    const char* label;
    const char* description;
};

constexpr MenuEntry play_entries[] = {
    { MenuItem::CONTINUE, "CONTINUE", "Resume your saved match." },
    { MenuItem::STORY, "STORY MODE", "Six kingdoms. Restore the Crown of Six." },
    { MenuItem::CLASSIC, "CLASSIC CHESS", "Traditional chess, without magic." },
    { MenuItem::ARCANE, "ARCANE CHESS", "Chess with mana and six spells." },
    { MenuItem::CHALLENGES, "CHALLENGES", "Fourteen tactical puzzles to master." },
    { MenuItem::GAUNTLET, "BOSS GAUNTLET", "All six rulers, one after another." },
    { MenuItem::TUTORIAL, "TUTORIAL", "Learn the board and every spell." },
};

constexpr MenuEntry extra_entries[] = {
    { MenuItem::GALLERY, "GALLERY", "Portraits, kingdoms, music and spells." },
    { MenuItem::ACHIEVEMENTS, "ACHIEVEMENTS", "Thirty-two feats to unlock." },
    { MenuItem::OPTIONS, "OPTIONS", "Audio, display and save data." },
    { MenuItem::CREDITS, "CREDITS", "About this game." },
};

constexpr int play_count = int(sizeof(play_entries) / sizeof(MenuEntry));
constexpr int extra_count = int(sizeof(extra_entries) / sizeof(MenuEntry));

bool gauntlet_unlocked()
{
    return encounter_cleared(globals().profile.progress, campaign_encounters - 1);
}

class MainMenuScene final : public Scene
{
public:
    MainMenuScene()
    {
        set_current_scene(SceneId::MAIN_MENU);
        _has_suspended = has_suspended_match();
        test_interface().suspend_present = _has_suspended;
        _panel.window(1, 1, 28, 18);
        print_big(_title, 120, 14, "CHESS LEGENDS", TitleColor::GOLD, Align::CENTER);
        _row = _has_suspended ? 0 : 1;
        _draw();
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        _input.update();
        const int rows = _column == 0 ? play_count : extra_count;

        if(_input.up())
        {
            _step(-1);
        }
        else if(_input.down())
        {
            _step(1);
        }
        else if(_input.left() || _input.right())
        {
            _column ^= 1;
            const int other = _column == 0 ? play_count : extra_count;
            _row = _row >= other ? other - 1 : _row;

            if(_column == 0 && _row == 0 && ! _has_suspended)
            {
                _row = 1;
            }

            _draw();
        }

        (void) rows;
        _toast.update();

        if(bn::keypad::a_pressed())
        {
            if(bn::optional<SceneId> next = _activate())
            {
                return next;
            }
        }

        if(bn::keypad::b_pressed())
        {
            return SceneId::TITLE;
        }

        if(_message_frames > 0 && --_message_frames == 0)
        {
            _draw();
        }

        _panel.commit();
        test_interface().menu_index = uint32_t(_column * 16 + _row);
        test_interface().menu_count = uint32_t(play_count + extra_count);
        return bn::nullopt;
    }

private:
    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 12> _title;
    bn::vector<bn::sprite_ptr, 64> _items;
    RepeatInput _input;
    ToastBanner _toast;
    int _column = 0;
    int _row = 1;
    int _message_frames = 0;
    bool _has_suspended = false;
    const char* _message = nullptr;

    void _step(int delta)
    {
        const int rows = _column == 0 ? play_count : extra_count;

        do
        {
            _row = (_row + rows + delta) % rows;
        }
        while(_column == 0 && _row == 0 && ! _has_suspended);

        _draw();
    }

    [[nodiscard]] const MenuEntry& _current() const
    {
        return _column == 0 ? play_entries[_row] : extra_entries[_row];
    }

    bn::optional<SceneId> _activate()
    {
        Globals& g = globals();
        MatchSetup& setup = g.setup;

        switch(_current().item)
        {
        case MenuItem::CONTINUE:
            return _resume();

        case MenuItem::STORY:
            return SceneId::STORY_MAP;

        case MenuItem::CLASSIC:
            setup.mode = MatchMode::CLASSIC;
            setup.arcane = false;
            return SceneId::MATCH_SETUP;

        case MenuItem::ARCANE:
            setup.mode = MatchMode::ARCANE;
            setup.arcane = true;
            return SceneId::MATCH_SETUP;

        case MenuItem::CHALLENGES:
            return SceneId::CHALLENGES;

        case MenuItem::GAUNTLET:
            if(! gauntlet_unlocked())
            {
                _show_message("Complete the story to unlock the gauntlet.");
                return bn::nullopt;
            }

            setup.gauntlet_stage = -1;     // the gauntlet scene starts a new run
            return SceneId::GAUNTLET;

        case MenuItem::TUTORIAL:
            return SceneId::TUTORIAL;

        case MenuItem::GALLERY:
            return SceneId::GALLERY;

        case MenuItem::ACHIEVEMENTS:
            return SceneId::ACHIEVEMENTS;

        case MenuItem::OPTIONS:
            return SceneId::OPTIONS;

        default:
            return SceneId::CREDITS;
        }
    }

    bn::optional<SceneId> _resume()
    {
        Globals& g = globals();
        SuspendedMatch& s = g.suspended;

        if(! load_suspended_match(s) || ! restore_game(s, g.game))
        {
            clear_suspended_match();
            _has_suspended = false;
            _row = 1;
            _show_message("The saved match could not be restored.");
            return bn::nullopt;
        }

        MatchSetup& setup = g.setup;
        setup.mode = MatchMode(s.mode);
        setup.arcane = g.game.position().rules.arcane;
        setup.human_mask = s.human_mask;
        setup.difficulty = Difficulty(s.difficulty);
        setup.personality = Personality(s.personality);
        setup.board_theme = s.board_theme;
        setup.opponent_faction = s.opponent_faction;
        setup.encounter = s.encounter;
        setup.challenge = s.challenge;
        setup.gauntlet_stage = s.gauntlet_stage;
        setup.resume_suspended = true;

        // A suspended match resumes once: it is consumed now (it is saved again by another "save and quit").
        clear_suspended_match();
        test_interface().suspend_present = 0;
        return SceneId::MATCH;
    }

    void _show_message(const char* message)
    {
        _message = message;
        _message_frames = 150;
        _draw();
    }

    void _draw()
    {
        _items.clear();
        print(_items, 64, 38, "PLAY", TextColor::CYAN, Align::CENTER);
        print(_items, 176, 38, "EXTRAS", TextColor::CYAN, Align::CENTER);

        for(int i = 0; i < play_count; ++i)
        {
            const MenuEntry& entry = play_entries[i];
            const bool locked = (entry.item == MenuItem::CONTINUE && ! _has_suspended) ||
                                (entry.item == MenuItem::GAUNTLET && ! gauntlet_unlocked());
            const bool current = _column == 0 && _row == i;
            const TextColor color = current ? TextColor::GOLD : (locked ? TextColor::GRAY : TextColor::WHITE);
            print(_items, 28, 50 + i * 11, entry.label, color);

            if(current)
            {
                print(_items, 18, 50 + i * 11, "→", TextColor::GOLD);
            }
        }

        for(int i = 0; i < extra_count; ++i)
        {
            const bool current = _column == 1 && _row == i;
            print(_items, 146, 50 + i * 11, extra_entries[i].label, current ? TextColor::GOLD : TextColor::WHITE);

            if(current)
            {
                print(_items, 136, 50 + i * 11, "→", TextColor::GOLD);
            }
        }

        if(_message_frames > 0 && _message)
        {
            print(_items, 120, 127, _message, TextColor::RED, Align::CENTER);
        }
        else
        {
            print(_items, 120, 127, _current().description, TextColor::CYAN, Align::CENTER);
        }

        print(_items, 120, 138, "Ⓐ Select   Ⓑ Title", TextColor::GRAY, Align::CENTER);
    }
};

// ------------------------------------------------------------------------------------------------ match setup

constexpr const char* theme_names[board_themes_count] = {
    "ROYAL", "VERDANT", "INFERNO", "FROST", "UMBRAL", "STORM", "NEXUS", "CONTRAST"
};

class MatchSetupScene final : public Scene
{
public:
    MatchSetupScene()
    {
        set_current_scene(SceneId::MATCH_SETUP);
        _panel.window(3, 2, 24, 17);
        MatchSetup& setup = globals().setup;
        const SaveSettings& settings = globals().profile.settings;
        setup.board_theme = settings.board_theme;

        if(! board_theme_unlocked(globals().profile.progress, setup.board_theme))
        {
            setup.board_theme = 0;
        }

        setup.difficulty = Difficulty(settings.preferred_difficulty & 3);
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
            {
                int theme = setup.board_theme;

                for(int i = 0; i < board_themes_count; ++i)
                {
                    theme = (theme + board_themes_count + delta) % board_themes_count;

                    if(board_theme_unlocked(globals().profile.progress, theme))
                    {
                        break;
                    }
                }

                setup.board_theme = uint8_t(theme);
                break;
            }

            default:
                break;
            }

            _draw();
        }

        _toast.update();

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

            // Remember the choices for next time.
            SaveSettings& settings = globals().profile.settings;
            settings.board_theme = setup.board_theme;
            settings.preferred_difficulty = uint8_t(setup.difficulty);
            setup.personality = Personality::BALANCED;
            setup.opponent_faction = 0;
            setup.encounter = -1;
            setup.challenge = -1;
            setup.gauntlet_stage = -1;
            setup.resume_suspended = false;
            save_profile_now();
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
    ToastBanner _toast;
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
        const char* values[rows] = {
            _two_players ? "TWO PLAYERS" : "COMPUTER",
            _two_players ? "-" : sides[_side],
            _two_players ? "-" : difficulty_name(setup.difficulty),
            theme_names[setup.board_theme % board_themes_count],
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

bn::unique_ptr<Scene> make_title_scene()
{
    return bn::unique_ptr<Scene>(new TitleScene());
}

bn::unique_ptr<Scene> make_main_menu_scene()
{
    return bn::unique_ptr<Scene>(new MainMenuScene());
}

bn::unique_ptr<Scene> make_match_setup_scene()
{
    return bn::unique_ptr<Scene>(new MatchSetupScene());
}

bn::unique_ptr<Scene> make_match_scene()
{
    return bn::unique_ptr<Scene>(new MatchScene());
}

bn::unique_ptr<Scene> create_scene(SceneId id)
{
    switch(id)
    {
    case SceneId::INTRO: return make_title_scene();
    case SceneId::TITLE: return make_title_scene();
    case SceneId::MATCH_SETUP: return make_match_setup_scene();
    case SceneId::MATCH: return make_match_scene();
    case SceneId::STORY_MAP: return make_story_map_scene();
    case SceneId::STORY_DIALOGUE: return make_story_dialogue_scene();
    case SceneId::CHALLENGES: return make_challenges_scene();
    case SceneId::GAUNTLET: return make_gauntlet_scene();
    case SceneId::GALLERY: return make_gallery_scene();
    case SceneId::ACHIEVEMENTS: return make_achievements_scene();
    case SceneId::OPTIONS: return make_options_scene();
    case SceneId::CREDITS: return make_credits_scene();
    default: return make_main_menu_scene();
    }
}

}
