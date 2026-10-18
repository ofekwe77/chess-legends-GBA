// CHESS LEGENDS: ARCANE WARS - gallery, achievements, options and credits scenes.
#include "bn_algorithm.h"
#include "bn_bg_palettes.h"
#include "bn_color.h"
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "cl_art.h"
#include "cl_help.h"
#include "cl_layers.h"
#include "cl_notation.h"
#include "cl_progress.h"
#include "cl_scenes.h"
#include "cl_storage.h"
#include "cl_text.h"
#include "cl_text_layout.h"

#include "bn_sprite_items_achievement_icons.h"

namespace cl::gba
{

namespace
{
    constexpr int locked_icon = 32;     // padlock frame in achievement_icons.bmp

    constexpr const char* theme_names[board_themes_count] = {
        "ROYAL", "VERDANT", "INFERNO", "FROST", "UMBRAL", "STORM", "NEXUS", "CONTRAST"
    };

    // Prints wrapped text; returns the number of lines used.
    int print_wrapped(bn::ivector<bn::sprite_ptr>& output, int x, int y, const char* text, int width, int max_lines,
                      TextColor color, int line_height = 10, Align align = Align::LEFT)
    {
        TextSpan spans[8];
        const int limit = max_lines < 8 ? max_lines : 8;
        const int lines = wrap_lines(FontId::SMALL, text, bn::string_view(text).size(), width, spans, limit);

        for(int i = 0; i < lines; ++i)
        {
            print(output, x, y + i * line_height, bn::string_view(text + spans[i].start, spans[i].length), color,
                  align);
        }

        return lines;
    }

// ------------------------------------------------------------------------------------------------ achievements

class AchievementsScene final : public Scene
{
public:
    AchievementsScene() :
        _backdrop(create_backdrop(Backdrop::TITLE))
    {
        set_current_scene(SceneId::ACHIEVEMENTS);
        _panel.window(0, 0, 30, 20);
        _draw();
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        _input.update();
        _toast.update();

        if(bn::keypad::r_pressed() || bn::keypad::l_pressed())
        {
            _statistics = ! _statistics;
            _draw();
        }
        else if(! _statistics && _input.up())
        {
            _index = (_index + achievements_count - 1) % achievements_count;
            _draw();
        }
        else if(! _statistics && _input.down())
        {
            _index = (_index + 1) % achievements_count;
            _draw();
        }

        if(bn::keypad::b_pressed())
        {
            return SceneId::MAIN_MENU;
        }

        _panel.commit();
        test_interface().menu_index = uint32_t(_statistics ? 100 : _index);
        return bn::nullopt;
    }

private:
    static constexpr int visible_rows = 4;

    bn::regular_bg_ptr _backdrop;
    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 110> _text;
    bn::vector<bn::sprite_ptr, visible_rows> _icons;
    RepeatInput _input;
    ToastBanner _toast;
    int _index = 0;
    int _first = 0;
    bool _statistics = false;

    void _draw()
    {
        _text.clear();
        _icons.clear();

        if(_statistics)
        {
            _draw_statistics();
            return;
        }

        const SaveProgress& progress = globals().profile.progress;
        int unlocked = 0;

        for(int i = 0; i < achievements_count; ++i)
        {
            unlocked += has_achievement(progress, Achievement(i));
        }

        print_big(_text, 12, 6, "ACHIEVEMENTS", TitleColor::GOLD);
        bn::string<16> count;
        bn::ostringstream count_stream(count);
        count_stream << unlocked << " / " << achievements_count;
        print(_text, 228, 10, count, TextColor::GOLD, Align::RIGHT);

        if(_index < _first)
        {
            _first = _index;
        }
        else if(_index >= _first + visible_rows)
        {
            _first = _index - visible_rows + 1;
        }

        for(int row = 0; row < visible_rows; ++row)
        {
            const int i = _first + row;
            const AchievementInfo& info = achievement_info(Achievement(i));
            const bool has = has_achievement(progress, Achievement(i));
            const bool current = i == _index;
            const int y = 28 + row * 28;
            bn::sprite_ptr icon = bn::sprite_items::achievement_icons.create_sprite(12 + 8 - 120, y + 4 + 8 - 80,
                                                                                    has ? info.icon : locked_icon);
            icon.set_bg_priority(0);
            _icons.push_back(bn::move(icon));
            print(_text, 34, y, info.title, has ? (current ? TextColor::GOLD : TextColor::WHITE) :
                                                  (current ? TextColor::CYAN : TextColor::GRAY));
            print_wrapped(_text, 34, y + 9, info.description, layout::screen_text_width - 36, 2, TextColor::GRAY, 8);

            if(current)
            {
                print(_text, 4, y + 4, "→", TextColor::GOLD);
            }
        }

        print(_text, 120, 146, "Ⓡ Statistics   Ⓑ Back", TextColor::GRAY, Align::CENTER);
    }

