// CHESS LEGENDS: ARCANE WARS - capture battle scenes.
#include "cl_battle.h"

#include "bn_bg_palettes.h"
#include "bn_blending.h"
#include "bn_color.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_sprite_double_size_mode.h"
#include "bn_sprite_palettes.h"
#include "bn_string.h"

#include "cl_audio.h"
#include "cl_board_view.h"
#include "cl_globals.h"
#include "cl_text.h"

#include "bn_sprite_items_battle_cavalry.h"
#include "bn_sprite_items_battle_floor.h"
#include "bn_sprite_items_battle_guardian.h"
#include "bn_sprite_items_battle_mystic.h"
#include "bn_sprite_items_battle_shadow.h"
#include "bn_sprite_items_battle_soldier.h"
#include "bn_sprite_items_battle_sorceress.h"
#include "bn_sprite_items_battle_sovereign.h"
#include "bn_sprite_items_particles.h"

namespace cl::gba
{

namespace
{
    // Battle sprite frames (assets_source/gen_battle.py).
    enum Pose : int16_t
    {
        IDLE, BREATH, WINDUP, STRIKE, HURT, DEFEAT
    };

    using namespace particle;

    enum class Beat : uint8_t
    {
        FRAME,          // actor pose = a
        MOVE,           // actor runs to screen x = a in b frames
        FLASH,          // white flash fading over b frames
        SHAKE,          // screen shake of amplitude a for b frames
        HOLD,           // the action freezes for a frames (impact)
        SOUND,          // sound effect a
        BURST,          // b particles (frame a) fly out of the actor's body
        FX,             // a 16x16 effect (Fx a) on the actor's body, b frames per animation frame
        CIRCLE,         // a circle of runes orbits the actor until shortly after the impact
        PROJECTILE,     // particle frame a flies from the attacker's hand to the victim in b frames
        FADE,           // actor fades out in b frames
        FALL,           // actor topples backwards by a degrees in b frames
        END             // the attacker is back home: the attack is over
    };

    constexpr uint8_t ATTACKER = 0;
    constexpr uint8_t VICTIM = 1;
    constexpr int attacker_home = 64;
    constexpr int victim_home = 176;
    constexpr int ground_y = 150;       // screen y of the feet
    constexpr int body_height = 28;     // feet to the sprite centre (64x64 frames, feet on row 60)
    constexpr int chest_y = ground_y - body_height - 6;
    constexpr int floor_top = 140;
    constexpr int entrance_frames = 28;
    constexpr int victory_frames = 32;
    constexpr int fade_frames = 8;

    struct Key
    {
        uint8_t frame;
        Beat beat;
        uint8_t actor;
        int16_t a;
        int16_t b;
    };

    struct Timeline
    {
        const Key* keys;
        int count;
        int hit;            // attacks: frame at which the strike lands
    };

    constexpr int16_t capture_sound = int16_t(Sound::CAPTURE);
    constexpr int16_t SMOKE_FX = int16_t(Fx::SMOKE);
    constexpr int16_t RING_FX = int16_t(Fx::RING);
    constexpr int16_t GOLD_FX = int16_t(Fx::GOLD_RING);
    constexpr int16_t FLARE_FX = int16_t(Fx::FLARE);

    // ---- shared parts of every sequence --------------------------------------------------------------------
    constexpr Key entrance[] = {
        { 2, Beat::MOVE, ATTACKER, attacker_home, 22 }, { 2, Beat::MOVE, VICTIM, victim_home, 22 },
    };
    constexpr Key impact[] = {
        { 0, Beat::HOLD, ATTACKER, 5, 0 },
    };
    constexpr Key victory[] = {
        { 0, Beat::FRAME, ATTACKER, WINDUP, 0 }, { 2, Beat::BURST, ATTACKER, STAR, 6 },
        { 20, Beat::FRAME, ATTACKER, IDLE, 0 },
    };

