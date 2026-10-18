// CHESS LEGENDS: ARCANE WARS - challenge list and boss gauntlet scenes.
#include "bn_keypad.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "cl_art.h"
#include "cl_audio.h"
#include "cl_layers.h"
#include "cl_progress.h"
#include "cl_scenes.h"
#include "cl_text.h"
#include "cl_text_layout.h"

#include "bn_sprite_items_map_marker.h"

namespace cl::gba
{

namespace
{
    const char* medal_text(int medal)
    {
        constexpr const char* texts[4] = { "-", "★", "★★", "★★★" };
        return texts[medal & 3];
    }

    const char* objective_summary(const ObjectiveSpec& spec, int par)
    {
        static bn::string<32> text;
        text.clear();
        bn::ostringstream stream(text);

        switch(spec.type)
        {
        case Objective::CHECKMATE_WITHIN: stream << "Mate in " << par; break;
        case Objective::SURVIVE: stream << "Survive " << int(spec.limit) << " moves"; break;
        case Objective::WIN_MATERIAL: stream << "Win " << int(spec.value) << "+ pawns"; break;
        case Objective::PROMOTE: stream << "Promote in " << par; break;
        default: stream << "Win"; break;
        }

        return text.c_str();
    }

// ------------------------------------------------------------------------------------------------ challenges

class ChallengesScene final : public Scene
{
public:
    ChallengesScene() :
        _backdrop(create_backdrop(Backdrop::UMBRAL)),
        _preview(164, 40)
    {
        set_current_scene(SceneId::CHALLENGES);
        play_music(Music::UMBRAL);
        Globals& g = globals();
        _index = g.setup.challenge >= 0 && g.setup.challenge < challenges_count ? g.setup.challenge : 0;

        // Start on the first challenge without a gold medal.
        if(g.setup.challenge < 0)
        {
            for(int i = 0; i < challenges_count; ++i)
            {
                if(challenge_medal_of(g.profile.progress, i) < 3)
                {
                    _index = i;
                    break;
                }
            }
        }

        _panel.window(0, 0, 30, 4);
        _panel.window(0, 4, 19, 16);
        _panel.window(19, 4, 11, 16);
        _scroll_to_index();
        _draw();
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        _input.update();
        _toast.update();

        if(_input.up())
        {
            play_sound(Sound::MENU_MOVE);
            _index = (_index + challenges_count - 1) % challenges_count;
            _scroll_to_index();
            _draw();
        }
        else if(_input.down())
        {
            play_sound(Sound::MENU_MOVE);
            _index = (_index + 1) % challenges_count;
            _scroll_to_index();
            _draw();
        }

        if(bn::keypad::a_pressed())
        {
            MatchSetup& setup = globals().setup;
            setup.mode = MatchMode::CHALLENGE;
            setup.challenge = int8_t(_index);
            setup.encounter = -1;
            setup.gauntlet_stage = -1;
            setup.resume_suspended = false;
            return SceneId::MATCH;
        }

        if(bn::keypad::b_pressed())
        {
            return SceneId::MAIN_MENU;
        }

        _panel.commit();
        test_interface().menu_index = uint32_t(_index);
        test_interface().menu_count = challenges_count;
        test_interface().challenge_medals = globals().profile.progress.challenge_medals;
        return bn::nullopt;
    }

private:
    static constexpr int visible_rows = 10;

    bn::regular_bg_ptr _backdrop;
    PanelLayer _panel;
    MiniBoard _preview;
    bn::vector<bn::sprite_ptr, 96> _text;
    RepeatInput _input;
    ToastBanner _toast;
    Game& _game = globals().game;      // free while no match is running: holds the previewed position
    int _index = 0;
    int _first = 0;

    void _scroll_to_index()
    {
        if(_index < _first)
        {
            _first = _index;
        }
        else if(_index >= _first + visible_rows)
        {
            _first = _index - visible_rows + 1;
        }
    }

