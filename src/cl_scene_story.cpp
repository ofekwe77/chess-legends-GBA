// CHESS LEGENDS: ARCANE WARS - story map, dialogue, boss introductions, briefings and campaign rewards.
#include "bn_bg_palettes.h"
#include "bn_blending.h"
#include "bn_color.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_double_size_mode.h"
#include "bn_sprite_palettes.h"
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

#include "bn_sprite_items_board_cursor.h"
#include "bn_sprite_items_map_marker.h"

namespace cl::gba
{

namespace
{
    // Kingdom positions on the world map (pixels), matching the regions drawn by gen_backdrops.py.
    constexpr int marker_x[worlds_count] = { 40, 98, 162, 80, 206, 136 };
    constexpr int marker_y[worlds_count] = { 76, 42, 42, 90, 76, 72 };

    constexpr const char* type_label(EncounterType type)
    {
        return type == EncounterType::BOSS ? "BOSS" : (type == EncounterType::LESSON ? "LESSON" : "TRIAL");
    }

    void reset_fades()
    {
        bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);
        bn::sprite_palettes::set_fade(bn::color(0, 0, 0), 0);
        bn::blending::set_transparency_alpha(1);
    }

    void set_fades(bn::color color, bn::fixed intensity)
    {
        bn::bg_palettes::set_fade(color, intensity);
        bn::sprite_palettes::set_fade(color, intensity);
    }

    bool world_restored(const SaveProgress& progress, int w)
    {
        return encounter_cleared(progress, w * 3 + 2);
    }

    bool world_open(const SaveProgress& progress, int w)
    {
        return encounter_unlocked(progress, w * 3);
    }

// ------------------------------------------------------------------------------------------------ story map

class StoryMapScene final : public Scene
{
public:
    StoryMapScene() :
        _backdrop(create_backdrop(Backdrop::MAP))
    {
        set_current_scene(SceneId::STORY_MAP);
        Globals& g = globals();
        const SaveProgress& progress = g.profile.progress;

        // First visit: the prologue; a newly opened kingdom: its introduction. Both play before the map.
        if(! (progress.worlds_seen & 1))
        {
            g.story.step = StoryStep::PROLOGUE;
            g.story.world = 0;
            _pending = SceneId::STORY_DIALOGUE;
            return;
        }

        if(! ((progress.worlds_seen >> progress.story_world) & 1))
        {
            g.story.step = StoryStep::WORLD_INTRO;
            g.story.world = int8_t(progress.story_world);
            _pending = SceneId::STORY_DIALOGUE;
            return;
        }

        play_music(Music::WORLD_MAP);

        for(int w = 0; w < worlds_count; ++w)
        {
            const int frame = world_restored(progress, w) ? 1 : (world_open(progress, w) ? 0 : 2);
            bn::sprite_ptr marker = bn::sprite_items::map_marker.create_sprite(marker_x[w] - 120, marker_y[w] - 80,
                                                                                frame);
            marker.set_bg_priority(2);     // above the map, below the windows
            _markers.push_back(bn::move(marker));
        }

        _world = g.story.world >= 0 && g.story.world < worlds_count && world_open(progress, g.story.world) ?
                     g.story.world : progress.story_world;
        _cursor = bn::sprite_items::board_cursor.create_sprite(0, 0, 0);
        _cursor->set_bg_priority(2);
        print(_header, 8, 4, "THE SIX KINGDOMS", TextColor::GOLD);
        print(_header, 232, 4, "Ⓑ Menu", TextColor::DARK, Align::RIGHT);
        _place_cursor();
        _draw_info();
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        if(_pending)
        {
            return _pending;
        }

        ++_frames;
        _input.update();
        _toast.update();

        if(_cursor)
        {
            _cursor->set_tiles(bn::sprite_items::board_cursor.tiles_item(), (_frames >> 3) & 3);
        }

        bn::optional<SceneId> next = _list_open ? _update_list() : _update_map();
        _panel.commit();
        test_interface().menu_index = uint32_t(_list_open ? 16 + _row : _world);
        test_interface().unlocked_worlds = uint32_t(globals().profile.progress.story_world + 1);
        test_interface().campaign_cleared = globals().profile.progress.encounters_cleared;
        return next;
    }

private:
    bn::regular_bg_ptr _backdrop;
    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 6> _markers;
    bn::optional<bn::sprite_ptr> _cursor;
    bn::vector<bn::sprite_ptr, 12> _header;
    bn::vector<bn::sprite_ptr, 64> _text;
    RepeatInput _input;
    ToastBanner _toast;
    bn::optional<SceneId> _pending;
    int _world = 0;
    int _row = 0;
    int _frames = 0;
    int _message_frames = 0;
    bool _list_open = false;

