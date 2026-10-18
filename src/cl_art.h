// CHESS LEGENDS: ARCANE WARS - shared presentation helpers: backdrops, portraits and particle fields.
#ifndef CL_ART_H
#define CL_ART_H

#include "bn_fixed.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_item.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "cl_campaign.h"

namespace cl::gba
{

enum class Backdrop : uint8_t
{
    VERDANT,
    INFERNO,
    FROZEN,
    UMBRAL,
    STORM,
    NEXUS,
    MAP,
    TITLE
};

// Frame indices in particles.bmp (assets_source/gen_effects.py).
namespace particle
{
    enum Frame : uint8_t
    {
        LEAF_A, LEAF_B, EMBER_A, EMBER_B, SNOW_A, SNOW_B, RUNE_A, RUNE_B, RAIN, SPARK_A, SPARK_B, FRAGMENT, STAR,
        ORB, SMOKE, SHARD
    };
}

[[nodiscard]] constexpr Backdrop world_backdrop(int world)
{
    return Backdrop(world < 0 || world > 5 ? 7 : world);
}

// Full-screen backdrop aligned with the screen (top-left of the art at the top-left of the screen).
[[nodiscard]] bn::regular_bg_ptr create_backdrop(Backdrop backdrop, int bg_priority = 3);

// 64x64 portrait with three frames: Expression::NEUTRAL, INTENSE, DEFEATED.
[[nodiscard]] const bn::sprite_item& portrait_item(Character character);

// 16x16 animated effects (spell_fx sheet).
enum class Fx : uint8_t
{
    SMOKE,          // a violet cloud that spreads and thins out
    RING,           // an expanding white ring (arrival, shockwave)
    GOLD_RING,      // an expanding golden ring (royal magic)
    FLARE,          // a star flare (teleport, rematerialization)
    GLOW            // a spectral aura (one frame: spawn it again to keep it on a moving piece)
};

// Short-lived effect sprites in Butano coordinates (screen centre = 0, 0): 8x8 particles (particles sheet frames)
// with a velocity and a lifetime - impact bursts, smoke, trails, sparks - and 16x16 animated effects.
class EffectLayer
{
public:
    void spawn(int frame, bn::fixed x, bn::fixed y, bn::fixed vx, bn::fixed vy, int life, int bg_priority = 0);

    // `count` particles flying out of (x, y) in all directions.
    void burst(int frame, bn::fixed x, bn::fixed y, int count, bn::fixed speed, int life, int bg_priority = 0);

    // A 16x16 effect centred on (x, y), showing each of its frames for `step` frames; `behind` draws it under
    // sprites of the same priority (auras behind pieces).
    void fx(Fx kind, bn::fixed x, bn::fixed y, int step = 4, int bg_priority = 0, bool behind = false);

    void update();
    void clear();

    // Displacement applied when drawing (camera pans and screen shakes move the particles with the scene).
    void set_offset(bn::fixed x, bn::fixed y)
    {
        _offset_x = x;
        _offset_y = y;
    }

    [[nodiscard]] bool empty() const
    {
        return _sprites.empty() && _fx_sprites.empty();
    }

private:
    struct Particle
    {
        bn::fixed x;
        bn::fixed y;
        bn::fixed vx;
        bn::fixed vy;
        int life = 0;
    };

    struct Effect
    {
        bn::fixed x;
        bn::fixed y;
        Fx kind = Fx::SMOKE;
        uint8_t index = 0;      // position in the kind's frame sequence
        uint8_t step = 4;
        uint8_t timer = 0;
    };

    static constexpr int max_particles = 28;
    static constexpr int max_effects = 10;

    bn::vector<bn::sprite_ptr, max_particles> _sprites;
    bn::vector<Particle, max_particles> _particles;
    bn::vector<bn::sprite_ptr, max_effects> _fx_sprites;
    bn::vector<Effect, max_effects> _effects;
    bn::fixed _offset_x;
    bn::fixed _offset_y;
    int _spawned = 0;
};

// A 64x64 position preview (8 px squares, mini piece sprites); (x, y) is its top-left corner on screen.
class MiniBoard
{
public:
    MiniBoard(int x, int y, int bg_priority = 0);

    void show(const Position& pos);
    void set_visible(bool visible);

private:
    bn::sprite_ptr _board;
    bn::vector<bn::sprite_ptr, 32> _pieces;
    int _x;
    int _y;
    int _bg_priority;
};

enum class ParticleKind : uint8_t
{
    LEAVES,         // drifting down and swaying
    EMBERS,         // rising and flickering
    SNOW,           // falling slowly
    RUNES,          // orbiting a centre
    RAIN,           // fast diagonal streaks
    SPARKLES,       // twinkling in place
    FRAGMENTS,      // crown fragments circling a centre
    STARS           // twinkling stars
};

// A small field of particle sprites (cinematics, celebrations). Motion is deterministic for a given seed.
class ParticleField
{
public:
    ParticleField(ParticleKind kind, int count, int bg_priority = 1, uint32_t seed = 1);

    void update();
    void set_center(int x, int y);
    void set_visible(bool visible);

private:
    struct Particle
    {
        bn::fixed x;
        bn::fixed y;
        bn::fixed vx;
        bn::fixed vy;
        int phase = 0;
        int frame = -1;     // sprite frame currently shown (tiles change only when it changes)
    };

    static constexpr int max_particles = 24;

    ParticleKind _kind;
    bn::vector<bn::sprite_ptr, max_particles> _sprites;
    bn::vector<Particle, max_particles> _particles;
    int _frame = 0;
    int _center_x = 120;
    int _center_y = 64;
    uint32_t _state;

    int _random(int range);
    void _place(int index);
};

}

#endif