    // ---- attacks ------------------------------------------------------------------------------------------
    // Soldier: defensive stance, draws the short sword, rushes forward, quick strike, returns to idle.
    constexpr Key soldier_attack[] = {
        { 0, Beat::FRAME, ATTACKER, IDLE, 0 }, { 6, Beat::FRAME, ATTACKER, WINDUP, 0 },
        { 12, Beat::MOVE, ATTACKER, 140, 12 }, { 24, Beat::FRAME, ATTACKER, STRIKE, 0 },
        { 26, Beat::SOUND, ATTACKER, capture_sound, 0 }, { 26, Beat::FLASH, ATTACKER, 0, 4 },
        { 26, Beat::SHAKE, ATTACKER, 2, 8 }, { 26, Beat::BURST, VICTIM, SPARK_A, 8 },
        { 26, Beat::FX, VICTIM, RING_FX, 3 }, { 40, Beat::FRAME, ATTACKER, IDLE, 0 },
        { 42, Beat::MOVE, ATTACKER, attacker_home, 14 }, { 60, Beat::END, ATTACKER, 0, 0 },
    };
    // Cavalry Champion: raises the lance, leans forward, charges, sweeping strike, slides back.
    constexpr Key cavalry_attack[] = {
        { 0, Beat::FRAME, ATTACKER, IDLE, 0 }, { 4, Beat::FRAME, ATTACKER, WINDUP, 0 },
        { 14, Beat::FRAME, ATTACKER, STRIKE, 0 }, { 14, Beat::MOVE, ATTACKER, 146, 10 },
        { 26, Beat::SOUND, ATTACKER, capture_sound, 0 }, { 26, Beat::FLASH, ATTACKER, 0, 5 },
        { 26, Beat::SHAKE, ATTACKER, 3, 10 }, { 26, Beat::BURST, VICTIM, SPARK_A, 10 },
        { 26, Beat::FX, VICTIM, RING_FX, 3 }, { 34, Beat::MOVE, ATTACKER, attacker_home, 18 },
        { 38, Beat::FRAME, ATTACKER, IDLE, 0 },
        { 60, Beat::END, ATTACKER, 0, 0 },
    };
    // Arcane Mystic: raises the staff, charges a glowing symbol, releases an energy bolt, back to idle.
    constexpr Key mystic_attack[] = {
        { 0, Beat::FRAME, ATTACKER, IDLE, 0 }, { 6, Beat::FRAME, ATTACKER, WINDUP, 0 },
        { 6, Beat::BURST, ATTACKER, STAR, 6 }, { 14, Beat::BURST, ATTACKER, SPARK_A, 6 },
        { 24, Beat::FRAME, ATTACKER, STRIKE, 0 }, { 24, Beat::PROJECTILE, ATTACKER, ORB, 10 },
        { 34, Beat::SOUND, ATTACKER, capture_sound, 0 }, { 34, Beat::FLASH, ATTACKER, 0, 6 },
        { 34, Beat::SHAKE, ATTACKER, 2, 6 }, { 34, Beat::BURST, VICTIM, ORB, 8 },
        { 34, Beat::FX, VICTIM, FLARE_FX, 4 }, { 44, Beat::FRAME, ATTACKER, IDLE, 0 },
        { 58, Beat::END, ATTACKER, 0, 0 },
    };
    // Fortress Guardian: braces behind the shield, raises the hammer, heavy blow, shockwave, defensive pose.
    constexpr Key guardian_attack[] = {
        { 0, Beat::FRAME, ATTACKER, IDLE, 0 }, { 8, Beat::FRAME, ATTACKER, WINDUP, 0 },
        { 16, Beat::MOVE, ATTACKER, 134, 12 }, { 30, Beat::FRAME, ATTACKER, STRIKE, 0 },
        { 32, Beat::SOUND, ATTACKER, capture_sound, 0 }, { 32, Beat::SHAKE, ATTACKER, 4, 14 },
        { 32, Beat::FLASH, ATTACKER, 0, 4 }, { 32, Beat::BURST, VICTIM, SHARD, 12 },
        { 32, Beat::FX, VICTIM, RING_FX, 4 }, { 44, Beat::FRAME, ATTACKER, IDLE, 0 },
        { 46, Beat::MOVE, ATTACKER, attacker_home, 14 }, { 66, Beat::END, ATTACKER, 0, 0 },
    };
    // Royal Sorceress: extends her hand, a spell circle appears, energy gathers, a dramatic strike.
    constexpr Key sorceress_attack[] = {
        { 0, Beat::FRAME, ATTACKER, IDLE, 0 }, { 4, Beat::FRAME, ATTACKER, WINDUP, 0 },
        { 4, Beat::CIRCLE, ATTACKER, 0, 0 }, { 16, Beat::BURST, ATTACKER, SPARK_A, 6 },
        { 26, Beat::FRAME, ATTACKER, STRIKE, 0 }, { 26, Beat::PROJECTILE, ATTACKER, STAR, 8 },
        { 34, Beat::SOUND, ATTACKER, capture_sound, 0 }, { 34, Beat::FLASH, ATTACKER, 0, 8 },
        { 34, Beat::SHAKE, ATTACKER, 3, 10 }, { 34, Beat::BURST, VICTIM, STAR, 12 },
        { 34, Beat::FX, VICTIM, FLARE_FX, 4 }, { 46, Beat::FRAME, ATTACKER, IDLE, 0 },
        { 62, Beat::END, ATTACKER, 0, 0 },
    };
    // Sovereign: draws the royal blade, raises an aura, a deliberate powerful attack, a dignified stance.
    constexpr Key sovereign_attack[] = {
        { 0, Beat::FRAME, ATTACKER, IDLE, 0 }, { 6, Beat::BURST, ATTACKER, SPARK_A, 10 },
        { 6, Beat::FX, ATTACKER, GOLD_FX, 5 }, { 16, Beat::FRAME, ATTACKER, WINDUP, 0 },
        { 22, Beat::MOVE, ATTACKER, 142, 16 },
        { 38, Beat::FRAME, ATTACKER, STRIKE, 0 }, { 40, Beat::SOUND, ATTACKER, capture_sound, 0 },
        { 40, Beat::FLASH, ATTACKER, 0, 6 }, { 40, Beat::SHAKE, ATTACKER, 3, 10 },
        { 40, Beat::BURST, VICTIM, SPARK_A, 12 }, { 40, Beat::FX, VICTIM, GOLD_FX, 3 },
        { 52, Beat::FRAME, ATTACKER, IDLE, 0 },
        { 54, Beat::MOVE, ATTACKER, attacker_home, 14 }, { 72, Beat::END, ATTACKER, 0, 0 },
    };

