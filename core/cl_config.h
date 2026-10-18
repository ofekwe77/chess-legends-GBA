// CHESS LEGENDS: ARCANE WARS - centralized balance parameters.
//
// Every tunable Arcane Chess value lives here (design document section 36). Changing a value requires a
// documented reason in docs/BALANCE_REPORT.md: previous value, new value, observed problem, test evidence and
// expected strategic effect.
#ifndef CL_CONFIG_H
#define CL_CONFIG_H

#include <cstdint>

namespace cl
{

enum Ability : uint8_t
{
    ABILITY_SHADOW_STEP = 0,    // pawn: one square sideways
    ABILITY_PHASE_SHIFT = 1,    // knight: teleport to an adjacent empty square
    ABILITY_GHOST_WALK = 2,     // bishop: pass diagonally through exactly one friendly piece
    ABILITY_CASTLE_SWAP = 3,    // rook: swap with a friendly bishop / knight up to 3 squares away
    ABILITY_ROYAL_COMMAND = 4,  // queen: move an adjacent friendly P/N/B/R to a square next to her
    ABILITY_ROYAL_ESCAPE = 5,   // king, only in check, once per game: leap exactly two squares
    ABILITIES_COUNT = 6
};

constexpr uint8_t ABILITY_MASK_ALL = (1 << ABILITIES_COUNT) - 1;

namespace config
{
    // Mana (ARCANE ENERGY).
    constexpr int max_mana = 6;
    constexpr int mana_regeneration = 1;

    // Ability costs (initial design parameters, unchanged after testing - see docs/BALANCE_REPORT.md).
    constexpr uint8_t ability_cost[ABILITIES_COUNT] = {
        2,  // Shadow Step
        3,  // Phase Shift
        3,  // Ghost Walk
        4,  // Castle Swap
        5,  // Royal Command
        5   // Royal Escape
    };

    // Royal Escape uses per player per game.
    constexpr int royal_escape_uses = 1;

    // Castle Swap maximum distance (squares along the rank / file).
    constexpr int castle_swap_max_distance = 3;

    // Automatic draw rules.
    constexpr int fifty_move_claim_plies = 100;
    constexpr int seventy_five_move_plies = 150;
    constexpr int threefold_claim_count = 3;
    constexpr int fivefold_automatic_count = 5;

    // Arcane Chess safety limit: magical pawn moves reset the halfmove counter (design rule 27), so the
    // seventy-five-move rule alone does not bound the length of an Arcane game. A match that reaches this many
    // plies is drawn. Normal games never get close (documented in docs/RULES.md).
    constexpr int arcane_max_plies = 600;
}

}

#endif
