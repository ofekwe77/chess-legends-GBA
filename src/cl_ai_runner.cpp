// CHESS LEGENDS: ARCANE WARS - runs the AI on the console without freezing the interface.
#include "cl_ai_runner.h"

#include "bn_timer.h"
#include "bn_timers.h"

#include "cl_globals.h"

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

    bool poll(void* /*user*/, uint32_t /*nodes*/)
    {
        // Yield once ~85% of a frame has been spent searching: bn::core::update() then waits for the next VBlank.
        constexpr int budget = bn::timers::ticks_per_frame() * 85 / 100;

        if(job.timer->elapsed_ticks() >= budget)
        {
            ++job.frames;

            if(job.frame_function(job.user))
            {
                job.cancelled = true;
            }

            job.timer->restart();
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
    job.settings.poll_mask = 7;
    job.standard_start = standard_start;
    job.frame_function = frame_function;
    job.user = user;
    job.decision = AiDecision();
    job.timer = &timer;
    job.frames = 0;
    job.cancelled = false;

    cl_call_with_stack(ai_stack + ai_stack_words, run_job, nullptr);

    ThinkReport report;
    report.decision = job.decision;
    report.frames = job.frames;
    report.cancelled = job.cancelled;
    return report;
}

}
