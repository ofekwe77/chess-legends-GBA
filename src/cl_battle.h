// CHESS LEGENDS: ARCANE WARS - capture battle scenes.
//
// A capture accepted by the rules engine is shown as a side-view duel: the attacking class performs its attack and
// the captured class its defeat reaction. The scene decides nothing - the game state is already final - so
// skipping it leaves exactly the same state. Timelines are data: keys at frames that change sprite frames, move
// actors, flash the palettes, shake or pan the backdrop, freeze the action at the impact, play sounds, spawn
// particles or projectiles, fade or topple an actor. A sequence runs: entrance, attack, impact, defeat, victory.
#ifndef CL_BATTLE_H
#define CL_BATTLE_H

#include "bn_fixed.h"
#include "bn_optional.h"
#include "bn_regular_bg_ptr.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "cl_art.h"
#include "cl_layers.h"
#include "cl_types.h"

namespace cl::gba
{

class BattleScene
{
public:
    // `panel` is the caller's window layer (used for the caption banner while the scene runs). `fast` plays the
    // sequence at double speed without the impact freeze.
    BattleScene(PieceType attacker, Color attacker_color, PieceType victim, Color victim_color, Backdrop backdrop,
                int dark_faction, bool fast, PanelLayer& panel);
    ~BattleScene();

    // Advances the scene (A or B skips it); returns false once it has finished.
    bool update();

    // Total length in frames at normal speed (tests, pacing checks).
    [[nodiscard]] int length() const
    {
        return _end;
    }

private:
    struct Actor
    {
        bn::optional<bn::sprite_ptr> sprite;
        bn::optional<bn::sprite_ptr> shadow;
        PieceType type = PAWN;
        bn::fixed x;            // screen x of the feet
        bn::fixed step;
        int pose = 0;           // frame set by the timeline (idle poses breathe)
        int shown = 0;          // frame currently displayed
        int move_frames = 0;
        int fall_frames = 0;
        int fall_total = 0;
        int fall_angle = 0;
        int fade_frames = 0;
        int fade_total = 0;
    };

    bn::regular_bg_ptr _backdrop;
    PanelLayer& _panel;
    bn::vector<bn::sprite_ptr, 9> _floor;
    Actor _actors[2];
    EffectLayer _effects;
    bn::optional<ParticleField> _circle;
    bn::vector<bn::sprite_ptr, 20> _caption;
    bn::optional<bn::sprite_ptr> _projectile;
    bn::fixed _projectile_x;
    bn::fixed _projectile_step;
    bn::fixed _camera;
    bn::fixed _camera_step;
    int _camera_frames = 0;
    int _projectile_frames = 0;
    int _projectile_frame = 0;
    int _frame = 0;
    int _attack_start = 0;
    int _hit = 0;
    int _victory = 0;
    int _end = 0;
    int _hold = 0;
    int _flash_frames = 0;
    int _flash_total = 0;
    int _shake_frames = 0;
    int _shake_amplitude = 0;
    bool _fast = false;

    bool _step();
    void _run_keys();
    void _animate(bool frozen);
};

}

#endif