    void _draw()
    {
        const SaveProgress& progress = globals().profile.progress;
        _text.clear();
        print_big(_text, 12, 8, "CHALLENGES", TitleColor::GOLD);
        int stars = 0;

        for(int i = 0; i < challenges_count; ++i)
        {
            stars += challenge_medal_of(progress, i);
        }

        bn::string<24> total;
        bn::ostringstream stream(total);
        stream << "★ " << stars << " / " << challenges_count * 3;
        print(_text, 228, 12, total, TextColor::GOLD, Align::RIGHT);

        for(int row = 0; row < visible_rows && _first + row < challenges_count; ++row)
        {
            const int i = _first + row;
            const Challenge& c = challenge(i);
            const int y = 40 + row * 11;
            const bool current = i == _index;
            bn::string<8> number;
            bn::ostringstream number_stream(number);
            number_stream << (i + 1);
            print(_text, 24, y, number, TextColor::GRAY, Align::RIGHT);
            print(_text, 30, y, c.title, current ? TextColor::GOLD : TextColor::WHITE);
            const int medal = challenge_medal_of(progress, i);
            print(_text, 146, y, medal_text(medal), medal ? TextColor::GOLD : TextColor::GRAY, Align::RIGHT);

            if(current)
            {
                print(_text, 2, y, "→", TextColor::GOLD);
            }
        }

        // More entries above / below the visible rows.
        if(_first > 0)
        {
            print(_text, 140, 33, "…", TextColor::GRAY);
        }

        if(_first + visible_rows < challenges_count)
        {
            print(_text, 140, 148, "…", TextColor::GRAY);
        }

        // Preview of the selected challenge: the real position, its theme and goal.
        const Challenge& c = challenge(_index);
        [[maybe_unused]] const bool ok = setup_challenge(c, _game);
        _preview.show(_game.position());
        TextSpan spans[2];
        const int lines = wrap_lines(FontId::SMALL, c.theme, bn::string_view(c.theme).size(), 80, spans, 2);

        for(int i = 0; i < lines; ++i)
        {
            print(_text, 196, 108 + i * 10, bn::string_view(c.theme + spans[i].start, spans[i].length),
                  TextColor::CYAN, Align::CENTER);
        }

        print(_text, 196, 128, objective_summary(c.objective, c.par), TextColor::WHITE, Align::CENTER);
        print(_text, 196, 138, c.arcane ? "Arcane" : "Classic", TextColor::GRAY, Align::CENTER);
        print(_text, 196, 149, "Ⓐ Play", TextColor::GOLD, Align::CENTER);
    }
};

// ------------------------------------------------------------------------------------------------ gauntlet

class GauntletScene final : public Scene
{
public:
    GauntletScene() :
        _g(globals())
    {
        set_current_scene(SceneId::GAUNTLET);
        play_music(Music::BOSS);
        MatchSetup& setup = _g.setup;

        if(setup.gauntlet_stage < 0)
        {
            _mode = Mode::INTRO;
            _stage = 0;
        }
        else if(! setup.result_ready || setup.mode != MatchMode::GAUNTLET)
        {
            _mode = Mode::ABANDONED;
            _stage = setup.gauntlet_stage;
        }
        else if(setup.last_success)
        {
            _stage = setup.gauntlet_stage + 1;
            _mode = _stage >= gauntlet_stages ? Mode::COMPLETE : Mode::NEXT;
        }
        else
        {
            _mode = Mode::DEFEAT;
            _stage = setup.gauntlet_stage;
        }

        setup.result_ready = false;
        const int art_world = _mode == Mode::NEXT ? _stage : (_mode == Mode::INTRO ? -1 : _stage % worlds_count);
        _backdrop = create_backdrop(_mode == Mode::COMPLETE ? Backdrop::NEXUS : world_backdrop(art_world));

        if(_mode == Mode::COMPLETE)
        {
            _particles.emplace(ParticleKind::FRAGMENTS, 6, 1, 3);
            _particles->set_center(120, 48);
        }

        _draw();
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        _input.update();
        _toast.update();

        if(_particles)
        {
            _particles->update();
        }

        if(bn::keypad::a_pressed())
        {
            if(_mode == Mode::INTRO || _mode == Mode::NEXT)
            {
                MatchSetup& setup = _g.setup;
                setup.mode = MatchMode::GAUNTLET;
                setup.gauntlet_stage = int8_t(_stage);
                setup.encounter = -1;
                setup.challenge = -1;
                setup.resume_suspended = false;
                return SceneId::MATCH;
            }

            _g.setup.gauntlet_stage = -1;
            return SceneId::MAIN_MENU;
        }

        if(bn::keypad::b_pressed())
        {
            _g.setup.gauntlet_stage = -1;
            return SceneId::MAIN_MENU;
        }

        _panel.commit();
        test_interface().gauntlet_stage = uint32_t(_stage);
        test_interface().menu_index = uint32_t(_mode);
        return bn::nullopt;
    }

private:
    enum class Mode : uint8_t
    {
        INTRO,          // a new run
        NEXT,           // the previous ruler fell: the next one waits
        COMPLETE,       // all six defeated
        DEFEAT,         // the run ended in a defeat
        ABANDONED       // the last battle was quit
    };

    Globals& _g;
    bn::optional<bn::regular_bg_ptr> _backdrop;
    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 96> _text;
    bn::vector<bn::sprite_ptr, 6> _markers;
    bn::optional<bn::sprite_ptr> _portrait;
    bn::optional<ParticleField> _particles;
    RepeatInput _input;
    ToastBanner _toast;
    Mode _mode = Mode::INTRO;
    int _stage = 0;