    void _draw_statistics()
    {
        const Profile& profile = globals().profile;
        const SaveStats& s = profile.stats;
        print_big(_text, 12, 6, "STATISTICS", TitleColor::GOLD);
        int y = 25;

        auto line = [&](const char* label, const bn::string<32>& value)
        {
            print(_text, 16, y, label, TextColor::GRAY);
            print(_text, 224, y, value, TextColor::WHITE, Align::RIGHT);
            y += 9;
        };

        auto number = [](uint32_t value)
        {
            bn::string<32> text;
            bn::ostringstream stream(text);
            stream << value;
            return text;
        };

        bn::string<32> record;
        bn::ostringstream record_stream(record);
        record_stream << s.games_won << " / " << s.games_lost << " / " << s.games_drawn;
        line("Games played", number(s.games_played));
        line("Won / lost / drawn", record);
        line("Checkmates delivered", number(s.checkmates));
        line("Checks and captures", [&] {
            bn::string<32> text;
            bn::ostringstream stream(text);
            stream << s.checks << " / " << s.captures;
            return text;
        }());
        line("Queens captured", number(s.queens_captured));
        line("Promotions / castles", [&] {
            bn::string<32> text;
            bn::ostringstream stream(text);
            stream << s.promotions << " / " << s.castles;
            return text;
        }());
        bn::string<32> spells;
        bn::ostringstream spells_stream(spells);

        for(int a = 0; a < ABILITIES_COUNT; ++a)
        {
            spells_stream << (a ? " " : "") << s.spells[a];
        }

        line("Spells (SS PS GW CS RC RE)", spells);
        line("Best win streak", number(s.best_streak));
        constexpr const char* levels[5] = { "-", "Apprentice", "Knight", "Sorcerer", "Grandmaster" };
        bn::string<32> best;
        best.append(levels[s.best_difficulty_beaten < 5 ? s.best_difficulty_beaten : 0]);
        line("Strongest opponent beaten", best);
        bn::string<32> time;
        bn::ostringstream time_stream(time);
        const uint32_t minutes = (s.play_seconds + globals().play_frames / 60) / 60;
        time_stream << minutes / 60 << "h " << (minutes % 60 < 10 ? "0" : "") << minutes % 60 << "m";
        line("Play time", time);
        int stars = 0;

        for(int c = 0; c < challenges_count; ++c)
        {
            stars += challenge_medal_of(profile.progress, c);
        }

        int cleared = 0;

        for(int e = 0; e < campaign_encounters; ++e)
        {
            cleared += encounter_cleared(profile.progress, e);
        }

        bn::string<32> story;
        bn::ostringstream story_stream(story);
        story_stream << cleared << " / " << campaign_encounters;
        line("Story encounters cleared", story);
        bn::string<32> medals;
        bn::ostringstream medals_stream(medals);
        medals_stream << "★ " << stars << " / " << challenges_count * 3;
        line("Challenge medals", medals);
        bn::string<32> gauntlet;
        bn::ostringstream gauntlet_stream(gauntlet);
        gauntlet_stream << int(profile.progress.gauntlet_best) << " / " << gauntlet_stages;
        line("Best gauntlet run", gauntlet);
        print(_text, 120, 146, "Ⓡ Achievements   Ⓑ Back", TextColor::GRAY, Align::CENTER);
    }
};

// ------------------------------------------------------------------------------------------------ options

class OptionsScene final : public Scene
{
public:
    OptionsScene() :
        _backdrop(create_backdrop(Backdrop::TITLE))
    {
        set_current_scene(SceneId::OPTIONS);
        _panel.window(0, 0, 30, 20);
        _draw();
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        _input.update();
        _toast.update();

        if(_confirm)
        {
            return _update_confirm();
        }

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
        else if(_input.left() || _input.right() || bn::keypad::a_pressed())
        {
            if(_row == ERASE && bn::keypad::a_pressed())
            {
                _confirm = 1;
                _choice = 1;
                _draw();
            }
            else if(_row == BACK && bn::keypad::a_pressed())
            {
                return _leave();
            }
            else
            {
                _change(_input.left() ? -1 : 1);
                _draw();
            }
        }

        if(bn::keypad::b_pressed())
        {
            return _leave();
        }

        _panel.commit();
        test_interface().menu_index = uint32_t(_row);
        test_interface().menu_count = rows;
        return bn::nullopt;
    }

private:
    enum Row
    {
        MUSIC_VOLUME, SOUND_VOLUME, MUSIC, SOUND, ANIMATIONS, TEXT_SPEED, BOARD, CONTRAST, EFFECTS, COORDINATES,
        ERASE, BACK, ROWS
    };

