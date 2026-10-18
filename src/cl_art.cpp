// CHESS LEGENDS: ARCANE WARS - shared presentation helpers.
#include "cl_art.h"

#include "bn_math.h"

#include "bn_regular_bg_items_backdrop_frozen.h"
#include "bn_regular_bg_items_backdrop_inferno.h"
#include "bn_regular_bg_items_backdrop_map.h"
#include "bn_regular_bg_items_backdrop_nexus.h"
#include "bn_regular_bg_items_backdrop_storm.h"
#include "bn_regular_bg_items_backdrop_title.h"
#include "bn_regular_bg_items_backdrop_umbral.h"
#include "bn_regular_bg_items_backdrop_verdant.h"
#include "bn_sprite_items_particles.h"
#include "bn_sprite_items_portrait_arden.h"
#include "bn_sprite_items_portrait_aurelia.h"
#include "bn_sprite_items_portrait_brin.h"
#include "bn_sprite_items_portrait_elsk.h"
#include "bn_sprite_items_portrait_harrik.h"
#include "bn_sprite_items_portrait_noctis.h"
#include "bn_sprite_items_portrait_orrin.h"
#include "bn_sprite_items_portrait_quill.h"
#include "bn_sprite_items_portrait_rael.h"
#include "bn_sprite_items_portrait_rowan.h"
#include "bn_sprite_items_portrait_seraph.h"
#include "bn_sprite_items_portrait_tamsin.h"
#include "bn_sprite_items_portrait_veyra.h"
#include "bn_sprite_items_portrait_ysra.h"

