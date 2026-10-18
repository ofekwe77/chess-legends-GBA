// CHESS LEGENDS: ARCANE WARS - persistent save data (format, serialization, integrity).
//
// The cartridge SRAM holds two kinds of records, each stored twice (A / B slots) so that an interrupted write never
// destroys the most recent valid copy:
//   * profile: settings, campaign progress, achievements, gallery unlocks and statistics;
//   * suspended match: the exact state needed to resume a match (window base position + actions since, scenario).
// Every record starts with a header (signature, format version, kind, sequence number, payload length, CRC-32).
// All fields are serialized explicitly (little endian), never by copying structures, so the format does not depend
// on compiler padding. This module works on byte buffers only; the console layer moves them to / from SRAM.
#ifndef CL_SAVE_H
#define CL_SAVE_H

#include "cl_game.h"

namespace cl
{

constexpr uint32_t save_signature = 0x57414C43u;   // "CLAW"
constexpr uint16_t save_version = 1;

enum class RecordKind : uint8_t
{
    PROFILE = 1,
    SUSPEND = 2
};

// SRAM layout (byte offsets).
namespace save_layout
{
    constexpr int profile_a = 0x0000;
    constexpr int profile_b = 0x0400;
    constexpr int profile_size = 0x0400;
    constexpr int suspend_a = 0x0800;
    constexpr int suspend_b = 0x1800;
    constexpr int suspend_size = 0x1000;
    constexpr int total_size = 0x2800;
}

constexpr int header_size = 20;

struct SaveSettings
{
    uint8_t music_volume = 8;
    uint8_t sfx_volume = 8;
    bool music_on = true;
    bool sfx_on = true;
    uint8_t animations = 0;
    uint8_t text_speed = 1;
    uint8_t board_theme = 0;
    bool high_contrast = false;
    bool reduced_effects = false;
    bool show_coordinates = true;
    uint8_t preferred_difficulty = 1;

    [[nodiscard]] bool operator==(const SaveSettings&) const = default;
};

constexpr int encounters_count = 18;
constexpr int challenges_max = 16;

constexpr int tutorial_lessons_count = 10;
constexpr int music_tracks_count = 13;

// Only facts are stored; gallery entries and board skins are derived from them (cl_progress.h), so an unlock can
// never disagree with the progress that earned it.
struct SaveProgress
{
    uint32_t encounters_cleared = 0;        // bit per campaign encounter
    uint8_t encounter_best[encounters_count] = {};   // fewest player moves used to clear (0 = never, capped 255)
    uint64_t achievements = 0;              // bit per achievement
    uint32_t challenge_medals = 0;          // 2 bits per challenge: 0 none, 1 bronze, 2 silver, 3 gold
    uint16_t music_unlocked = 0;            // bit per soundtrack entry (heard at least once)
    uint16_t tutorial_lessons = 0;          // bit per completed tutorial lesson
    uint8_t gauntlet_best = 0;              // bosses defeated in the best gauntlet run (0-6)
    bool gauntlet_cleared = false;
    bool intro_seen = false;
    bool ending_seen = false;
    uint8_t story_world = 0;                // furthest unlocked world (0-5)
    uint8_t worlds_seen = 0;                // bit per world whose introduction was shown (bit 0: prologue too)

    [[nodiscard]] bool operator==(const SaveProgress&) const = default;
};

struct SaveStats
{
    uint32_t games_played = 0;
    uint32_t games_won = 0;
    uint32_t games_lost = 0;
    uint32_t games_drawn = 0;
    uint32_t checkmates = 0;
    uint32_t captures = 0;
    uint32_t checks = 0;
    uint32_t promotions = 0;
    uint32_t castles = 0;
    uint32_t en_passants = 0;
    uint32_t queens_captured = 0;
    uint32_t spells[ABILITIES_COUNT] = {};
    uint32_t play_seconds = 0;
    uint16_t win_streak = 0;
    uint16_t best_streak = 0;
    uint8_t best_difficulty_beaten = 0;     // 0 none, 1 apprentice ... 4 grandmaster

    [[nodiscard]] bool operator==(const SaveStats&) const = default;
};

struct Profile
{
    SaveSettings settings;
    SaveProgress progress;
    SaveStats stats;

    [[nodiscard]] bool operator==(const Profile&) const = default;
};

// Everything needed to resume a match exactly.
struct SuspendedMatch
{
    uint8_t mode = 0;               // console MatchMode
    uint8_t human_mask = 1;
    uint8_t difficulty = 1;
    uint8_t personality = 0;
    uint8_t board_theme = 0;
    uint8_t opponent_faction = 0;
    int8_t encounter = -1;
    int8_t challenge = -1;
    int8_t gauntlet_stage = -1;
    uint32_t match_seed = 0;
    uint32_t objective_state = 0;   // scenario bookkeeping (moves made, flags)
    uint8_t recorder[20] = {};      // MatchRecorder counters (statistics / achievements of this match)
    Position base;                  // repetition window base
    uint16_t move_count = 0;
    Move moves[Game::max_window];
};

[[nodiscard]] uint32_t crc32(const uint8_t* data, int size, uint32_t crc = 0);

// ---- records ----
// Serializes into buffer (capacity bytes). Returns the record size, or 0 if it does not fit.
int write_profile_record(const Profile& profile, uint32_t sequence, uint8_t* buffer, int capacity);
int write_suspend_record(const SuspendedMatch& match, uint32_t sequence, uint8_t* buffer, int capacity);

enum class RecordStatus : uint8_t
{
    VALID,
    BLANK,              // no signature (fresh or erased memory)
    CORRUPTED,          // signature present but CRC / length / content invalid
    UNSUPPORTED         // written by a newer format version
};

RecordStatus read_profile_record(const uint8_t* buffer, int size, Profile& profile, uint32_t& sequence);
RecordStatus read_suspend_record(const uint8_t* buffer, int size, SuspendedMatch& match, uint32_t& sequence);

// Chooses between the A and B copies: the valid one with the newer sequence number (wrap-safe). Returns 0 for A,
// 1 for B, -1 if neither is valid.
int choose_record(RecordStatus a, uint32_t sequence_a, RecordStatus b, uint32_t sequence_b);

// Builds / restores a suspended match from a running game.
void capture_game(const Game& game, SuspendedMatch& match);
bool restore_game(const SuspendedMatch& match, Game& game);

// Compact position serialization used by the suspend record (also used by tests).
int write_position(const Position& pos, uint8_t* buffer);
bool read_position(const uint8_t* buffer, Position& pos);
constexpr int position_bytes = 58;

}

#endif