    static constexpr int rows = ROWS;

    bn::regular_bg_ptr _backdrop;
    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 100> _text;
    RepeatInput _input;
    ToastBanner _toast;
    int _row = 0;
    int _confirm = 0;       // 0 none, 1 first question, 2 second question
    int _choice = 1;
    bool _changed = false;

    bn::optional<SceneId> _leave()
    {
        if(_changed)
        {
            save_profile_now();
        }

        return SceneId::MAIN_MENU;
    }

    void _change(int delta)
    {
        SaveSettings& s = globals().profile.settings;
        _changed = true;

        switch(_row)
        {
        case MUSIC_VOLUME:
            s.music_volume = uint8_t(bn::max(0, bn::min(10, s.music_volume + delta)));
            break;

        case SOUND_VOLUME:
            s.sfx_volume = uint8_t(bn::max(0, bn::min(10, s.sfx_volume + delta)));
            break;

        case MUSIC:
            s.music_on = ! s.music_on;
            break;

        case SOUND:
            s.sfx_on = ! s.sfx_on;
            break;

        case ANIMATIONS:
            s.animations = uint8_t((s.animations + 4 + delta) % 4);
            break;

        case TEXT_SPEED:
            s.text_speed = uint8_t((s.text_speed + 4 + delta) % 4);
            break;

        case BOARD:
        {
            int theme = s.board_theme;

            for(int i = 0; i < board_themes_count; ++i)
            {
                theme = (theme + board_themes_count + delta) % board_themes_count;

                if(board_theme_unlocked(globals().profile.progress, theme))
                {
                    break;
                }
            }

            s.board_theme = uint8_t(theme);
            break;
        }

        case CONTRAST:
            s.high_contrast = ! s.high_contrast;
            break;

        case EFFECTS:
            s.reduced_effects = ! s.reduced_effects;
            break;

        case COORDINATES:
            s.show_coordinates = ! s.show_coordinates;
            break;

        default:
            break;
        }
    }

    bn::optional<SceneId> _update_confirm()
    {
        if(_input.up() || _input.down())
        {
            _choice ^= 1;
            _draw();
        }

        if(bn::keypad::a_pressed())
        {
            if(_choice == 0 && _confirm == 1)
            {
                _confirm = 2;
                _choice = 1;
                _draw();
            }
            else if(_choice == 0 && _confirm == 2)
            {
                erase_all_saves();
                Globals& g = globals();
                g.profile = Profile();
                g.toasts = 0;
                test_interface().suspend_present = 0;
                return SceneId::TITLE;
            }
            else
            {
                _confirm = 0;
                _draw();
            }
        }
        else if(bn::keypad::b_pressed())
        {
            _confirm = 0;
            _draw();
        }

        _panel.commit();
        return bn::nullopt;
    }