    void _place_cursor()
    {
        _cursor->set_position(marker_x[_world] - 120, marker_y[_world] - 80);
    }

    bn::optional<SceneId> _update_map()
    {
        if(_input.left() || _input.up())
        {
            play_sound(Sound::MENU_MOVE);
            _world = (_world + worlds_count - 1) % worlds_count;
            _place_cursor();
            _draw_info();
        }
        else if(_input.right() || _input.down())
        {
            play_sound(Sound::MENU_MOVE);
            _world = (_world + 1) % worlds_count;
            _place_cursor();
            _draw_info();
        }

        if(_message_frames > 0 && --_message_frames == 0)
        {
            _draw_info();
        }

        if(bn::keypad::a_pressed())
        {
            if(world_open(globals().profile.progress, _world))
            {
                play_sound(Sound::MENU_SELECT);
                _open_list();
            }
            else
            {
                play_sound(Sound::INVALID);
                _message_frames = 120;
                _draw_info();
            }
        }
        else if(bn::keypad::b_pressed())
        {
            play_sound(Sound::MENU_CANCEL);
            return SceneId::MAIN_MENU;
        }

        return bn::nullopt;
    }

    void _open_list()
    {
        _list_open = true;
        const SaveProgress& progress = globals().profile.progress;
        _row = 0;

        // Start on the first encounter not cleared yet.
        for(int i = 0; i < 3; ++i)
        {
            if(! encounter_cleared(progress, _world * 3 + i))
            {
                _row = i;
                break;
            }
        }

        _draw_list();
    }

    bn::optional<SceneId> _update_list()
    {
        if(_input.up())
        {
            play_sound(Sound::MENU_MOVE);
            _row = (_row + 2) % 3;
            _draw_list();
        }
        else if(_input.down())
        {
            play_sound(Sound::MENU_MOVE);
            _row = (_row + 1) % 3;
            _draw_list();
        }

        if(bn::keypad::a_pressed())
        {
            const int index = _world * 3 + _row;

            if(encounter_unlocked(globals().profile.progress, index))
            {
                play_sound(Sound::MENU_SELECT);
                Globals& g = globals();
                g.story.step = StoryStep::BRIEFING;
                g.story.world = int8_t(_world);
                g.story.encounter = int8_t(index);
                return SceneId::STORY_DIALOGUE;
            }
        }
        else if(bn::keypad::b_pressed())
        {
            play_sound(Sound::MENU_CANCEL);
            _list_open = false;
            _draw_info();
        }

        return bn::nullopt;
    }

