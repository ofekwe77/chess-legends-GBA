// CHESS LEGENDS: ARCANE WARS - opening and ending cinematics (in-engine: backdrops, tile reveals, sprites,
// palette fades, particles and dramatic text). Both are skippable; the reduced-effects option removes the flashes.
#include "bn_bg_palettes.h"
#include "bn_blending.h"
#include "bn_color.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_double_size_mode.h"
#include "bn_sprite_palette_ptr.h"
#include "bn_sprite_palettes.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "cl_art.h"
#include "cl_audio.h"
#include "cl_board_tiles.h"
#include "cl_layers.h"
#include "cl_scenes.h"
#include "cl_text.h"

#include "bn_bg_palette_items_board_palette_nexus.h"
#include "bn_bg_palette_items_board_palette_royal.h"
#include "bn_regular_bg_tiles_items_board_tiles.h"
#include "bn_sprite_items_crown.h"
#include "bn_sprite_items_pieces.h"
#include "bn_sprite_items_sigils.h"
#include "bn_sprite_palette_items_army_white.h"

namespace cl::gba
{

namespace
{
    void fades(bn::color color, bn::fixed intensity)
    {
        bn::bg_palettes::set_fade(color, intensity);
        bn::sprite_palettes::set_fade(color, intensity);
    }

    void clear_fades()
    {
        fades(bn::color(0, 0, 0), 0);
        bn::blending::set_transparency_alpha(1);
    }

    bool reduced_effects()
    {
        return globals().profile.settings.reduced_effects;
    }

    // A white flash that fades over `length` frames (none with reduced effects).
    void flash(int frame, int length)
    {
        if(reduced_effects())
        {
            return;
        }

        if(frame >= 0 && frame <= length)
        {
            fades(bn::color(31, 31, 31), bn::fixed(length - frame) / length);
        }
    }

    // Chessboard drawn square by square on a tile layer (board at screen (56, 16), 16 px squares).
    class RevealBoard
    {
    public:
        explicit RevealBoard(const bn::bg_palette_item& palette) :
            _layer(bn::regular_bg_tiles_items::board_tiles, palette, 2, 0)
        {
            _layer.clear(0);
            _layer.commit();
        }

        void set_square(int file, int rank, bool glow)
        {
            using namespace board_tiles;
            const bool light = ((file + rank) & 1) != 0;
            const bool variant = ((file * 3 + rank * 5) % 7) < 3;
            const int base = glow ? (light ? light_sel : dark_sel) :
                                    (light ? (variant ? light_b : light_a) : (variant ? dark_b : dark_a));
            const int mx = 7 + file * 2;
            const int my = 2 + (7 - rank) * 2;
            _layer.set(mx, my, base);
            _layer.set(mx + 1, my, base + 1);
            _layer.set(mx, my + 1, base + 2);
            _layer.set(mx + 1, my + 1, base + 3);
        }

        void fill()
        {
            for(int r = 0; r < 8; ++r)
            {
                for(int f = 0; f < 8; ++f)
                {
                    set_square(f, r, false);
                }
            }
        }

        void set_palette(const bn::bg_palette_item& palette)
        {
            _layer.set_palette(palette);
        }

        void set_visible(bool visible)
        {
            _layer.bg().set_visible(visible);
        }

        void commit()
        {
            _layer.commit();
        }

    private:
        TileLayer _layer;
    };

    // Square reveal order: a diagonal wave from a1.
    int reveal_time(int file, int rank)
    {
        return (file + rank) * 7 + ((file * 5 + rank * 3) % 4);
    }

// ------------------------------------------------------------------------------------------------ opening

class IntroScene final : public Scene
{
public:
    IntroScene()
    {
        set_current_scene(SceneId::INTRO);
        play_music(Music::TITLE);
        test_interface().cinematic = 100;
        fades(bn::color(0, 0, 0), 1);
    }

    ~IntroScene() override
    {
        clear_fades();
        test_interface().cinematic = 0;
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        const int t = ++_frames;

        if(t > 10 && (bn::keypad::start_pressed() || bn::keypad::a_pressed() || bn::keypad::b_pressed()))
        {
            return _finish();
        }

        if(_particles)
        {
            _particles->update();
        }

        if(t < 300)
        {
            _shot_board(t);
        }
        else if(t < 460)
        {
            _shot_crown(t - 300);
        }
        else if(t < 700)
        {
            _shot_war(t - 460);
        }
        else if(t < 830)
        {
            _shot_hero(t - 700);
        }
        else if(t < 960)
        {
            _shot_title(t - 830);
        }
        else
        {
            return _finish();
        }

        if(_board)
        {
            _board->commit();
        }

        _panel.commit();
        test_interface().menu_index = uint32_t(t);
        return bn::nullopt;
    }

private:
    bn::optional<RevealBoard> _board;
    bn::optional<bn::regular_bg_ptr> _backdrop;
    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 6> _sigils;
    bn::optional<bn::sprite_ptr> _crown;
    bn::optional<bn::sprite_ptr> _portrait;
    bn::optional<bn::sprite_ptr> _piece;
    bn::optional<ParticleField> _particles;
    bn::vector<bn::sprite_ptr, 40> _text;
    int _frames = 0;
    int _cut = -1;