    // ---- defeats (frames relative to the hit) ---------------------------------------------------------------
    constexpr Key soldier_defeat[] = {      // falls backward
        { 0, Beat::FRAME, VICTIM, HURT, 0 }, { 6, Beat::FALL, VICTIM, 80, 14 }, { 20, Beat::FADE, VICTIM, 0, 12 },
    };
    constexpr Key cavalry_defeat[] = {      // loses balance
        { 0, Beat::FRAME, VICTIM, HURT, 0 }, { 4, Beat::FALL, VICTIM, 25, 14 }, { 4, Beat::MOVE, VICTIM, 198, 14 },
        { 18, Beat::FADE, VICTIM, 0, 14 },
    };
    constexpr Key mystic_defeat[] = {       // dissolves into magical particles
        { 0, Beat::FRAME, VICTIM, HURT, 0 }, { 4, Beat::BURST, VICTIM, SPARK_A, 12 }, { 4, Beat::FADE, VICTIM, 0, 20 },
        { 12, Beat::BURST, VICTIM, STAR, 8 },
    };
    constexpr Key guardian_defeat[] = {     // collapses
        { 0, Beat::FRAME, VICTIM, HURT, 0 }, { 8, Beat::FRAME, VICTIM, DEFEAT, 0 }, { 8, Beat::SHAKE, VICTIM, 2, 6 },
        { 8, Beat::BURST, VICTIM, SMOKE, 6 }, { 10, Beat::FX, VICTIM, SMOKE_FX, 6 }, { 18, Beat::FADE, VICTIM, 0, 16 },
    };
    constexpr Key sorceress_defeat[] = {    // her magical aura shatters
        { 0, Beat::FRAME, VICTIM, HURT, 0 }, { 2, Beat::BURST, VICTIM, FRAGMENT, 12 }, { 2, Beat::FX, VICTIM, RING_FX, 2 },
        { 6, Beat::FLASH, VICTIM, 0, 3 }, { 10, Beat::FADE, VICTIM, 0, 16 },
    };
    constexpr Key sovereign_defeat[] = {    // kneels (kings are never captured; kept for completeness)
        { 0, Beat::FRAME, VICTIM, HURT, 0 }, { 8, Beat::FRAME, VICTIM, DEFEAT, 0 }, { 16, Beat::FADE, VICTIM, 0, 16 },
    };