    void _draw()
    {
        _text.clear();
        _panel.clear(0);
        _panel.window(0, 0, 30, 20);
        const SaveSettings& s = globals().profile.settings;
        print_big(_text, 120, 6, "OPTIONS", TitleColor::GOLD, Align::CENTER);

        if(_confirm)
        {
            _panel.window(3, 6, 24, 9);
            print_big(_text, 120, 56, _confirm == 1 ? "ERASE ALL DATA?" : "ARE YOU SURE?", TitleColor::RUBY,
                      Align::CENTER);
            print(_text, 120, 76, _confirm == 1 ? "Progress, achievements and settings." :
                                                  "This cannot be undone.", TextColor::WHITE, Align::CENTER);
            constexpr const char* options[2] = { "ERASE", "KEEP" };

            for(int i = 0; i < 2; ++i)
            {
                print(_text, 120, 92 + i * 12, options[i], i == _choice ? TextColor::GOLD : TextColor::WHITE,
                      Align::CENTER);
            }

            print(_text, 84, 92 + _choice * 12, "→", TextColor::GOLD);
            return;
        }

        constexpr const char* labels[rows] = {
            "MUSIC VOLUME", "SOUND VOLUME", "MUSIC", "SOUND EFFECTS", "ANIMATIONS", "TEXT SPEED", "BOARD",
            "HIGH CONTRAST", "REDUCED EFFECTS", "COORDINATES", "ERASE SAVE DATA", "BACK"
        };
        constexpr const char* descriptions[rows] = {
            "Loudness of the music.", "Loudness of the sound effects.", "Play music.", "Play sound effects.",
            "Board animations and battle scenes.", "How fast dialogue text appears.",
            "Board for quick matches (more unlock in the story).", "Board with maximum square contrast.",
            "No flashes or screen shakes in cinematics.", "Show file and rank letters on the board.",
            "Delete all progress (asks twice).", "Return to the main menu."
        };
        constexpr const char* animation_names[] = { "FULL", "FAST", "MINIMAL", "OFF" };
        constexpr const char* speed_names[] = { "SLOW", "NORMAL", "FAST", "INSTANT" };

        for(int i = 0; i < rows; ++i)
        {
            const int y = 26 + i * 9;
            const bool current = i == _row;
            print(_text, 22, y, labels[i], current ? TextColor::GOLD : (i == ERASE ? TextColor::RED : TextColor::WHITE));
            bn::string<24> value;
            bn::ostringstream stream(value);

            switch(i)
            {
            case MUSIC_VOLUME: stream << int(s.music_volume) << " / 10"; break;
            case SOUND_VOLUME: stream << int(s.sfx_volume) << " / 10"; break;
            case MUSIC: stream << (s.music_on ? "ON" : "OFF"); break;
            case SOUND: stream << (s.sfx_on ? "ON" : "OFF"); break;
            case ANIMATIONS: stream << animation_names[s.animations & 3]; break;
            case TEXT_SPEED: stream << speed_names[s.text_speed & 3]; break;
            case BOARD: stream << theme_names[s.board_theme % board_themes_count]; break;
            case CONTRAST: stream << (s.high_contrast ? "ON" : "OFF"); break;
            case EFFECTS: stream << (s.reduced_effects ? "ON" : "OFF"); break;
            case COORDINATES: stream << (s.show_coordinates ? "ON" : "OFF"); break;
            default: break;
            }

            print(_text, 220, y, value, current ? TextColor::GOLD : TextColor::CYAN, Align::RIGHT);

            if(current)
            {
                print(_text, 10, y, "→", TextColor::GOLD);
            }
        }

        print(_text, 120, 136, descriptions[_row], TextColor::GRAY, Align::CENTER);
        print(_text, 120, 146, "◆ Left / Right to change   Ⓑ Back ◆", TextColor::GRAY, Align::CENTER);
    }
};

// ------------------------------------------------------------------------------------------------ credits

class CreditsScene final : public Scene
{
public:
    CreditsScene() :
        _backdrop(create_backdrop(Backdrop::TITLE)),
        _stars(ParticleKind::STARS, 14, 2, 11)
    {
        set_current_scene(SceneId::CREDITS);
        _panel.window(3, 0, 24, 20);
        _lines = credits_lines(_count);
        _total_height = _count * 14 + 160;
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        ++_frames;
        _stars.update();

        // The roll scrolls half a pixel per frame; A speeds it up, B or START leave.
        _offset += bn::keypad::a_held() ? 3 : 1;
        _draw();
        _panel.commit();

        if(bn::keypad::b_pressed() || bn::keypad::start_pressed() || _offset / 2 > _total_height)
        {
            return SceneId::MAIN_MENU;
        }

        test_interface().menu_index = uint32_t(_offset / 2);
        return bn::nullopt;
    }

private:
    bn::regular_bg_ptr _backdrop;
    ParticleField _stars;
    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 80> _text;
    const char* const* _lines = nullptr;
    int _count = 0;
    int _offset = 0;
    int _frames = 0;
    int _total_height = 0;

