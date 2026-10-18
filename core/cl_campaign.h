// CHESS LEGENDS: ARCANE WARS - campaign, challenges and scenario objectives.
//
// Encounters and challenges are data. Every objective is enforced by game-state logic (ObjectiveTracker), never
// by presentation: an encounter is won only when the tracker reports SUCCESS after a real action.
#ifndef CL_CAMPAIGN_H
#define CL_CAMPAIGN_H

#include "cl_ai.h"
#include "cl_game.h"

namespace cl
{

enum class Objective : uint8_t
{
    WIN,                // win the match (checkmate, or the opponent resigns)
    CHECKMATE_WITHIN,   // checkmate within `limit` of the player's moves
    SURVIVE,            // do not lose for `limit` of the player's moves
    PROMOTE,            // promote a pawn within `limit` moves
    WIN_MATERIAL,       // lead by at least `value` pawns in material (after the reply) within `limit` moves
    CAPTURE_PAWNS       // capture every enemy pawn within `limit` moves
};

struct ObjectiveSpec
{
    Objective type = Objective::WIN;
    uint8_t limit = 0;      // player moves (0 = unlimited)
    uint8_t value = 0;
};

enum class ObjectiveStatus : uint8_t
{
    ONGOING,
    SUCCESS,
    FAILED
};

class ObjectiveTracker
{
public:
    void start(const ObjectiveSpec& spec, Color player, const Position& start);

    // Call after every applied action (player or opponent).
    void on_action(const ActionInfo& info, const Game& game);

    [[nodiscard]] ObjectiveStatus status(const Game& game) const;

    [[nodiscard]] int player_moves() const
    {
        return _player_moves;
    }

    [[nodiscard]] int moves_left() const
    {
        return _spec.limit ? (_spec.limit > _player_moves ? _spec.limit - _player_moves : 0) : -1;
    }

    [[nodiscard]] const ObjectiveSpec& spec() const
    {
        return _spec;
    }

    // Compact state for suspended matches.
    [[nodiscard]] uint32_t pack() const;
    void unpack(uint32_t state);

private:
    ObjectiveSpec _spec;
    Color _player = WHITE;
    int _player_moves = 0;
    int _start_balance = 0;
    bool _promoted = false;
};

[[nodiscard]] int material_balance(const Position& pos, Color side);   // centipawns, side's point of view

// ------------------------------------------------------------------------------------------------ campaign data

enum class Character : uint8_t
{
    ARDEN,      // the player: young royal tactician
    ORRIN,      // mentor
    ROWAN,      // World 1 boss
    VEYRA,      // World 2 boss
    YSRA,       // World 3 boss
    NOCTIS,     // World 4 boss
    RAEL,       // World 5 boss
    AURELIA,    // World 6 boss
    BRIN,       // World 1 lieutenant
    HARRIK,     // World 2 lieutenant
    ELSK,       // World 3 lieutenant
    QUILL,      // World 4 lieutenant
    TAMSIN,     // World 5 lieutenant
    SERAPH,     // World 6 lieutenant
    NARRATOR,
    COUNT
};

enum class Expression : uint8_t
{
    NEUTRAL,
    INTENSE,    // angry / determined / smug
    DEFEATED    // humbled / surprised
};

struct DialogueLine
{
    Character speaker;
    Expression expression;
    const char* text;
};

struct Dialogue
{
    const DialogueLine* lines;
    int count;
};

enum class EncounterType : uint8_t
{
    LESSON,
    TRIAL,
    BOSS
};

struct Encounter
{
    uint8_t world;
    EncounterType type;
    const char* title;
    Character opponent;
    const char* fen;                // nullptr: standard start position
    Color player;
    ObjectiveSpec objective;
    Difficulty difficulty;
    Personality personality;
    uint8_t node_scale;             // percent of the difficulty's node budget
    uint8_t max_mana;               // 0: standard (6)
    uint8_t player_abilities;       // ability mask (0x3F all)
    uint8_t enemy_abilities;
    const char* briefing;           // objective and every special rule, shown before the match
    Dialogue intro;
    Dialogue victory;
    Dialogue defeat;
};

struct World
{
    const char* name;
    const char* subtitle;
    Character boss;
    Character lieutenant;
    uint8_t faction;                // dark-army palette (1-6)
    uint8_t board_theme;
    Dialogue introduction;
};

constexpr int worlds_count = 6;
constexpr int campaign_encounters = 18;

[[nodiscard]] const World& world(int index);
[[nodiscard]] const Encounter& encounter(int index);
[[nodiscard]] const char* character_name(Character character);
[[nodiscard]] const char* character_title(Character character);

[[nodiscard]] const Dialogue& prologue();
[[nodiscard]] const Dialogue& epilogue();

// Rules for an encounter (Arcane baseline plus the announced scenario modifiers).
[[nodiscard]] RuleSet encounter_rules(const Encounter& encounter);

// Sets up the game for an encounter. Returns false if the data is invalid.
bool setup_encounter(const Encounter& encounter, Game& game);

// ------------------------------------------------------------------------------------------------ boss gauntlet

// The six rulers in a row, each from an opening position that suits their style, with rising difficulty.
struct GauntletStage
{
    Character boss;
    const char* opening;            // opening name shown before the battle
    const char* fen;                // nullptr: standard start position (White to move in every stage)
    Difficulty difficulty;
    Personality personality;
    uint8_t node_scale;
    const char* taunt;
};

constexpr int gauntlet_stages = 6;

[[nodiscard]] const GauntletStage& gauntlet_stage(int index);
bool setup_gauntlet_stage(const GauntletStage& stage, Game& game);

// ------------------------------------------------------------------------------------------------ challenges

struct Challenge
{
    const char* title;
    const char* theme;              // what it teaches
    const char* fen;
    bool arcane;
    ObjectiveSpec objective;        // usually CHECKMATE_WITHIN
    uint8_t par;                    // fewest player moves that solve it against best defense (gold medal)
    uint8_t required_abilities;     // mask: without any one of these no solution within par exists (validated)
    Difficulty defender;            // AI level for the defending side
    const char* hint;
    const char* solution;           // a verified key move (coordinate notation)
};

constexpr int challenges_count = 14;

[[nodiscard]] const Challenge& challenge(int index);
bool setup_challenge(const Challenge& challenge, Game& game);

// Medal for a solved challenge: gold = within the minimum moves, silver = within limit, bronze = solved late.
[[nodiscard]] int challenge_medal(const Challenge& challenge, int player_moves);

}

#endif