    template<int N>
    constexpr Timeline timeline(const Key (&keys)[N], int hit = 0)
    {
        return Timeline{ keys, N, hit };
    }

    template<int N>
    constexpr int count_of(const Key (&)[N])
    {
        return N;
    }

    constexpr Timeline attacks[6] = {
        timeline(soldier_attack, 26), timeline(cavalry_attack, 26), timeline(mystic_attack, 34),
        timeline(guardian_attack, 32), timeline(sorceress_attack, 34), timeline(sovereign_attack, 40),
    };

    constexpr Timeline defeats[6] = {
        timeline(soldier_defeat), timeline(cavalry_defeat), timeline(mystic_defeat),
        timeline(guardian_defeat), timeline(sorceress_defeat), timeline(sovereign_defeat),
    };

    // Last frame touched by a timeline (including the duration of moves, fades and falls).
    constexpr int timeline_end(const Timeline& timeline)
    {
        int end = 0;

        for(int k = 0; k < timeline.count; ++k)
        {
            const Key& key = timeline.keys[k];
            const bool lasting = key.beat == Beat::MOVE || key.beat == Beat::FADE || key.beat == Beat::FALL;
            const int last = key.frame + (lasting ? key.b : 0);
            end = last > end ? last : end;
        }

        return end;
    }

    // Every attack lands its blow (the capture sound) at its hit frame and is back home at its last key.
    constexpr bool attacks_consistent()
    {
        for(const Timeline& attack : attacks)
        {
            bool sound_at_hit = false;

            for(int k = 0; k < attack.count; ++k)
            {
                const Key& key = attack.keys[k];
                sound_at_hit |= key.beat == Beat::SOUND && key.frame == attack.hit;
            }

            if(! sound_at_hit || attack.keys[attack.count - 1].beat != Beat::END ||
               timeline_end(attack) != attack.keys[attack.count - 1].frame)
            {
                return false;
            }
        }

        return true;
    }

    static_assert(attacks_consistent());

    const bn::sprite_item& battle_item(PieceType type)
    {
        switch(type)
        {
        case KNIGHT: return bn::sprite_items::battle_cavalry;
        case BISHOP: return bn::sprite_items::battle_mystic;
        case ROOK: return bn::sprite_items::battle_guardian;
        case QUEEN: return bn::sprite_items::battle_sorceress;
        case KING: return bn::sprite_items::battle_sovereign;
        default: return bn::sprite_items::battle_soldier;
        }
    }

    const char* class_name(PieceType type)
    {
        constexpr const char* names[6] = { "Soldier", "Cavalry", "Mystic", "Guardian", "Sorceress", "Sovereign" };
        return names[type - PAWN];
    }