namespace cl::gba
{

namespace
{
    // Frame indices in particles.bmp (see assets_source/gen_effects.py).
    enum ParticleFrame
    {
        LEAF_A, LEAF_B, EMBER_A, EMBER_B, SNOW_A, SNOW_B, RUNE_A, RUNE_B, RAIN, SPARK_A, SPARK_B, FRAGMENT, STAR,
        ORB, SMOKE, SHARD
    };
}

bn::regular_bg_ptr create_backdrop(Backdrop backdrop, int bg_priority)
{
    const bn::regular_bg_item* item;

    switch(backdrop)
    {
    case Backdrop::VERDANT: item = &bn::regular_bg_items::backdrop_verdant; break;
    case Backdrop::INFERNO: item = &bn::regular_bg_items::backdrop_inferno; break;
    case Backdrop::FROZEN: item = &bn::regular_bg_items::backdrop_frozen; break;
    case Backdrop::UMBRAL: item = &bn::regular_bg_items::backdrop_umbral; break;
    case Backdrop::STORM: item = &bn::regular_bg_items::backdrop_storm; break;
    case Backdrop::NEXUS: item = &bn::regular_bg_items::backdrop_nexus; break;
    case Backdrop::MAP: item = &bn::regular_bg_items::backdrop_map; break;
    default: item = &bn::regular_bg_items::backdrop_title; break;
    }

    // A 256x256 BG is centred on the screen at (0, 0); shifting it by (8, 48) aligns its top-left corner.
    bn::regular_bg_ptr bg = item->create_bg(8, 48);
    bg.set_priority(bg_priority);
    return bg;
}

const bn::sprite_item& portrait_item(Character character)
{
    switch(character)
    {
    case Character::ARDEN: return bn::sprite_items::portrait_arden;
    case Character::ORRIN: return bn::sprite_items::portrait_orrin;
    case Character::ROWAN: return bn::sprite_items::portrait_rowan;
    case Character::VEYRA: return bn::sprite_items::portrait_veyra;
    case Character::YSRA: return bn::sprite_items::portrait_ysra;
    case Character::NOCTIS: return bn::sprite_items::portrait_noctis;
    case Character::RAEL: return bn::sprite_items::portrait_rael;
    case Character::AURELIA: return bn::sprite_items::portrait_aurelia;
    case Character::BRIN: return bn::sprite_items::portrait_brin;
    case Character::HARRIK: return bn::sprite_items::portrait_harrik;
    case Character::ELSK: return bn::sprite_items::portrait_elsk;
    case Character::QUILL: return bn::sprite_items::portrait_quill;
    case Character::TAMSIN: return bn::sprite_items::portrait_tamsin;
    default: return bn::sprite_items::portrait_seraph;
    }
}

// ------------------------------------------------------------------------------------------------ particles

ParticleField::ParticleField(ParticleKind kind, int count, int bg_priority, uint32_t seed) :
    _kind(kind),
    _state(seed * 2654435761u + 1)
{
    count = count > max_particles ? max_particles : count;

    for(int i = 0; i < count; ++i)
    {
        Particle particle;
        particle.phase = _random(2048);
        _particles.push_back(particle);
        bn::sprite_ptr sprite = bn::sprite_items::particles.create_sprite(0, 0, 0);
        sprite.set_bg_priority(bg_priority);
        sprite.set_z_order(-10);
        _sprites.push_back(bn::move(sprite));
        _place(i);

        // Spread the falling / rising kinds over the whole screen from the first frame.
        if(kind == ParticleKind::LEAVES || kind == ParticleKind::SNOW || kind == ParticleKind::RAIN)
        {
            _particles[i].y = _random(168);
        }
        else if(kind == ParticleKind::EMBERS)
        {
            _particles[i].y = 20 + _random(150);
        }
    }

    update();
}

int ParticleField::_random(int range)
{
    _state ^= _state << 13;
    _state ^= _state >> 17;
    _state ^= _state << 5;
    return int((_state >> 8) % uint32_t(range));
}

void ParticleField::_place(int index)
{
    Particle& p = _particles[index];

    switch(_kind)
    {
    case ParticleKind::LEAVES:
        p.x = _random(240);
        p.y = -8 - _random(40);
        p.vx = 0;
        p.vy = bn::fixed(0.35) + bn::fixed(_random(40)) / 100;
        break;

    case ParticleKind::EMBERS:
        p.x = _random(240);
        p.y = 168;
        p.vx = 0;
        p.vy = -(bn::fixed(0.4) + bn::fixed(_random(60)) / 100);
        break;

    case ParticleKind::SNOW:
        p.x = _random(240);
        p.y = -8 - _random(40);
        p.vx = 0;
        p.vy = bn::fixed(0.2) + bn::fixed(_random(30)) / 100;
        break;

    case ParticleKind::RAIN:
        p.x = _random(280);
        p.y = -8 - _random(60);
        p.vx = bn::fixed(-1.2);
        p.vy = bn::fixed(3.5) + bn::fixed(_random(100)) / 100;
        break;

    default:
        p.x = _random(240);
        p.y = _random(150);
        p.vx = 0;
        p.vy = 0;
        break;
    }
}

void ParticleField::set_center(int x, int y)
{
    _center_x = x;
    _center_y = y;
}

void ParticleField::set_visible(bool visible)
{
    for(bn::sprite_ptr& sprite : _sprites)
    {
        sprite.set_visible(visible);
    }
}

void ParticleField::update()
{
    ++_frame;
    const int count = _particles.size();

    for(int i = 0; i < count; ++i)
    {
        Particle& p = _particles[i];
        bn::sprite_ptr& sprite = _sprites[i];
        int frame = 0;
        bool visible = true;

        switch(_kind)
        {
        case ParticleKind::LEAVES:
            p.phase = (p.phase + 12) & 2047;
            p.x += bn::lut_sin(p.phase) / 2;
            p.y += p.vy;
            frame = ((_frame + i * 7) >> 4) & 1 ? LEAF_B : LEAF_A;

            if(p.y > 168)
            {
                _place(i);
            }
            break;

        case ParticleKind::EMBERS:
            p.phase = (p.phase + 40) & 2047;
            p.x += bn::lut_sin(p.phase) / 3;
            p.y += p.vy;
            frame = ((_frame + i) >> 2) & 1 ? EMBER_B : EMBER_A;

            if(p.y < -8)
            {
                _place(i);
            }
            break;

        case ParticleKind::SNOW:
            p.phase = (p.phase + 8) & 2047;
            p.x += bn::lut_sin(p.phase) / 4;
            p.y += p.vy;
            frame = (i & 1) ? SNOW_B : SNOW_A;

            if(p.y > 168)
            {
                _place(i);
            }
            break;

        case ParticleKind::RAIN:
            p.x += p.vx;
            p.y += p.vy;
            frame = RAIN;

            if(p.y > 168)
            {
                _place(i);
            }
            break;

        case ParticleKind::RUNES:
        case ParticleKind::FRAGMENTS:
        {
            const int angle = (p.phase + _frame * (_kind == ParticleKind::RUNES ? 6 : 4) + i * (2048 / count)) & 2047;
            const int radius = _kind == ParticleKind::RUNES ? 44 + (i % 3) * 6 : 34;
            p.x = _center_x + bn::lut_sin((angle + 512) & 2047) * radius;
            p.y = _center_y + bn::lut_sin(angle) * (radius * 2 / 3);
            frame = _kind == ParticleKind::RUNES ? (((_frame >> 3) + i) & 1 ? RUNE_B : RUNE_A) : FRAGMENT;
            break;
        }

        default:    // SPARKLES / STARS twinkle in place
        {
            const int cycle = ((_frame + p.phase) >> 3) & 7;
            visible = cycle < 5;
            frame = _kind == ParticleKind::STARS ? STAR : (cycle & 1 ? SPARK_B : SPARK_A);

            if(cycle == 7 && ((_frame + p.phase) & 7) == 0)
            {
                _place(i);
            }
            break;
        }
        }

        if(frame != p.frame)
        {
            p.frame = frame;
            sprite.set_tiles(bn::sprite_items::particles.tiles_item(), frame);
        }

        sprite.set_position(p.x - 120 + 4, p.y - 80 + 4);
        sprite.set_visible(visible);
    }
}

}
