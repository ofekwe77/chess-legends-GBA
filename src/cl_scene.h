// CHESS LEGENDS: ARCANE WARS - scenes, per-frame services and input helpers.
#ifndef CL_SCENE_H
#define CL_SCENE_H

#include "bn_optional.h"
#include "bn_timer.h"
#include "bn_sprite_ptr.h"
#include "bn_unique_ptr.h"
#include "bn_vector.h"

#include "cl_globals.h"

namespace cl::gba
{

class Scene
{
public:
    virtual ~Scene() = default;

    // Runs one frame of logic; returns the next scene to switch to, if any.
    [[nodiscard]] virtual bn::optional<SceneId> update() = 0;
};

[[nodiscard]] bn::unique_ptr<Scene> create_scene(SceneId id);

// Ends the current frame: refreshes the test interface and calls bn::core::update().
void frame_end();

// Current scene id (mirrored to the test interface).
void set_current_scene(SceneId id);

// Restarts the CPU usage peak, sprite peak and missed frame measurements (test interface).
void reset_cpu_statistics();

// Profiled sections: the worst duration of each since the last statistics reset is published to the test
// interface (prof_*) in percent of a frame.
enum class ProfileSlot : uint8_t
{
    PANEL,          // match side panel redraw
    PIECES,         // board piece sprites rebuilt
    EXECUTE,        // one action applied and presented (animations off: up to the next turn)
    SEARCH_START,   // AI turn start to its first chance to yield a frame
    APPLY,          // the rules engine applying one action
    AFTER_ACTION,   // statistics, achievements, objectives after an action
    BEGIN_TURN,     // turn start (legal actions, status, hints, game over)
    STATUS,         // status line redraw
    BOARD_COMMIT,   // board squares / markers redrawn and committed
    COUNT
};

void profile_note(ProfileSlot slot, int timer_ticks);

// Measures the enclosing scope.
class ProfileScope
{
public:
    explicit ProfileScope(ProfileSlot slot) :
        _slot(slot)
    {
    }

    ~ProfileScope()
    {
        profile_note(_slot, _timer.elapsed_ticks());
    }

private:
    bn::timer _timer;
    ProfileSlot _slot;
};

// Fills the free part of the IWRAM stack with a known pattern. Called first thing at boot (interrupts are not yet
// enabled); frame_end then publishes how much of the stack has never been touched (test interface stack_free).
void paint_stack();

// Directional input with auto-repeat (cursor movement in menus and on the board).
class RepeatInput
{
public:
    void update();

    [[nodiscard]] bool left() const
    {
        return _fired & 1;
    }

    [[nodiscard]] bool right() const
    {
        return _fired & 2;
    }

    [[nodiscard]] bool up() const
    {
        return _fired & 4;
    }

    [[nodiscard]] bool down() const
    {
        return _fired & 8;
    }

private:
    int _held_frames[4] = {};
    int _fired = 0;
};

// Announces newly unlocked achievements (globals().toasts) one at a time, sliding a banner in at the top of the
// screen. Every scene owns one and calls update() once per frame.
class ToastBanner
{
public:
    // Returns true while a banner is shown. While `allowed` is false (a battle scene) no banner starts, and one
    // being shown is withdrawn and queued again, so it is seen in full later.
    bool update(bool allowed = true);

private:
    bn::vector<bn::sprite_ptr, 8> _banner;
    bn::vector<bn::sprite_ptr, 24> _text;
    int _frames = 0;
    int _id = 0;

    void _place(int y);
};

// Screen fades (blocking helpers used between scenes).
void fade_out(int frames = 16);
void fade_in(int frames = 16);

}

#endif
