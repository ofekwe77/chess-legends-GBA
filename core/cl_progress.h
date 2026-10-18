// CHESS LEGENDS: ARCANE WARS - achievements, statistics, gallery and cosmetic unlocks.
//
// Every achievement has a stable identifier, a title, a description, a concrete trigger, an icon and a bit in the
// save profile. Triggers come only from validated game events (ActionInfo returned by Game::apply), final match
// results, and stored progress facts. Gallery entries and board skins are pure functions of the stored progress, so
// an unlock can never disagree with what earned it. Nothing here grants gameplay power.
#ifndef CL_PROGRESS_H
#define CL_PROGRESS_H

#include "cl_ai.h"
#include "cl_campaign.h"
#include "cl_save.h"

namespace cl
{

// ------------------------------------------------------------------------------------------------ achievements

enum class Achievement : uint8_t
{
    // beginner
    FIRST_VICTORY, APT_PUPIL, FIRST_CHECK, QUEENSBANE,
    // classic chess
    CLASSICIST, CHECKMATE_ARTIST, CORONATION, SAFE_HARBOR, IN_PASSING,
    // arcane chess
    FIRST_SPARK, SHADOW_STEPPER, PHASE_WALKER, VEIL_WALKER, MOVING_WALLS, ROYAL_DECREE, GREAT_ESCAPE,
    // strategy
    GIANT_SLAYER, GRANDMASTER_SLAYER, MANA_MISER, SPELLWEAVER, UNSHAKEN, DOUBLE_TROUBLE, FLAWLESS, MARATHON,
    // campaign
    WARDENS_FALL, THREE_CROWNS, CROWN_RESTORED,
    // other modes
    PUZZLE_SOLVER, GRAND_TACTICIAN, GAUNTLET_CHAMPION, FRIENDLY_RIVALS, LOREKEEPER,
    COUNT
};

constexpr int achievements_count = int(Achievement::COUNT);
static_assert(achievements_count <= 64, "achievements are stored in a 64-bit mask");

enum class AchievementCategory : uint8_t
{
    BEGINNER,
    CLASSIC,
    ARCANE,
    STRATEGY,
    CAMPAIGN,
    MODES
};

struct AchievementInfo
{
    const char* key;                // stable identifier (documentation, tests)
    const char* title;
    const char* description;        // the exact trigger, as shown to the player
    AchievementCategory category;
    uint8_t icon;                   // index in the achievement icon sheet
};

[[nodiscard]] const AchievementInfo& achievement_info(Achievement achievement);

[[nodiscard]] constexpr uint64_t achievement_bit(Achievement achievement)
{
    return uint64_t(1) << int(achievement);
}

[[nodiscard]] inline bool has_achievement(const SaveProgress& progress, Achievement achievement)
{
    return progress.achievements & achievement_bit(achievement);
}

// What kind of match is played (decides which achievements and statistics it can produce).
enum class MatchKind : uint8_t
{
    CLASSIC,        // quick Classic Chess match (vs computer or local two-player)
    ARCANE,         // quick Arcane Chess match
    STORY,          // campaign encounter
    CHALLENGE,      // tactical puzzle (counts for move events only, not for victories)
    GAUNTLET,       // boss gauntlet stage
    TUTORIAL        // guided lesson (counts for move events only)
};

struct MatchContext
{
    MatchKind kind = MatchKind::ARCANE;
    uint8_t human_mask = 1;         // bit per color controlled by a person (3 = local two-player)
    Difficulty difficulty = Difficulty::KNIGHT;     // computer opponent's level (single-player)
    bool boss = false;              // story boss encounter or gauntlet stage
};

// Records the validated events of one match: statistics and the achievements they earn.
class MatchRecorder
{
public:
    void start(const MatchContext& context);

    // After every applied action (Game::apply), with the resulting game.
    void on_action(const ActionInfo& info, const Game& game);

    // Once, when the match has a final result. `objective_success`: the scenario objective was met (story,
    // challenge, gauntlet); for quick matches pass whether a person won.
    void on_finish(const Game& game, bool objective_success);