    bn::optional<SceneId> _finish()
    {
        Globals& g = globals();

        if(! g.profile.progress.intro_seen)
        {
            g.profile.progress.intro_seen = true;
            save_profile_now();
        }

        return SceneId::TITLE;
    }

    void _caption(const char* text)
    {
        _text.clear();

        if(text)
        {
            print(_text, 120, 149, text, TextColor::WHITE, Align::CENTER);
        }
    }

    // Shots 1 and 2: the ancient board lights up square by square, then six kingdom sigils appear around it.
    void _shot_board(int t)
    {
        if(t == 1)
        {
            _board.emplace(bn::bg_palette_items::board_palette_nexus);
        }

        if(t <= 40)
        {
            fades(bn::color(0, 0, 0), bn::fixed(40 - t) / 40);
        }

        for(int r = 0; r < 8; ++r)
        {
            for(int f = 0; f < 8; ++f)
            {
                const int appear = 30 + reveal_time(f, r);

                if(t == appear)
                {
                    _board->set_square(f, r, true);
                }
                else if(t == appear + 14)
                {
                    _board->set_square(f, r, false);
                }
            }
        }

        // Sigils on both sides of the board, one by one.
        constexpr int xs[6] = { 28, 212, 28, 212, 28, 212 };
        constexpr int ys[6] = { 28, 28, 72, 72, 116, 116 };

        for(int i = 0; i < 6; ++i)
        {
            const int appear = 170 + i * 18;

            if(t == appear)
            {
                bn::sprite_ptr sigil = bn::sprite_items::sigils.create_sprite(xs[i] - 120, ys[i] + 2 - 80, i);
                sigil.set_bg_priority(1);
                sigil.set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
                _sigils.push_back(bn::move(sigil));
            }
        }

        for(int i = 0; i < _sigils.size(); ++i)
        {
            const int age = t - (170 + i * 18);
            const bn::fixed scale = age < 8 ? bn::fixed(age + 1) / 4 : (age < 14 ? bn::fixed(2) - bn::fixed(age - 8) / 6 :
                                                                                  bn::fixed(1));
            _sigils[i].set_scale(scale);
        }

        if(t == 30)
        {
            _caption("Long ago, the board was enchanted...");
        }
        else if(t == 170)
        {
            _caption("...and six kingdoms shared its magic.");
        }
    }

    // Shot 3: the crown of six materializes above the board, then breaks into six fragments.
    void _shot_crown(int t)
    {
        if(t == 1)
        {
            _crown = bn::sprite_items::crown.create_sprite(0, 52 - 80);
            _crown->set_bg_priority(0);
            _crown->set_blending_enabled(true);
            _caption("They were bound by the Crown of Six.");
        }

        if(t <= 50)
        {
            bn::blending::set_transparency_alpha(bn::fixed(t) / 50);
        }

        if(t == 95)
        {
            bn::blending::set_transparency_alpha(1);
            _crown.reset();
            _particles.emplace(ParticleKind::FRAGMENTS, 6, 0, 5);
            _particles->set_center(120, 50);
            _caption("Until the night it shattered.");
        }

        flash(t - 95, 20);
    }