    void _draw()
    {
        // Only lines inside the screen get sprites.
        _text.clear();
        const int scroll = _offset / 2;

        for(int i = 0; i < _count; ++i)
        {
            const int y = 160 + i * 14 - scroll;

            if(y < -12 || y > 156)
            {
                continue;
            }

            const char* line = _lines[i];

            if(line[0] == '#')
            {
                print(_text, 120, y, line + 1, TextColor::GOLD, Align::CENTER);
            }
            else if(line[0])
            {
                print(_text, 120, y, line, TextColor::WHITE, Align::CENTER);
            }
        }
    }
};

// ------------------------------------------------------------------------------------------------ gallery

class GalleryScene final : public Scene
{
public:
    GalleryScene()
    {
        set_current_scene(SceneId::GALLERY);
        _open_section();
    }

    ~GalleryScene() override
    {
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        ++_frames;
        _input.update();
        _toast.update();

        if(bn::keypad::r_pressed() || bn::keypad::l_pressed())
        {
            const int delta = bn::keypad::r_pressed() ? 1 : -1;
            _section = GallerySection((int(_section) + 4 + delta) % 4);
            _index = 0;
            _open_section();
        }
        else if(_input.left() || _input.up() || _input.right() || _input.down())
        {
            const int size = gallery_section_size(_section);
            const int delta = (_input.left() || _input.up()) ? -1 : 1;
            _index = (_index + size + delta) % size;
            _open_section();
        }
        else if(bn::keypad::a_pressed() && _section == GallerySection::CHARACTERS && _unlocked)
        {
            _expression = (_expression + 1) % 3;

            if(_portrait)
            {
                _portrait->set_tiles(portrait_item(Character(_index)).tiles_item(), _expression);
            }
        }

        if(_section == GallerySection::SPELLS && _unlocked && _demo_board && (_frames % 90) == 0)
        {
            // Alternate between the position before and after the spell.
            _demo_after = ! _demo_after;
            _show_demo();
        }

        if(bn::keypad::b_pressed())
        {
            return SceneId::MAIN_MENU;
        }

        _panel.commit();
        test_interface().menu_index = uint32_t(int(_section) * 16 + _index);
        test_interface().gallery_unlocked = uint32_t(gallery_unlocked_count(globals().profile));
        return bn::nullopt;
    }

private:
    bn::optional<bn::regular_bg_ptr> _backdrop;
    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 110> _text;
    bn::optional<bn::sprite_ptr> _portrait;
    bn::optional<MiniBoard> _demo_board;
    RepeatInput _input;
    ToastBanner _toast;
    GallerySection _section = GallerySection::CHARACTERS;
    int _index = 0;
    int _frames = 0;
    int _expression = 0;
    bool _unlocked = false;
    bool _demo_after = false;

    void _header()
    {
        constexpr const char* names[4] = { "CHARACTERS", "KINGDOMS", "MUSIC", "SPELLS" };
        bn::string<40> title;
        bn::ostringstream stream(title);
        stream << "Ⓛ " << names[int(_section)] << " Ⓡ";
        print(_text, 120, 4, title, TextColor::GOLD, Align::CENTER);
        bn::string<24> count;
        bn::ostringstream count_stream(count);
        count_stream << gallery_unlocked_count(globals().profile) << " / " << gallery_entries;
        print(_text, 236, 4, count, TextColor::CYAN, Align::RIGHT);
        bn::string<16> position;
        bn::ostringstream position_stream(position);
        position_stream << "◇ " << (_index + 1) << "/" << gallery_section_size(_section);
        print(_text, 4, 4, position, TextColor::GRAY);
    }

    void _open_section()
    {
        const Profile& profile = globals().profile;
        _text.clear();
        _portrait.reset();
        _demo_board.reset();
        _panel.clear(0);
        _expression = 0;
        _unlocked = gallery_unlocked(profile, _section, _index);
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);