    void _draw_info()
    {
        _text.clear();
        _panel.clear(0);
        _panel.window(1, 14, 28, 5);
        const World& w = world(_world);
        const SaveProgress& progress = globals().profile.progress;
        const bool open = world_open(progress, _world);
        print_big(_text, 16, 117, open ? w.name : "???", open ? TitleColor::GOLD : TitleColor::DIM);

        if(_message_frames > 0)
        {
            print(_text, 16, 136, "Restore the previous kingdom first.", TextColor::RED);
            return;
        }

        if(! open)
        {
            print(_text, 16, 136, "A kingdom still sealed by the Arcane Wars.", TextColor::GRAY);
            return;
        }

        print(_text, 16, 136, w.subtitle, TextColor::WHITE);
        int cleared = 0;

        for(int i = 0; i < 3; ++i)
        {
            cleared += encounter_cleared(progress, _world * 3 + i);
        }

        bn::string<16> status;
        bn::ostringstream stream(status);

        if(world_restored(progress, _world))
        {
            stream << "★ Restored";
        }
        else
        {
            stream << cleared << " / 3";
        }

        print(_text, 224, 136, status, world_restored(progress, _world) ? TextColor::GOLD : TextColor::CYAN,
              Align::RIGHT);
    }

    void _draw_list()
    {
        _text.clear();
        _panel.clear(0);
        _panel.window(1, 8, 28, 11);
        const SaveProgress& progress = globals().profile.progress;
        print_big(_text, 120, 70, world(_world).name, TitleColor::GOLD, Align::CENTER);

        for(int i = 0; i < 3; ++i)
        {
            const int index = _world * 3 + i;
            const Encounter& e = encounter(index);
            const bool unlocked = encounter_unlocked(progress, index);
            const bool cleared = encounter_cleared(progress, index);
            const bool current = i == _row;
            const int y = 92 + i * 15;
            const TextColor type_color = e.type == EncounterType::BOSS ? TextColor::RED : TextColor::CYAN;
            print(_text, 22, y, type_label(e.type), unlocked ? type_color : TextColor::GRAY);
            print(_text, 66, y, unlocked ? e.title : "Locked", current && unlocked ? TextColor::GOLD :
                                                                 (unlocked ? TextColor::WHITE : TextColor::GRAY));
            bn::string<20> status;
            bn::ostringstream stream(status);

            if(cleared)
            {
                stream << "★ " << int(progress.encounter_best[index]) << " mv";
            }
            else if(unlocked)
            {
                stream << "New";
            }

            print(_text, 222, y, status, cleared ? TextColor::GOLD : TextColor::CYAN, Align::RIGHT);

            if(current)
            {
                print(_text, 12, y, "→", TextColor::GOLD);
            }
        }

        print(_text, 120, 138, "Ⓐ Begin   Ⓑ Map", TextColor::GRAY, Align::CENTER);
    }
};

// ------------------------------------------------------------------------------------------------ dialogue

enum class Segment : uint8_t
{
    BOSS_INTRO,
    DIALOGUE,
    BRIEFING_CARD,
    REWARD_CARD,
    RETRY_CARD
};

struct Step
{
    Segment segment;
    const Dialogue* dialogue;
};

class StoryDialogueScene final : public Scene
{
public:
    StoryDialogueScene() :
        _g(globals()),
        _flow(globals().story),
        _backdrop(create_backdrop(_choose_backdrop()))
    {
        set_current_scene(SceneId::STORY_DIALOGUE);
        _build();

        switch(_flow.step)
        {
        case StoryStep::PROLOGUE: play_music(Music::TITLE); break;
        case StoryStep::EPILOGUE: play_music(Music::ENDING); break;
        case StoryStep::BRIEFING:
            play_music(encounter(_flow.encounter).type == EncounterType::BOSS ? Music::BOSS :
                                                                               world_music(_flow.world));
            break;
        default:
            if(_flow.step == StoryStep::VICTORY && _flow.encounter == campaign_encounters - 1)
            {
                play_music(Music::ENDING);
            }
            else
            {
                play_music(world_music(_flow.world));
            }
            break;
        }

        _begin_step();
    }

