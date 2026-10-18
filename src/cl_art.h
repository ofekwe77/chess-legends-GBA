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

[[nodiscard]] constexpr Backdrop world_backdrop(int world)
{
    return Backdrop(world < 0 || world > 5 ? 7 : world);
}

// Full-screen backdrop aligned with the screen (top-left of the art at the top-left of the screen).
[[nodiscard]] bn::regular_bg_ptr create_backdrop(Backdrop backdrop, int bg_priority = 3);

// 64x64 portrait with three frames: Expression::NEUTRAL, INTENSE, DEFEATED.
[[nodiscard]] const bn::sprite_item& portrait_item(Character character);

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