    void _draw_track(int y)
    {
        // Six banners: defeated rulers in gold, the next one open, the rest sealed.
        for(int i = 0; i < gauntlet_stages; ++i)
        {
            const bool defeated = i < _stage || _mode == Mode::COMPLETE;
            const int frame = defeated ? 1 : (i == _stage ? 0 : 2);
            bn::sprite_ptr marker = bn::sprite_items::map_marker.create_sprite(60 + i * 24 - 120, y - 80, frame);
            marker.set_bg_priority(0);
            _markers.push_back(bn::move(marker));
        }
    }

    void _draw()
    {
        const SaveProgress& progress = _g.profile.progress;
        _text.clear();
        _markers.clear();
        _portrait.reset();
        _panel.clear(0);
        _panel.window(1, 1, 28, 18);
        bn::string<40> best;
        bn::ostringstream best_stream(best);
        best_stream << "Best run: " << int(progress.gauntlet_best) << " / " << gauntlet_stages;

        switch(_mode)
        {
        case Mode::INTRO:
            print_big(_text, 120, 16, "BOSS GAUNTLET", TitleColor::GOLD, Align::CENTER);
            _draw_track(48);
            print(_text, 120, 66, "Face the six rulers, one after another.", TextColor::WHITE, Align::CENTER);
            print(_text, 120, 78, "Each one chooses the opening,", TextColor::WHITE, Align::CENTER);
            print(_text, 120, 90, "and each is stronger than the last.", TextColor::WHITE, Align::CENTER);
            print(_text, 120, 102, "A single defeat ends the run.", TextColor::RED, Align::CENTER);
            print(_text, 120, 118, best, progress.gauntlet_cleared ? TextColor::GOLD : TextColor::GRAY, Align::CENTER);
            print(_text, 120, 140, "Ⓐ Begin   Ⓑ Back", TextColor::GOLD, Align::CENTER);
            break;

        case Mode::NEXT:
        {
            const GauntletStage& stage = gauntlet_stage(_stage);
            bn::string<24> header;
            bn::ostringstream header_stream(header);
            header_stream << "RULER " << (_stage + 1) << " OF " << gauntlet_stages;
            print(_text, 120, 14, header, TextColor::CYAN, Align::CENTER);
            _draw_track(30);
            _portrait = portrait_item(stage.boss).create_sprite(16 + 32 - 120, 44 + 32 - 80, int(Expression::INTENSE));
            _portrait->set_bg_priority(0);
            _portrait->set_horizontal_flip(true);
            print_big(_text, 92, 46, character_name(stage.boss), TitleColor::GOLD);
            print(_text, 92, 64, character_title(stage.boss), TextColor::WHITE);
            print(_text, 92, 78, stage.opening, TextColor::CYAN);
            bn::string<48> level;
            bn::ostringstream level_stream(level);
            level_stream << difficulty_name(stage.difficulty) << ", " << personality_name(stage.personality);
            print(_text, 92, 90, level, TextColor::GRAY);
            TextSpan spans[layout::dialogue_lines];
            const int lines = wrap_lines(FontId::SMALL, stage.taunt, bn::string_view(stage.taunt).size(),
                                         layout::dialogue_text_width, spans, layout::dialogue_lines);

            for(int i = 0; i < lines; ++i)
            {
                print(_text, 120, 114 + i * 10, bn::string_view(stage.taunt + spans[i].start, spans[i].length),
                      TextColor::WHITE, Align::CENTER);
            }

            print(_text, 120, 140, "Ⓐ Fight   Ⓑ Abandon the run", TextColor::GOLD, Align::CENTER);
            break;
        }

        case Mode::COMPLETE:
            print_big(_text, 120, 70, "GAUNTLET COMPLETE", TitleColor::GOLD, Align::CENTER);
            _draw_track(100);
            print(_text, 120, 116, "All six rulers fell in a single run.", TextColor::WHITE, Align::CENTER);
            print(_text, 120, 140, "Ⓐ Continue", TextColor::GOLD, Align::CENTER);
            break;

        case Mode::DEFEAT:
        case Mode::ABANDONED:
        {
            print_big(_text, 120, 40, _mode == Mode::DEFEAT ? "THE RUN ENDS" : "RUN ABANDONED", TitleColor::RUBY,
                      Align::CENTER);
            _draw_track(70);
            bn::string<48> line;
            bn::ostringstream line_stream(line);
            line_stream << "Rulers defeated: " << _stage << " / " << gauntlet_stages;
            print(_text, 120, 90, line, TextColor::WHITE, Align::CENTER);
            print(_text, 120, 102, best, TextColor::GRAY, Align::CENTER);
            print(_text, 120, 140, "Ⓐ Continue", TextColor::GOLD, Align::CENTER);
            break;
        }

        default:
            break;
        }
    }
};

}

bn::unique_ptr<Scene> make_challenges_scene()
{
    return bn::unique_ptr<Scene>(new ChallengesScene());
}

bn::unique_ptr<Scene> make_gauntlet_scene()
{
    return bn::unique_ptr<Scene>(new GauntletScene());
}

}