    ~StoryDialogueScene() override
    {
        reset_fades();
        test_interface().dialogue_active = 0;
        test_interface().cinematic = 0;
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        ++_frames;
        _input.update();

        if(_particles)
        {
            _particles->update();
        }

        switch(_steps[_index].segment)
        {
        case Segment::BOSS_INTRO:
            _update_boss_intro();
            break;

        case Segment::DIALOGUE:
            _update_dialogue();
            break;

        case Segment::BRIEFING_CARD:
            _update_briefing();
            break;

        case Segment::REWARD_CARD:
            _update_reward();
            break;

        default:
            _update_retry();
            break;
        }

        _toast.update();
        _panel.commit();
        test_interface().story_step = uint32_t(_flow.step);
        test_interface().encounter = uint32_t(int32_t(_flow.encounter));
        return _next;
    }

private:
    Globals& _g;
    StoryFlow _flow;
    bn::regular_bg_ptr _backdrop;
    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 72> _text;
    bn::vector<bn::sprite_ptr, 12> _name_text;
    bn::vector<bn::sprite_ptr, 2> _continue;
    bn::optional<bn::sprite_ptr> _portrait;
    bn::optional<ParticleField> _particles;
    RepeatInput _input;
    ToastBanner _toast;
    Step _steps[6];
    int _count = 0;
    int _index = 0;
    int _frames = 0;
    int _step_frames = 0;
    int _line = 0;
    int _revealed = 0;
    int _menu = 0;
    Character _speaker = Character::NARRATOR;
    bool _line_complete = false;
    bn::optional<SceneId> _next;
    SceneId _after = SceneId::STORY_MAP;

    // ---------------------------------------------------------------- setup

    Backdrop _choose_backdrop() const
    {
        const StoryFlow& flow = globals().story;

        if(flow.step == StoryStep::PROLOGUE)
        {
            return Backdrop::TITLE;
        }

        if(flow.step == StoryStep::EPILOGUE)
        {
            return Backdrop::NEXUS;
        }

        return world_backdrop(flow.world);
    }

    void _push(Segment segment, const Dialogue* dialogue = nullptr)
    {
        CL_ASSERT(_count < 6);
        _steps[_count++] = Step{ segment, dialogue };
    }

    void _build()
    {
        switch(_flow.step)
        {
        case StoryStep::PROLOGUE:
            _push(Segment::DIALOGUE, &prologue());
            _push(Segment::DIALOGUE, &world(0).introduction);
            _after = SceneId::STORY_MAP;
            break;

        case StoryStep::WORLD_INTRO:
            _push(Segment::DIALOGUE, &world(_flow.world).introduction);
            _after = SceneId::STORY_MAP;
            break;

        case StoryStep::BRIEFING:
        {
            const Encounter& e = encounter(_flow.encounter);

            if(e.type == EncounterType::BOSS)
            {
                _push(Segment::BOSS_INTRO);
            }

            _push(Segment::DIALOGUE, &e.intro);
            _push(Segment::BRIEFING_CARD);
            _after = SceneId::MATCH;
            break;
        }

        case StoryStep::VICTORY:
        {
            const Encounter& e = encounter(_flow.encounter);
            _push(Segment::DIALOGUE, &e.victory);

            if(e.type == EncounterType::BOSS)
            {
                _push(Segment::REWARD_CARD);
            }

            if(_flow.encounter == campaign_encounters - 1)
            {
                _push(Segment::DIALOGUE, &epilogue());
                _after = SceneId::ENDING;
            }
            else
            {
                _after = SceneId::STORY_MAP;
            }
            break;
        }

        case StoryStep::DEFEAT:
            _push(Segment::DIALOGUE, &encounter(_flow.encounter).defeat);
            _push(Segment::RETRY_CARD);
            _after = SceneId::STORY_MAP;
            break;

        default:
            _push(Segment::DIALOGUE, &epilogue());
            _after = SceneId::ENDING;
            break;
        }
    }