    // Shot 4: the war - rulers of three kingdoms in quick cuts, with their magic.
    void _shot_war(int t)
    {
        const int cut = t / 80;

        if(cut != _cut)
        {
            _cut = cut;
            constexpr Backdrop backdrops[3] = { Backdrop::INFERNO, Backdrop::STORM, Backdrop::UMBRAL };
            constexpr Character rulers[3] = { Character::VEYRA, Character::RAEL, Character::NOCTIS };
            constexpr ParticleKind kinds[3] = { ParticleKind::EMBERS, ParticleKind::RAIN, ParticleKind::RUNES };
            constexpr const char* captions[3] = { "Six rulers seized the fragments.", "The kingdoms went to war...",
                                                  "...on the enchanted board." };
            _board.reset();
            _sigils.clear();
            _backdrop = create_backdrop(backdrops[cut]);
            _particles.emplace(kinds[cut], cut == 1 ? 22 : 16, 1, uint32_t(9 + cut));
            _particles->set_center(120, 64);
            _portrait = portrait_item(rulers[cut]).create_sprite(0, -6, int(Expression::INTENSE));
            _portrait->set_bg_priority(0);
            _portrait->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _portrait->set_scale(2);
            _portrait->set_horizontal_flip(cut != 1);
            _caption(captions[cut]);
        }

        const int local = t % 80;

        // Each ruler slides in from a different side; the storm cut flashes with lightning.
        if(_portrait)
        {
            const int from = cut == 1 ? -180 : 180;
            const bn::fixed x = local < 20 ? bn::fixed(from) * (20 - local) / 20 + bn::fixed(cut == 1 ? -60 : 60) :
                                             bn::fixed(cut == 1 ? -60 : 60);
            _portrait->set_x(x);
        }

        if(cut == 1)
        {
            flash(local - 30, 6);
            flash(local - 50, 6);
        }
    }

    // Shot 5: the hero - Arden's silhouette, a glowing chess piece in hand.
    void _shot_hero(int t)
    {
        if(t == 1)
        {
            _backdrop = create_backdrop(Backdrop::TITLE);
            bn::bg_palettes::set_fade(bn::color(0, 0, 0), bn::fixed(0.6));
            _particles.emplace(ParticleKind::SPARKLES, 10, 0, 21);
            _portrait = portrait_item(Character::ARDEN).create_sprite(-30, -4, 0);
            _portrait->set_bg_priority(0);
            _portrait->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _portrait->set_scale(2);
            bn::sprite_palette_ptr palette = _portrait->palette();
            palette.set_fade(bn::color(9, 6, 16), 1);
            _piece = bn::sprite_items::pieces.create_sprite(52, 30, 5);
            _piece->set_palette(bn::sprite_palette_items::army_white);
            _piece->set_bg_priority(0);
            _caption("One young tactician rose to restore it.");
        }

        if(_piece)
        {
            // The piece glows: it pulses between full light and a soft golden fade.
            const int phase = (t * 24) & 2047;
            bn::sprite_palette_ptr palette = _piece->palette();
            palette.set_fade(bn::color(31, 28, 12), (bn::lut_sin(phase) + 1) / 4);
        }
    }

    // Shot 6: the title.
    void _shot_title(int t)
    {
        if(t == 1)
        {
            _portrait.reset();

            if(_piece)
            {
                bn::sprite_palette_ptr palette = _piece->palette();
                palette.set_fade(bn::color(0, 0, 0), 0);
            }

            _piece.reset();
            _particles.emplace(ParticleKind::STARS, 14, 1, 31);
            bn::bg_palettes::set_fade(bn::color(0, 0, 0), 0);
            _crown = bn::sprite_items::crown.create_sprite(0, 26 - 80);
            _crown->set_bg_priority(0);
            _text.clear();
            print_big(_text, 120, 66, "CHESS LEGENDS", TitleColor::GOLD, Align::CENTER);
            print_big(_text, 120, 88, "ARCANE WARS", TitleColor::ARCANE, Align::CENTER);
        }

        flash(t, 30);
    }
};

// ------------------------------------------------------------------------------------------------ ending

class EndingScene final : public Scene
{
public:
    EndingScene()
    {
        set_current_scene(SceneId::ENDING);
        play_music(Music::ENDING);
        test_interface().cinematic = 200;
        Globals& g = globals();

        // Completion is saved before the first frame: skipping the ending loses nothing.
        if(! g.profile.progress.ending_seen)
        {
            g.profile.progress.ending_seen = true;
            save_profile_now();
        }
    }

    ~EndingScene() override
    {
        clear_fades();
        test_interface().cinematic = 0;
    }

    [[nodiscard]] bn::optional<SceneId> update() override
    {
        const int t = ++_frames;

        if(t > 10 && (bn::keypad::start_pressed() || bn::keypad::b_pressed()))
        {
            return SceneId::CREDITS;
        }

        if(_particles)
        {
            _particles->update();
        }

        if(t < 200)
        {
            _shot_reunion(t);
        }
        else if(t < 330)
        {
            _shot_board(t - 200);
        }
        else if(t < 330 + worlds_count * 75)
        {
            _shot_kingdoms(t - 330);
        }
        else if(t < 330 + worlds_count * 75 + 240)
        {
            _shot_guardian(t - 330 - worlds_count * 75);
        }
        else
        {
            return SceneId::CREDITS;
        }

        if(_board)
        {
            _board->commit();
        }

        _panel.commit();
        test_interface().menu_index = uint32_t(t);
        return bn::nullopt;
    }

private:
    bn::optional<bn::regular_bg_ptr> _backdrop;
    bn::optional<RevealBoard> _board;
    PanelLayer _panel;
    bn::optional<bn::sprite_ptr> _crown;
    bn::optional<bn::sprite_ptr> _portrait;
    bn::optional<ParticleField> _particles;
    bn::vector<bn::sprite_ptr, 40> _text;
    int _frames = 0;
    int _kingdom = -1;

