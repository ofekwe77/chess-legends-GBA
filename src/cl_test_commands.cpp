// CHESS LEGENDS: ARCANE WARS - diagnostic mailbox of the test interface.
#include "cl_test_commands.h"

#include "cl_progress.h"
#include "cl_scene.h"
#include "cl_storage.h"

namespace cl::gba
{

namespace
{
    uint32_t pending_move = 0;

    void unlock_everything()
    {
        Profile& profile = globals().profile;
        SaveProgress& progress = profile.progress;

        for(int e = 0; e < campaign_encounters; ++e)
        {
            record_encounter_cleared(progress, e, 20);
        }

        for(int c = 0; c < challenges_count; ++c)
        {
            record_challenge_medal(progress, c, 3);
        }

        progress.worlds_seen = 0x3F;
        progress.music_unlocked = uint16_t((1u << music_tracks_count) - 1);
        progress.tutorial_lessons = uint16_t((1u << tutorial_lessons_count) - 1);
        progress.intro_seen = true;
        progress.ending_seen = true;

        // Every spell counts as cast once (gallery demonstrations).
        for(uint32_t& count : profile.stats.spells)
        {
            count = count ? count : 1;
        }

        save_profile_now();
    }
}

bn::optional<SceneId> process_test_commands()
{
    cl_test_interface& ti = test_interface();
    const uint32_t command = ti.cmd;

    if(command == CL_TI_CMD_NONE || ti.ack == command)
    {
        return bn::nullopt;
    }

    Globals& g = globals();
    MatchSetup& setup = g.setup;
    bn::optional<SceneId> next;

    switch(command)
    {
    case CL_TI_CMD_SET_SEED:
        g.seed = ti.arg0;
        break;

    case CL_TI_CMD_START_MATCH:
        setup = MatchSetup();
        setup.mode = ti.arg0 ? MatchMode::ARCANE : MatchMode::CLASSIC;
        setup.arcane = ti.arg0 != 0;
        setup.human_mask = uint8_t(ti.arg1 ? ti.arg1 & 3 : 1);
        setup.difficulty = Difficulty(ti.arg2 & 3);
        setup.board_theme = uint8_t(ti.arg3 & 7);
        next = SceneId::MATCH;
        break;

    case CL_TI_CMD_PLAY_MOVE:
        pending_move = ti.arg0;
        break;

    case CL_TI_CMD_SET_ANIMATIONS:
        g.profile.settings.animations = uint8_t(ti.arg0 & 3);
        break;

    case CL_TI_CMD_GOTO_SCENE:
        if(ti.arg0 < uint32_t(SceneId::COUNT))
        {
            if(SceneId(ti.arg0) == SceneId::GAUNTLET)
            {
                setup.gauntlet_stage = -1;   // a new run
            }

            next = SceneId(ti.arg0);
        }
        break;

    case CL_TI_CMD_UNLOCK_ALL:
        unlock_everything();
        break;

    case CL_TI_CMD_RESET_STATS:
        reset_cpu_statistics();
        break;

    case CL_TI_CMD_START_ENCOUNTER:
        if(ti.arg0 < uint32_t(campaign_encounters))
        {
            setup = MatchSetup();
            setup.mode = MatchMode::STORY;
            setup.encounter = int8_t(ti.arg0);
            next = SceneId::MATCH;
        }
        break;

    case CL_TI_CMD_START_CHALLENGE:
        if(ti.arg0 < uint32_t(challenges_count))
        {
            setup = MatchSetup();
            setup.mode = MatchMode::CHALLENGE;
            setup.challenge = int8_t(ti.arg0);
            next = SceneId::MATCH;
        }
        break;

    case CL_TI_CMD_WIPE_SAVE:
        erase_all_saves();
        g.profile = Profile();
        g.toasts = 0;
        ti.suspend_present = 0;
        break;

    default:
        break;
    }

    ti.ack = command;
    return next;
}

uint32_t take_test_move()
{
    const uint32_t move = pending_move;
    pending_move = 0;
    return move;
}

}