    void _finish()
    {
        SaveProgress& progress = _g.profile.progress;

        if(_flow.step == StoryStep::PROLOGUE)
        {
            progress.worlds_seen |= 1;
            save_profile_now();
        }
        else if(_flow.step == StoryStep::WORLD_INTRO)
        {
            progress.worlds_seen |= uint8_t(1 << _flow.world);
            save_profile_now();
        }

        if(_after == SceneId::MATCH)
        {
            MatchSetup& setup = _g.setup;
            setup.mode = MatchMode::STORY;
            setup.encounter = _flow.encounter;
            setup.challenge = -1;
            setup.gauntlet_stage = -1;
            setup.resume_suspended = false;
        }

        // Back on the map, the cursor stays on this kingdom.
        _g.story.world = _flow.world;
        _next = _after;
    }

    void _advance()
    {
        if(++_index >= _count)
        {
            _finish();
            return;
        }

        _begin_step();
    }

    void _begin_step()
    {
        _step_frames = 0;
        _text.clear();
        _name_text.clear();
        _continue.clear();
        _portrait.reset();
        _particles.reset();
        _panel.clear(0);
        reset_fades();
        _menu = 0;

        switch(_steps[_index].segment)
        {
        case Segment::BOSS_INTRO:
            _begin_boss_intro();
            break;

        case Segment::DIALOGUE:
            _line = 0;
            _begin_line();
            break;

        case Segment::BRIEFING_CARD:
            _draw_briefing();
            break;

        case Segment::REWARD_CARD:
            _particles.emplace(ParticleKind::SPARKLES, 16, 0, 7);
            play_sound(Sound::UNLOCK);
            _draw_reward();
            break;

        default:
            _draw_retry();
            break;
        }
    }

    // ---------------------------------------------------------------- dialogue

    [[nodiscard]] static bool left_side(Character speaker)
    {
        return speaker == Character::ARDEN || speaker == Character::ORRIN;
    }

    [[nodiscard]] int _chars_per_frame_x2() const
    {
        // text speed: 0 slow, 1 normal, 2 fast, 3 instant (half characters per frame)
        constexpr int speeds[4] = { 1, 2, 4, 9999 };
        return speeds[_g.profile.settings.text_speed & 3];
    }

    void _begin_line()
    {
        const Dialogue& dialogue = *_steps[_index].dialogue;
        const DialogueLine& line = dialogue.lines[_line];
        const bool narrator = line.speaker == Character::NARRATOR;
        _text.clear();
        _name_text.clear();
        _continue.clear();
        _panel.clear(0);
        _panel.window(0, 13, 30, 7);
        _revealed = 0;
        _line_complete = false;

        if(line.speaker != _speaker || ! _portrait)
        {
            _portrait.reset();
        }

        _speaker = line.speaker;

        if(! narrator)
        {
            const bool left = left_side(line.speaker);
            const int window_x = left ? 0 : 21;
            const int name_x = left ? 9 : 8;
            _panel.window(window_x, 4, 9, 9);
            _panel.window(name_x, 10, 13, 3);

            if(! _portrait)
            {
                _portrait = portrait_item(line.speaker).create_sprite(window_x * 8 + 36 - 120, 32 + 36 - 80,
                                                                      int(line.expression));
                _portrait->set_horizontal_flip(! left);
                _portrait->set_bg_priority(0);
            }
            else
            {
                _portrait->set_tiles(portrait_item(line.speaker).tiles_item(), int(line.expression));
            }

            const int name_center = name_x * 8 + 52;
            print(_name_text, name_center, 85, character_name(line.speaker), TextColor::GOLD, Align::CENTER);
            print(_name_text, name_center, 95, character_title(line.speaker), TextColor::GRAY, Align::CENTER);
        }

        test_interface().dialogue_active = 1;
        test_interface().dialogue_line = uint32_t(_line);
        _render_text();
    }