        switch(_section)
        {
        case GallerySection::KINGDOMS:
            _backdrop = create_backdrop(world_backdrop(_index));

            if(! _unlocked)
            {
                bn::bg_palettes::set_fade(bn::color(0, 0, 0), bn::fixed(0.85));
            }

            _panel.window(0, 0, 30, 3);
            _panel.window(0, 12, 30, 8);
            _header();
            print_big(_text, 120, 102, _unlocked ? world(_index).name : "???", TitleColor::GOLD, Align::CENTER);
            print_wrapped(_text, 120, 122, _unlocked ? kingdom_lore(_index) :
                                                      gallery_unlock_hint(_section, _index),
                          layout::card_text_width, 3, _unlocked ? TextColor::WHITE : TextColor::GRAY, 10,
                          Align::CENTER);
            break;

        case GallerySection::CHARACTERS:
        {
            _backdrop = create_backdrop(Backdrop::TITLE);
            _panel.window(0, 0, 30, 20);
            _header();
            const Character character = Character(_index);
            _portrait = portrait_item(character).create_sprite(16 + 32 - 120, 22 + 32 - 80, 0);
            _portrait->set_bg_priority(0);

            if(! _unlocked)
            {
                // A silhouette: the portrait's palette faded to ink.
                bn::sprite_palette_ptr palette = _portrait->palette();
                palette.set_fade(bn::color(2, 2, 6), 1);
            }
            else
            {
                bn::sprite_palette_ptr palette = _portrait->palette();
                palette.set_fade(bn::color(0, 0, 0), 0);
            }

            print_big(_text, 90, 30, _unlocked ? character_name(character) : "???", TitleColor::GOLD);
            print(_text, 90, 50, _unlocked ? character_title(character) : "Not met yet", TextColor::CYAN);

            if(_unlocked)
            {
                print(_text, 90, 64, "Ⓐ Expression", TextColor::GRAY);
            }

            print_wrapped(_text, 16, 94, _unlocked ? character_lore(character) :
                                                    gallery_unlock_hint(_section, _index),
                          layout::card_text_width, 5, _unlocked ? TextColor::WHITE : TextColor::GRAY);
            break;
        }

        case GallerySection::MUSIC:
            _backdrop = create_backdrop(Backdrop::TITLE);
            _panel.window(0, 0, 30, 20);
            _header();
            print_big(_text, 120, 20, "SOUNDTRACK", TitleColor::GOLD, Align::CENTER);

            for(int i = 0; i < gallery_music; ++i)
            {
                const bool open = gallery_unlocked(profile, GallerySection::MUSIC, i);
                const bool current = i == _index;
                const int y = 40 + i * 8;
                bn::string<40> line;
                bn::ostringstream stream(line);
                stream << (i + 1) << ". " << (open ? music_title(i) : "???");
                print(_text, 30, y, line, current ? TextColor::GOLD : (open ? TextColor::WHITE : TextColor::GRAY));

                if(current)
                {
                    print(_text, 18, y, "→", TextColor::GOLD);
                }
            }

            if(! _unlocked)
            {
                print(_text, 120, 148, gallery_unlock_hint(_section, _index), TextColor::GRAY, Align::CENTER);
            }
            break;

        default:
        {
            _backdrop = create_backdrop(Backdrop::UMBRAL);
            _panel.window(0, 0, 30, 20);
            _header();
            const Ability ability = Ability(_index);
            print_big(_text, 120, 18, ability_name(ability), TitleColor::ARCANE, Align::CENTER);

            if(_unlocked)
            {
                print_wrapped(_text, 92, 40, ability_summary(ability), 136, 2, TextColor::CYAN);
                print_wrapped(_text, 16, 108, ability_rule_text(ability), layout::card_text_width, 4,
                              TextColor::WHITE);
                _demo_board.emplace(16, 38);
                _demo_after = false;
                _show_demo();
            }
            else
            {
                print_wrapped(_text, 120, 70, gallery_unlock_hint(_section, _index), 200, 2, TextColor::GRAY, 10,
                              Align::CENTER);
            }
            break;
        }
        }
    }

    void _show_demo()
    {
        const SpellDemo& demo = spell_demo(Ability(_index));
        Game& game = globals().game;
        RuleSet rules = RuleSet::arcane_rules();

        if(game.setup(demo.fen, rules, false) != FenError::NONE)
        {
            return;
        }

        if(_demo_after)
        {
            Position pos = game.position();
            const Move move = parse_coordinate(pos, demo.action);

            if(! move.is_null())
            {
                game.apply(move);
            }
        }

        _demo_board->show(game.position());
    }
};

}

bn::unique_ptr<Scene> make_achievements_scene()
{
    return bn::unique_ptr<Scene>(new AchievementsScene());
}

bn::unique_ptr<Scene> make_options_scene()
{
    return bn::unique_ptr<Scene>(new OptionsScene());
}

bn::unique_ptr<Scene> make_credits_scene()
{
    return bn::unique_ptr<Scene>(new CreditsScene());
}

bn::unique_ptr<Scene> make_gallery_scene()
{
    return bn::unique_ptr<Scene>(new GalleryScene());
}

}