    void _caption(const char* text, int y = 140)
    {
        _text.clear();

        if(text)
        {
            print(_text, 120, y, text, TextColor::WHITE, Align::CENTER);
        }
    }

    // The six fragments circle closer and closer, then the crown reforms in a flash of light.
    void _shot_reunion(int t)
    {
        if(t == 1)
        {
            _backdrop = create_backdrop(Backdrop::NEXUS);
            _particles.emplace(ParticleKind::FRAGMENTS, 6, 0, 13);
            _particles->set_center(120, 60);
            _caption("The six fragments rose from the six rulers...");
        }

        if(t == 120)
        {
            _particles.reset();
            _crown = bn::sprite_items::crown.create_sprite(0, 60 - 80);
            _crown->set_bg_priority(0);
            _caption("...and the Crown of Six was whole again.");
        }

        flash(t - 120, 30);
    }

    // The enchanted board is transformed: its dark magic gives way to the royal colours.
    void _shot_board(int t)
    {
        if(t == 1)
        {
            _crown.reset();
            _backdrop.reset();
            _board.emplace(bn::bg_palette_items::board_palette_nexus);
            _board->fill();
            _caption("The enchanted board itself was renewed.");
        }

        if(t == 60)
        {
            _board->set_palette(bn::bg_palette_items::board_palette_royal);
        }

        flash(t - 60, 24);
    }

    // The six kingdoms, restored.
    void _shot_kingdoms(int t)
    {
        const int kingdom = t / 75;

        if(kingdom != _kingdom && kingdom < worlds_count)
        {
            _kingdom = kingdom;
            constexpr const char* captions[worlds_count] = {
                "The Verdant March blooms again.", "The Bastion's fires warm, not burn.",
                "The Frozen Court opens its gates.", "The Archive shares its pages.",
                "The Storm Spire sings with thunder.", "The Crown Nexus shines at dawn."
            };
            constexpr ParticleKind kinds[worlds_count] = { ParticleKind::LEAVES, ParticleKind::EMBERS,
                                                           ParticleKind::SNOW, ParticleKind::SPARKLES,
                                                           ParticleKind::SPARKLES, ParticleKind::STARS };
            _board.reset();
            _backdrop = create_backdrop(world_backdrop(kingdom));
            _particles.emplace(kinds[kingdom], 14, 1, uint32_t(40 + kingdom));
            _portrait = portrait_item(world(kingdom).boss).create_sprite(84, -40, int(Expression::NEUTRAL));
            _portrait->set_bg_priority(0);
            _portrait->set_horizontal_flip(true);
            _panel.clear(0);
            _panel.window(0, 16, 30, 4);
            _caption(captions[kingdom], 136);
        }

        const int local = t % 75;

        if(local < 12)
        {
            fades(bn::color(0, 0, 0), bn::fixed(12 - local) / 12);
        }
        else if(local > 63)
        {
            fades(bn::color(0, 0, 0), bn::fixed(local - 63) / 12);
        }
    }

    // Arden is named guardian of the Arcane board.
    void _shot_guardian(int t)
    {
        if(t == 1)
        {
            clear_fades();
            _panel.clear(0);
            _backdrop = create_backdrop(Backdrop::TITLE);
            _particles.emplace(ParticleKind::STARS, 14, 1, 51);
            _crown = bn::sprite_items::crown.create_sprite(0, 22 - 80);
            _crown->set_bg_priority(0);
            _portrait = portrait_item(Character::ARDEN).create_sprite(0, 2, int(Expression::NEUTRAL));
            _portrait->set_bg_priority(0);
            _portrait->set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
            _portrait->set_scale(bn::fixed(1.5));
            _text.clear();
            print_big(_text, 120, 122, "ARDEN", TitleColor::GOLD, Align::CENTER);
            print(_text, 120, 142, "Guardian of the Arcane Board", TextColor::CYAN, Align::CENTER);
        }

        flash(t, 30);
    }
};

}

bn::unique_ptr<Scene> make_intro_scene()
{
    return bn::unique_ptr<Scene>(new IntroScene());
}

bn::unique_ptr<Scene> make_ending_scene()
{
    return bn::unique_ptr<Scene>(new EndingScene());
}

}