    void _render_text()
    {
        const DialogueLine& line = _steps[_index].dialogue->lines[_line];
        const char* text = line.text;
        const int length = bn::string_view(text).size();
        const int visible = _revealed / 2 >= length ? length : _revealed / 2;
        TextSpan spans[layout::dialogue_lines];
        const int lines = wrap_lines(FontId::SMALL, text, length, layout::dialogue_text_width, spans,
                                     layout::dialogue_lines);
        _text.clear();
        const TextColor color = line.speaker == Character::NARRATOR ? TextColor::CYAN : TextColor::WHITE;

        for(int i = 0; i < lines; ++i)
        {
            const int start = spans[i].start;
            int count = spans[i].length;

            if(start >= visible)
            {
                break;
            }

            if(start + count > visible)
            {
                count = visible - start;
            }

            print(_text, 12, 114 + i * 11, bn::string_view(text + start, count), color);
        }

        _line_complete = visible >= length;
    }

    void _update_dialogue()
    {
        const Dialogue& dialogue = *_steps[_index].dialogue;

        if(bn::keypad::start_pressed() || bn::keypad::b_pressed())
        {
            // Skip the rest of this dialogue.
            _advance();
            return;
        }

        if(! _line_complete)
        {
            _revealed += _chars_per_frame_x2();

            if(bn::keypad::a_pressed())
            {
                _revealed = 99999;
            }

            _render_text();
            return;
        }

        // Blinking continue marker.
        if((_frames & 15) == 0)
        {
            _continue.clear();

            if(_frames & 16)
            {
                print(_continue, 224, 144, "◆", TextColor::GOLD);
            }
        }

        if(bn::keypad::a_pressed())
        {
            play_sound(Sound::CURSOR);

            if(++_line >= dialogue.count)
            {
                _advance();
                return;
            }

            _begin_line();
        }
    }

    // ---------------------------------------------------------------- boss introduction

    void _begin_boss_intro()
    {
        test_interface().cinematic = uint32_t(1 + _flow.world);
        play_sound(Sound::BOSS_APPEAR);
        const Encounter& e = encounter(_flow.encounter);
        constexpr ParticleKind kinds[worlds_count] = { ParticleKind::LEAVES, ParticleKind::EMBERS, ParticleKind::SNOW,
                                                       ParticleKind::RUNES, ParticleKind::RAIN, ParticleKind::FRAGMENTS };
        _particles.emplace(kinds[_flow.world], _flow.world == 4 ? 22 : 16, 1, uint32_t(_flow.world + 3));
        _particles->set_center(120, 60);
        _portrait = portrait_item(e.opponent).create_sprite(0, 200, int(Expression::INTENSE));
        _portrait->set_bg_priority(0);
        _portrait->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
        _portrait->set_scale(2);
        _portrait->set_horizontal_flip(true);

        // Opening state of each composition.
        switch(_flow.world)
        {
        case 2: set_fades(bn::color(31, 31, 31), 1); break;          // Frozen Court: out of a white-out
        case 3: set_fades(bn::color(0, 0, 0), 1); break;             // Umbral Archive: out of darkness
        case 5: set_fades(bn::color(31, 26, 10), bn::fixed(0.8)); break;   // Crown Nexus: golden light
        default: break;
        }
    }