    bool reduced_effects()
    {
        return globals().profile.settings.reduced_effects;
    }

    void set_fades(bn::color color, bn::fixed intensity)
    {
        bn::bg_palettes::set_fade(color, intensity);
        bn::sprite_palettes::set_fade(color, intensity);
    }
}

BattleScene::BattleScene(PieceType attacker, Color attacker_color, PieceType victim, Color victim_color,
                         Backdrop backdrop, int dark_faction, bool fast, PanelLayer& panel) :
    _backdrop(create_backdrop(backdrop, 3)),
    _panel(panel),
    _fast(fast)
{
    // The stage floor and the actors' shadows.
    for(int i = 0; i < _floor.max_size(); ++i)
    {
        bn::sprite_ptr tile = bn::sprite_items::battle_floor.create_sprite(-16 + i * 32 - 120, floor_top + 16 - 80);
        tile.set_bg_priority(2);
        _floor.push_back(bn::move(tile));
    }

    const PieceType types[2] = { attacker, victim };
    const Color colors[2] = { attacker_color, victim_color };
    const int starts[2] = { -40, 280 };

    for(int i = 0; i < 2; ++i)
    {
        Actor& actor = _actors[i];
        actor.type = types[i];
        actor.x = starts[i];
        bn::sprite_ptr sprite = battle_item(types[i]).create_sprite(0, 0, IDLE);
        sprite.set_palette(army_palette(colors[i], dark_faction));
        sprite.set_bg_priority(1);
        sprite.set_horizontal_flip(i == VICTIM);
        sprite.set_double_size_mode(bn::sprite_double_size_mode::ENABLED);
        actor.sprite = bn::move(sprite);
        bn::sprite_ptr shadow = bn::sprite_items::battle_shadow.create_sprite(0, 0);
        shadow.set_bg_priority(1);
        shadow.set_z_order(10);
        actor.shadow = bn::move(shadow);
    }

    const Timeline& attack = attacks[attacker - PAWN];
    const Timeline& defeat = defeats[victim - PAWN];
    _attack_start = entrance_frames;
    _hit = _attack_start + attack.hit;
    const int attack_end = _attack_start + timeline_end(attack);
    const int defeat_end = _hit + timeline_end(defeat);
    _victory = (attack_end > defeat_end ? attack_end : defeat_end) + 2;
    _end = _victory + victory_frames;
    _camera = -8;

    // Caption banner: who captures whom, and the skip key.
    _panel.clear(0);
    _panel.window(0, 0, 30, 3);
    bn::string<48> caption;
    bn::ostringstream stream(caption);
    stream << class_name(attacker) << " captures " << class_name(victim);
    print(_caption, 112, 8, caption, TextColor::GOLD, Align::CENTER);
    print(_caption, 234, 8, "Ⓐ Skip", TextColor::GRAY, Align::RIGHT);
    _panel.commit();

    set_fades(bn::color(0, 0, 0), 1);
    _animate(false);
    test_interface().battle_scenes += 1;
}

BattleScene::~BattleScene()
{
    bn::blending::set_transparency_alpha(1);
    _panel.clear(0);
    _panel.commit();
}

bool BattleScene::update()
{
    if(bn::keypad::a_pressed() || bn::keypad::b_pressed())
    {
        return false;   // skipping changes nothing: the capture was already applied
    }

    if(! _step())
    {
        return false;
    }

    return ! _fast || _step();
}

bool BattleScene::_step()
{
    if(_frame > _end)
    {
        return false;
    }

    if(_hold > 0)
    {
        // Impact freeze: the actors hold their pose while the flash and the particles go on.
        --_hold;
        _animate(true);
        return true;
    }

    _run_keys();
    _animate(false);
    ++_frame;
    return true;
}

void BattleScene::_run_keys()
{
    struct Track
    {
        const Key* keys;
        int count;
        int offset;
    };

    const Timeline& attack = attacks[_actors[ATTACKER].type - PAWN];
    const Timeline& defeat = defeats[_actors[VICTIM].type - PAWN];
    const Track tracks[] = {
        { entrance, count_of(entrance), 0 },
        { attack.keys, attack.count, _attack_start },
        { impact, count_of(impact), _hit },
        { defeat.keys, defeat.count, _hit },
        { victory, count_of(victory), _victory },
    };

    for(const Track& track : tracks)
    {
        for(int k = 0; k < track.count; ++k)
        {
            const Key& key = track.keys[k];

            if(key.frame + track.offset != _frame)
            {
                continue;
            }

            Actor& actor = _actors[key.actor];
            const bn::fixed body_x = actor.x - 120;
            const bn::fixed body_y = ground_y - body_height - 4 - 80;

            switch(key.beat)
            {
            case Beat::FRAME:
                actor.pose = key.a;
                break;

            case Beat::MOVE:
                actor.move_frames = key.b;
                actor.step = (bn::fixed(key.a) - actor.x) / key.b;
                break;

            case Beat::FLASH:
                if(! reduced_effects())
                {
                    _flash_total = key.b;
                    _flash_frames = key.b;
                }
                break;

            case Beat::SHAKE:
                if(! reduced_effects())
                {
                    _shake_amplitude = key.a;
                    _shake_frames = key.b;
                }
                break;

            case Beat::HOLD:
                _hold = _fast ? 0 : key.a;
                break;

            case Beat::SOUND:
                play_sound(Sound(key.a));
                break;

            case Beat::BURST:
                _effects.burst(key.a, body_x, body_y, key.b, bn::fixed(1.4), 24, 0);
                break;

            case Beat::FX:
                _effects.fx(Fx(key.a), body_x, body_y, key.b, 0);
                break;

            case Beat::CIRCLE:
                _circle.emplace(ParticleKind::RUNES, 8, 0, 17);
                break;

            case Beat::PROJECTILE:
                _projectile = bn::sprite_items::particles.create_sprite(0, 0, key.a);
                _projectile->set_bg_priority(0);
                _projectile_frame = key.a;
                _projectile_x = actor.x + 16;
                _projectile_frames = key.b;
                _projectile_step = (_actors[VICTIM].x - _projectile_x) / key.b;
                break;

            case Beat::FADE:
                actor.fade_total = key.b;
                actor.fade_frames = key.b;
                actor.sprite->set_blending_enabled(true);
                break;

            case Beat::FALL:
                actor.fall_total = key.b;
                actor.fall_frames = key.b;
                actor.fall_angle = key.a;
                break;

            default:
                break;
            }
        }
    }
}

void BattleScene::_animate(bool frozen)
{
    // Screen: fade in from black, flashes, fade out to black.
    if(_frame <= fade_frames)
    {
        set_fades(bn::color(0, 0, 0), bn::fixed(fade_frames - _frame) / fade_frames);
    }
    else if(_frame >= _end - fade_frames)
    {
        set_fades(bn::color(0, 0, 0), bn::fixed(_frame - (_end - fade_frames)) / fade_frames);
    }
    else if(_flash_frames > 0)
    {
        --_flash_frames;
        set_fades(bn::color(31, 31, 31), bn::fixed(_flash_frames) / _flash_total);
    }
    else
    {
        set_fades(bn::color(0, 0, 0), 0);
    }

    // Camera: follows the attacker's advance (eased), plus the shake. The backdrop moves at half the rate of the
    // stage (parallax); its hidden margins mirror the art, so a few pixels of travel never show an edge.
    Actor& attacker = _actors[ATTACKER];
    bn::fixed target = (attacker.x - attacker_home) / 10;
    target = target < -8 ? bn::fixed(-8) : target > 8 ? bn::fixed(8) : target;
    _camera += (target - _camera) / 4;
    int shake_x = 0;
    int shake_y = 0;

    if(_shake_frames > 0)
    {
        --_shake_frames;
        shake_x = (_shake_frames & 1) ? _shake_amplitude : -_shake_amplitude;
        shake_y = (_shake_frames & 2) ? 1 : -1;
    }

    const bn::fixed stage_x = -_camera + shake_x;
    _backdrop.set_position(8 - _camera / 2 + shake_x, 48 + shake_y);
    _effects.set_offset(stage_x, shake_y);

    for(int i = 0; i < _floor.size(); ++i)
    {
        _floor[i].set_position(-16 + i * 32 - 120 + stage_x, floor_top + 16 - 80 + shake_y);
    }

    for(int i = 0; i < 2; ++i)
    {
        Actor& actor = _actors[i];
        int bob = 0;

        if(! frozen && actor.move_frames > 0)
        {
            --actor.move_frames;
            actor.x += actor.step;

            if(_frame < _attack_start)
            {
                bob = (actor.move_frames >> 2) & 1;     // running steps
            }
            else if(i == ATTACKER && (actor.move_frames & 1) && bn::abs(actor.step) > 2)
            {
                // Motion trail behind a charging attacker.
                _effects.spawn(SMOKE, actor.x - 120 - (actor.step > 0 ? 18 : -18), ground_y - 6 - 80, 0,
                               bn::fixed(-0.3), 10, 1);
            }
        }

        // Idle poses breathe.
        int frame = actor.pose;

        if(frame == IDLE && actor.move_frames == 0)
        {
            frame = ((_frame + i * 9) / 18) & 1 ? BREATH : IDLE;
        }

        if(frame != actor.shown)
        {
            actor.shown = frame;
            actor.sprite->set_tiles(battle_item(actor.type).tiles_item(), frame);
        }

        bn::fixed x = actor.x;
        bn::fixed y = ground_y - body_height - bob;

        if(actor.fall_total > 0)
        {
            if(! frozen && actor.fall_frames > 0)
            {
                --actor.fall_frames;
            }

            // Topples backwards around the feet: the victim (facing left) clockwise, the attacker anticlockwise.
            const int done = actor.fall_total - actor.fall_frames;
            const int angle = actor.fall_angle * done / actor.fall_total;
            const bn::fixed sine = bn::degrees_lut_sin(angle);
            const bn::fixed cosine = bn::degrees_lut_cos(angle);
            x += (i == VICTIM ? sine : -sine) * body_height;
            y = ground_y - cosine * body_height;
            actor.sprite->set_rotation_angle(i == VICTIM ? (360 - angle) % 360 : angle);
        }

        if(! frozen && actor.fade_frames > 0)
        {
            --actor.fade_frames;
            bn::blending::set_transparency_alpha(bn::fixed(actor.fade_frames) / actor.fade_total);

            if(actor.fade_frames == 0)
            {
                actor.sprite->set_visible(false);
                actor.shadow->set_visible(false);
            }
        }

        actor.sprite->set_position(x - 120 + stage_x, y - 80 + shake_y);
        actor.shadow->set_position(actor.x - 120 + stage_x, ground_y - 1 - 80 + shake_y);
    }

    if(_projectile && ! frozen)
    {
        if(_projectile_frames > 0)
        {
            --_projectile_frames;
            _projectile_x += _projectile_step;
            _effects.spawn(_projectile_frame, _projectile_x - 120, chest_y - 80, 0, 0, 6, 0);
        }

        if(_projectile_frames == 0)
        {
            _projectile.reset();
        }
    }

    if(_projectile)
    {
        _projectile->set_position(_projectile_x - 120 + stage_x, chest_y - 80 + shake_y);
    }

    if(_circle)
    {
        _circle->set_center((attacker.x + stage_x).integer(), ground_y - body_height + shake_y);
        _circle->update();

        if(_frame > _hit + 6)
        {
            _circle.reset();
        }
    }

    _effects.update();
}

}
