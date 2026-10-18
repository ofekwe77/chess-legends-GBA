// CHESS LEGENDS: ARCANE WARS - position state, make / unmake and attack detection.
#ifndef CL_POSITION_H
#define CL_POSITION_H

#include "cl_move.h"

namespace cl
{

// Rules that apply to one match. The baseline Arcane Chess rules come from cl_config.h; scenario encounters
// may change them through data (always announced to the player before the encounter, and the AI uses the same
// values because it reads them from the position).
struct RuleSet
{
    bool arcane = false;
    uint8_t max_mana = config::max_mana;
    uint8_t regeneration = config::mana_regeneration;
    uint8_t ability_mask[2] = { ABILITY_MASK_ALL, ABILITY_MASK_ALL };
    uint8_t cost[ABILITIES_COUNT] = { config::ability_cost[0], config::ability_cost[1], config::ability_cost[2],
                                      config::ability_cost[3], config::ability_cost[4], config::ability_cost[5] };

    [[nodiscard]] static RuleSet classic()
    {
        return RuleSet();
    }

    [[nodiscard]] static RuleSet arcane_rules()
    {
        RuleSet result;
        result.arcane = true;
        return result;
    }

    [[nodiscard]] bool operator==(const RuleSet& other) const = default;
};

// Everything make() changes that unmake() cannot recompute cheaply.
struct Undo
{
    uint64_t hash;
    int16_t score_mg;
    int16_t score_eg;
    uint16_t halfmove;
    uint8_t phase;
    Piece captured;
    uint8_t castling;
    uint8_t ep;
    uint8_t mana[2];
    uint8_t escapes[2];
    uint8_t home[2];
};

enum class FenError : uint8_t
{
    NONE,
    SYNTAX,
    KING_COUNT,
    TOO_MANY_PIECES,
    PAWN_ON_BACK_RANK,
    OPPONENT_IN_CHECK,
    BAD_EN_PASSANT,
    BAD_ARCANE_FIELDS
};

struct Position
{
    // ---- board ----
    Piece board[128];
    uint8_t plist[2][16];     // squares of each side's pieces, the king is always at index 0
    uint8_t pcount[2];
    int8_t pindex[128];       // index of the piece in its side's plist (valid for occupied squares)
    uint8_t piece_count[16];  // number of pieces per piece code

    // ---- state ----
    Color side;
    uint8_t castling;
    uint8_t ep;               // en passant target square, or NO_SQUARE (only set when a pawn can capture)
    uint16_t halfmove;        // plies since the last capture or pawn movement (normal or magical)
    uint16_t ply;             // plies played since the game start position
    uint16_t fullmove;        // FEN fullmove number (incremented after each Black action)
    uint8_t mana[2];          // current ARCANE ENERGY (turn-start regeneration already applied for side)
    uint8_t escapes[2];       // Royal Escape uses left
    uint8_t home[2];          // bit f: the pawn on file f of that side's second rank has never moved
    RuleSet rules;

    // ---- incremental evaluation terms (White's point of view) ----
    uint64_t hash;
    int16_t score_mg;
    int16_t score_eg;
    uint8_t phase;

    // ---- setup ----
    void clear(const RuleSet& rule_set);
    void set_start(const RuleSet& rule_set);
    FenError set_fen(const char* fen, const RuleSet& rule_set);
    int write_fen(char* output, int capacity) const;

    // ---- queries ----
    [[nodiscard]] int king_square(Color c) const
    {
        return plist[c][0];
    }

    [[nodiscard]] bool attacked(int sq, Color by) const;
    [[nodiscard]] int attackers_count(int sq, Color by, int limit = 16) const;

    [[nodiscard]] bool in_check(Color c) const
    {
        return attacked(king_square(c), opposite(c));
    }

    [[nodiscard]] bool in_check() const
    {
        return in_check(side);
    }

    [[nodiscard]] bool can_afford(Color c, Ability ability) const
    {
        return rules.arcane && (rules.ability_mask[c] & (1 << ability)) && mana[c] >= rules.cost[ability];
    }

    [[nodiscard]] uint64_t compute_hash() const;

    // Full consistency check (host tests): lists, counters, hash and incremental scores.
    [[nodiscard]] bool validate() const;

    // ---- make / unmake (the single authoritative state transition) ----
    void make(Move move, Undo& undo);
    void unmake(Move move, const Undo& undo);

    // Search-only "pass": hands the turn to the opponent (who receives the normal turn-start regeneration). Never
    // a legal action in a game; used by null-move pruning.
    void make_null(Undo& undo);
    void unmake_null(const Undo& undo);

private:
    void _put(Piece piece, int sq);
    void _remove(int sq);
    void _move(int from, int to);
    void _raw_put(Piece piece, int sq);
    void _raw_remove(int sq);
    void _raw_move(int from, int to);
    void _touch_home(int sq);
    void _set_mana(Color c, int value);
};

[[nodiscard]] bool piece_is_slider(PieceType type);

// Castling rights kept after a piece leaves or arrives on a square.
[[nodiscard]] uint8_t castle_mask(int sq);

constexpr const char* start_fen = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

}

#endif