    void _update_boss_intro()
    {
        const int t = ++_step_frames;
        const int world_index = _flow.world;
        bn::fixed px = 64;
        bn::fixed py = -4;

        switch(world_index)
        {
        case 0:     // Verdant March: the warden rises slowly among falling leaves
            py = t < 50 ? bn::fixed(100) - bn::fixed(104 * t) / 50 : bn::fixed(-4);
            break;

        case 1:     // Inferno Bastion: slams in from the right with a red flash and a shaking fortress
            px = t < 14 ? bn::fixed(64) + (14 - t) * 12 : bn::fixed(64);

            if(t == 14)
            {
                set_fades(bn::color(31, 6, 2), bn::fixed(0.7));
            }
            else if(t > 14 && t < 30)
            {
                set_fades(bn::color(31, 6, 2), bn::fixed(0.7) - bn::fixed(t - 14) / 23);
                _backdrop.set_position(8 + ((t * 7) % 5) - 2, 48 + ((t * 3) % 5) - 2);
            }
            else if(t == 30)
            {
                reset_fades();
                _backdrop.set_position(8, 48);
            }
            break;

        case 2:     // Frozen Court: the white-out clears, the empress fades in through the snow
            if(t <= 60)
            {
                set_fades(bn::color(31, 31, 31), bn::fixed(60 - t) / 60);
            }

            if(t < 40)
            {
                py = 200;
            }
            break;

        case 3:     // Umbral Archive: out of darkness, the scholar flickers into being inside a ring of runes
            if(t <= 40)
            {
                set_fades(bn::color(0, 0, 0), bn::fixed(40 - t) / 40);
            }

            _portrait->set_visible(t > 60 || (t > 24 && ((t >> 1) & 1)));
            break;

        case 4:     // Storm Spire: lightning flashes, the sovereign drops from the clouds and lands
            if(t == 12 || t == 40 || t == 44)
            {
                set_fades(bn::color(31, 31, 31), bn::fixed(0.85));
            }
            else if(t == 14 || t == 42 || t == 46)
            {
                reset_fades();
            }

            if(t < 30)
            {
                py = bn::fixed(-120) + bn::fixed(116 * t) / 30;
            }
            else if(t < 40)
            {
                py = bn::fixed(-4) - bn::lut_sin(((t - 30) * 102) & 2047) * 8;
            }
            break;

        default:    // Crown Nexus: the sovereign grows out of the light, circled by the crown fragments
            if(t <= 50)
            {
                set_fades(bn::color(31, 26, 10), bn::fixed(0.8) * (50 - t) / 50);
                _portrait->set_scale(bn::fixed(0.2) + bn::fixed(18 * t) / 500);
            }
            break;
        }

        _portrait->set_position(px - 0, py);

        if(t == 56)
        {
            // Name and title banner (distinct colour per kingdom).
            constexpr TitleColor colors[worlds_count] = { TitleColor::GOLD, TitleColor::RUBY, TitleColor::SILVER,
                                                         TitleColor::ARCANE, TitleColor::SILVER, TitleColor::GOLD };
            const Encounter& e = encounter(_flow.encounter);
            _panel.window(0, 15, 30, 5);
            print_big(_text, 120, 124, character_name(e.opponent), colors[world_index], Align::CENTER);
            print(_text, 120, 143, character_title(e.opponent), TextColor::WHITE, Align::CENTER);
        }

        const bool skip = bn::keypad::start_pressed() || bn::keypad::b_pressed() ||
                          (t > 30 && bn::keypad::a_pressed());

        if(skip || t >= 210)
        {
            _backdrop.set_position(8, 48);
            _advance();
        }
    }

    // ---------------------------------------------------------------- cards

    void _draw_briefing()
    {
        const Encounter& e = encounter(_flow.encounter);
        const SaveProgress& progress = _g.profile.progress;
        _text.clear();
        _panel.window(1, 1, 28, 18);
        print_big(_text, 120, 14, e.title, TitleColor::GOLD, Align::CENTER);

        bn::string<64> versus;
        bn::ostringstream stream(versus);
        stream << type_label(e.type) << " vs " << character_name(e.opponent);
        print(_text, 120, 34, versus, TextColor::CYAN, Align::CENTER);

        TextSpan spans[layout::briefing_lines];
        const int length = bn::string_view(e.briefing).size();
        const int lines = wrap_lines(FontId::SMALL, e.briefing, length, layout::briefing_text_width, spans,
                                     layout::briefing_lines);

        for(int i = 0; i < lines; ++i)
        {
            print(_text, 16, 50 + i * 11, bn::string_view(e.briefing + spans[i].start, spans[i].length));
        }

        bn::string<48> level;
        bn::ostringstream level_stream(level);
        level_stream << "Opponent: " << difficulty_name(e.difficulty) << ", " << personality_name(e.personality);
        print(_text, 16, 108, level, TextColor::GRAY);

        if(encounter_cleared(progress, _flow.encounter))
        {
            bn::string<32> best;
            bn::ostringstream best_stream(best);
            best_stream << "★ Best: " << int(progress.encounter_best[_flow.encounter]) << " moves";
            print(_text, 16, 120, best, TextColor::GOLD);
        }

        print(_text, 120, 138, "Ⓐ Begin   Ⓑ Back to the map", TextColor::WHITE, Align::CENTER);
    }

