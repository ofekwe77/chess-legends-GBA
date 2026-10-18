// CHESS LEGENDS: ARCANE WARS - runs the AI on the console without freezing the interface.
#include "cl_ai_runner.h"

#include "bn_core.h"
#include "bn_timer.h"
#include "bn_timers.h"

#include "cl_globals.h"
#include "cl_scene.h"

extern "C" void cl_call_with_stack(void* stack_top, void (*function)(void*), void* argument);

namespace cl::gba
{

namespace
{
    // Dedicated search stack (the deepest searches measured use ~3 KB; the guard keeps 768 bytes in reserve).
    constexpr int ai_stack_words = 8192 / 4;
    alignas(8) BN_DATA_EWRAM_BSS uint32_t ai_stack[ai_stack_words];

    struct Job
    {
        Game* game;
        AiSettings settings;
        bool standard_start;
        ThinkFrameFunction frame_function;
        void* user;
        AiDecision decision;
        bn::timer* timer;
        int frames;
        bool cancelled;
        bool started;       // the first poll has happened (search start measured)
    };

    Job job;

    // Lives in IWRAM (.bss): the position being searched stays in the fastest memory.
    TranspositionTable tt;
    bool tt_ready = false;

    Ai& ai_instance()
    {
        static Ai ai(globals().search_memory, tt);
        static bool guarded = false;

        if(! guarded)
        {
            ai.set_stack_limit(reinterpret_cast<uintptr_t>(ai_stack) + 768);
            guarded = true;
        }

        return ai;
    }

    // Yield once 70% of the current frame has been used (by the search and by whatever the scene did earlier in the
    // frame): the rest is Butano's update before the next VBlank. Polled at every node (~4% of a frame here).
    constexpr int frame_budget = bn::timers::ticks_per_frame() * 70 / 100;

    bool poll(void* /*user*/, uint32_t /*nodes*/)
    {

        if(! job.started)
        {
            job.started = true;
            profile_note(ProfileSlot::SEARCH_START, job.timer->elapsed_ticks());
        }

        if(bn::core::current_cpu_ticks() >= frame_budget)
        {
            ++job.frames;

            if(job.frame_function(job.user))
            {
                job.cancelled = true;
            }
        }

        return job.cancelled;
    }

    void run_job(void* /*data*/)
    {
        job.decision = ai_instance().choose(*job.game, job.settings, job.standard_start, poll, nullptr);
    }
}

ThinkReport ai_think(Game& game, const AiSettings& settings, bool standard_start, ThinkFrameFunction frame_function,
                     void* user)
{
    if(! tt_ready)
    {
        tt.init(globals().tt_entries, 4096);
        tt_ready = true;
    }

    bn::timer timer;
    job.game = &game;
    job.settings = settings;
    job.settings.poll_mask = 0;
    job.standard_start = standard_start;
    job.frame_function = frame_function;
    job.user = user;
    job.decision = AiDecision();
    job.timer = &timer;
    job.frames = 0;
    job.cancelled = false;
    job.started = false;

    cl_call_with_stack(ai_stack + ai_stack_words, run_job, nullptr);

    // Playing the chosen action costs a good part of a frame: it starts a fresh frame when this one is well used.
    if(! job.cancelled && bn::core::current_cpu_ticks() >= bn::timers::ticks_per_frame() / 3)
    {
        ++job.frames;
        job.cancelled = job.frame_function(job.user);
    }

    ThinkReport report;
    report.decision = job.decision;
    report.frames = job.frames;
    report.cancelled = job.cancelled;
    return report;
}

}