    // Achievements earned so far in this match (shown as they happen).
    [[nodiscard]] uint64_t earned() const
    {
        return _earned;
    }

    [[nodiscard]] bool finished() const
    {
        return _finished;
    }

    // Adds this match's statistics (finished matches only) and earned achievements to the profile. Returns the
    // achievements that were newly unlocked by this call.
    uint64_t commit(Profile& profile) const;

    // Per-match counters for suspended matches (the context is rebuilt from the suspended match setup).
    static constexpr int packed_size = 20;
    void pack(uint8_t* out) const;
    bool unpack(const uint8_t* in);

    [[nodiscard]] const MatchContext& context() const
    {
        return _context;
    }

private:
    MatchContext _context;
    uint64_t _earned = 0;
    uint16_t _actions = 0;
    uint8_t _spells_cast = 0;       // abilities cast by people
    bool _person_lost_material = false;
    bool _finished = false;
    bool _person_won = false;
    bool _person_lost = false;

    // statistics of people's actions in this match
    uint16_t _captures = 0;
    uint16_t _checks = 0;
    uint8_t _promotions = 0;
    uint8_t _castles = 0;
    uint8_t _en_passants = 0;
    uint8_t _queens = 0;
    uint8_t _spell_counts[ABILITIES_COUNT] = {};
    bool _checkmate = false;

    [[nodiscard]] bool person(Color color) const
    {
        return (_context.human_mask >> int(color)) & 1;
    }
};

// Achievements that follow from stored progress (campaign, challenges, gauntlet, tutorial, gallery).
[[nodiscard]] uint64_t progress_achievements(const Profile& profile);

// Sets the bits; returns the newly unlocked ones.
uint64_t unlock_achievements(SaveProgress& progress, uint64_t earned);

// ------------------------------------------------------------------------------------------------ progress facts

[[nodiscard]] bool encounter_cleared(const SaveProgress& progress, int encounter);
[[nodiscard]] int bosses_defeated(const SaveProgress& progress);
[[nodiscard]] bool tutorial_complete(const SaveProgress& progress);

// Story map: an encounter is playable once every earlier encounter is cleared.
[[nodiscard]] bool encounter_unlocked(const SaveProgress& progress, int encounter);

// Records a cleared encounter (best move count) and advances the unlocked world.
void record_encounter_cleared(SaveProgress& progress, int encounter, int player_moves);

[[nodiscard]] int challenge_medal_of(const SaveProgress& progress, int challenge);
void record_challenge_medal(SaveProgress& progress, int challenge, int medal);

// ------------------------------------------------------------------------------------------------ cosmetics

constexpr int board_themes_count = 8;   // royal, six kingdom boards, high contrast

// Royal and high contrast are always available; each kingdom board is earned by defeating its ruler.
[[nodiscard]] bool board_theme_unlocked(const SaveProgress& progress, int theme);

// ------------------------------------------------------------------------------------------------ gallery

enum class GallerySection : uint8_t
{
    CHARACTERS,
    KINGDOMS,
    MUSIC,
    SPELLS
};

constexpr int gallery_characters = int(Character::NARRATOR);   // 14
constexpr int gallery_kingdoms = worlds_count;                // 6
constexpr int gallery_music = music_tracks_count;             // 13
constexpr int gallery_spells = ABILITIES_COUNT;               // 6
constexpr int gallery_entries = gallery_characters + gallery_kingdoms + gallery_music + gallery_spells;

[[nodiscard]] int gallery_section_size(GallerySection section);
[[nodiscard]] bool gallery_unlocked(const Profile& profile, GallerySection section, int index);
[[nodiscard]] int gallery_unlocked_count(const Profile& profile);

// Lore text for characters and kingdoms; unlock hint for locked entries.
[[nodiscard]] const char* character_lore(Character character);
[[nodiscard]] const char* kingdom_lore(int world);
[[nodiscard]] const char* gallery_unlock_hint(GallerySection section, int index);

// Soundtrack titles (gallery music player order = audio track order).
[[nodiscard]] const char* music_title(int track);

}

#endif