    void _update_briefing()
    {
        if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            _advance();
        }
        else if(bn::keypad::b_pressed())
        {
            _g.story.world = _flow.world;
            _next = SceneId::STORY_MAP;
        }
    }

    void _draw_reward()
    {
        const int w = _flow.world;
        _text.clear();
        _panel.window(2, 3, 26, 14);
        print_big(_text, 120, 32, "KINGDOM RESTORED", TitleColor::GOLD, Align::CENTER);
        print(_text, 120, 54, world(w).name, TextColor::CYAN, Align::CENTER);

        constexpr const char* boards[worlds_count] = { "VERDANT", "INFERNO", "FROST", "UMBRAL", "STORM", "NEXUS" };
        bn::string<48> board;
        bn::ostringstream board_stream(board);
        board_stream << "New board skin: " << boards[w];
        print(_text, 120, 72, board, TextColor::WHITE, Align::CENTER);

        bn::string<48> gallery;
        bn::ostringstream gallery_stream(gallery);
        gallery_stream << "Gallery: " << character_name(world(w).boss);
        print(_text, 120, 84, gallery, TextColor::WHITE, Align::CENTER);

        if(w + 1 < worlds_count)
        {
            bn::string<48> next;
            bn::ostringstream next_stream(next);
            next_stream << "The road opens to " << world(w + 1).name;
            print(_text, 120, 100, next, TextColor::GOLD, Align::CENTER);
        }

        print(_text, 120, 120, "Ⓐ Continue", TextColor::GRAY, Align::CENTER);
    }

    void _update_reward()
    {
        if(bn::keypad::a_pressed() || bn::keypad::start_pressed())
        {
            _advance();
        }
    }

    void _draw_retry()
    {
        _text.clear();
        _panel.window(5, 6, 20, 9);
        print_big(_text, 120, 56, "TRY AGAIN?", TitleColor::RUBY, Align::CENTER);
        constexpr const char* options[2] = { "RETRY", "STORY MAP" };

        for(int i = 0; i < 2; ++i)
        {
            print(_text, 120, 80 + i * 14, options[i], i == _menu ? TextColor::GOLD : TextColor::WHITE, Align::CENTER);
        }

        print(_text, 72, 80 + _menu * 14, "→", TextColor::GOLD);
    }

    void _update_retry()
    {
        if(_input.up() || _input.down())
        {
            _menu ^= 1;
            _draw_retry();
        }

        if(bn::keypad::a_pressed())
        {
            if(_menu == 0)
            {
                MatchSetup& setup = _g.setup;
                setup.mode = MatchMode::STORY;
                setup.encounter = _flow.encounter;
                setup.resume_suspended = false;
                _next = SceneId::MATCH;
            }
            else
            {
                _g.story.world = _flow.world;
                _next = SceneId::STORY_MAP;
            }
        }
        else if(bn::keypad::b_pressed())
        {
            _g.story.world = _flow.world;
            _next = SceneId::STORY_MAP;
        }
    }
};

}

bn::unique_ptr<Scene> make_story_map_scene()
{
    return bn::unique_ptr<Scene>(new StoryMapScene());
}

bn::unique_ptr<Scene> make_story_dialogue_scene()
{
    return bn::unique_ptr<Scene>(new StoryDialogueScene());
}

}
