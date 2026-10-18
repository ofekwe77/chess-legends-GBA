// CHESS LEGENDS: ARCANE WARS - a complete match: position, repetition history, terminal conditions and draw
// claims. Game::apply() is the single authoritative action transition used by every mode.
#ifndef CL_GAME_H
#define CL_GAME_H

#include "cl_movegen.h"

namespace cl
{

enum class GameResult : uint8_t
{
    ONGOING,
    WHITE_WINS,
    BLACK_WINS,
    DRAW
};

enum class ResultReason : uint8_t
{
    NONE,
    CHECKMATE,
    RESIGNATION,
    STALEMATE,
    FIVEFOLD_REPETITION,       // automatic
    SEVENTY_FIVE_MOVES,        // automatic (150 plies without capture or pawn movement)
    DEAD_POSITION,             // automatic (no sequence of actions can produce checkmate)
    THREEFOLD_CLAIM,           // claimed by the player to move
    FIFTY_MOVE_CLAIM,          // claimed by the player to move
    AGREEMENT,                 // local multiplayer draw by agreement
    ARCANE_MOVE_LIMIT,         // Arcane safety limit (config::arcane_max_plies)
    OBJECTIVE_COMPLETE,        // scenario objective met (campaign / challenge layer)
    OBJECTIVE_FAILED           // scenario objective failed (campaign / challenge layer)
};

// Exact, normalized identity of an Arcane Chess position for repetition detection (design section 28).
struct CompactPosition
{
    uint8_t cells[32];      // two squares per byte (piece codes)
    uint8_t side_castling;  // bit 7 side to move, bits 0-3 castling rights
    uint8_t ep;             // en passant square only when an en passant capture is legal, else NO_SQUARE
    uint8_t mana;           // White low nibble, Black high nibble
    uint8_t escapes;        // White low nibble, Black high nibble
    uint8_t home[2];

    [[nodiscard]] bool operator==(const CompactPosition& other) const = default;
};

// Details of an accepted action, consumed by the presentation layer (animations, sounds, achievements).
struct ActionInfo
{
    Move move;
    Color mover = WHITE;
    Piece piece = EMPTY;        // moving piece (Royal Command: the commanded ally; Castle Swap: the rook)
    Piece partner = EMPTY;      // Castle Swap partner, Royal Command queen
    Piece captured = EMPTY;
    int captured_sq = NO_SQUARE;
    int mana_spent = 0;
    bool check = false;         // the opponent is in check after the action
};

class Game
{
public:
    // Repetition window capacity: Classic windows end at 150 plies (seventy-five-move rule), Arcane windows end at
    // the Arcane move limit.
    static constexpr int max_window = config::arcane_max_plies + 8;

    // Standard start position. In Arcane Chess White receives the first turn-start mana point immediately.
    void start(const RuleSet& rules);

    // Scenario / puzzle start. The FEN mana values are the current values; when apply_turn_start_regeneration is
    // true the side to move then receives its turn-start regeneration once (used for "before the first turn"
    // scenario definitions).
    FenError setup(const char* fen, const RuleSet& rules, bool apply_turn_start_regeneration);

    [[nodiscard]] const Position& position() const
    {
        return _pos;
    }

    [[nodiscard]] const RuleSet& rules() const
    {
        return _pos.rules;
    }

    // Legal actions for the side to move (normal moves and affordable abilities).
    void legal_actions(MoveList& list);

    // Validates and applies an action. Returns false (and changes nothing, consumes no mana) when the action is
    // illegal or the game is already over.
    bool apply(Move move, ActionInfo* info = nullptr);

    [[nodiscard]] GameResult result() const
    {
        return _result;
    }

    [[nodiscard]] ResultReason reason() const
    {
        return _reason;
    }

    [[nodiscard]] bool over() const
    {
        return _result != GameResult::ONGOING;
    }

    // Draw claims available to the player to move (FIDE-style: threefold repetition / fifty-move rule).
    [[nodiscard]] bool can_claim_threefold() const;
    [[nodiscard]] bool can_claim_fifty() const;
    bool claim_draw();

    void resign(Color loser);
    void agree_draw();

    // Ends the game from the scenario layer.
    void finish_objective(bool success, Color player);

    [[nodiscard]] int repetition_count() const;

    [[nodiscard]] int plies_played() const
    {
        return _pos.ply;
    }

    // Repetition window: base position plus the actions played since then (used by the save system, which
    // stores exactly this to restore full adjudication state).
    [[nodiscard]] const Position& window_base() const
    {
        return _base;
    }

    [[nodiscard]] int window_count() const
    {
        return _window_count;
    }

    [[nodiscard]] Move window_move(int index) const
    {
        return _window_moves[index];
    }

    // Zobrist hashes of the window positions (index 0 = base), for the AI's repetition detection.
    [[nodiscard]] const uint64_t* window_hashes() const
    {
        return _window_hashes;
    }

    // Restores a game from a window base and its actions (save data). Returns false if any action is illegal.
    bool restore(const Position& base, const Move* moves, int count, GameResult result, ResultReason reason);

    // Last actions (for the move log display), newest last.
    static constexpr int log_capacity = 16;

    [[nodiscard]] int log_count() const
    {
        return _log_count;
    }

    [[nodiscard]] Move log_move(int index) const;

    [[nodiscard]] bool dead_position() const;

    static void compact(Position& pos, CompactPosition& output);

private:
    Position _pos;
    Position _base;
    Move _window_moves[max_window];
    CompactPosition _window_positions[max_window + 1];
    uint64_t _window_hashes[max_window + 1];
    int _window_count = 0;
    Move _log[log_capacity];
    int _log_count = 0;
    int _log_start = 0;
    GameResult _result = GameResult::ONGOING;
    ResultReason _reason = ResultReason::NONE;

    void _reset_window();
    void _push_log(Move move);
    void _update_result();
};

[[nodiscard]] const char* result_reason_name(ResultReason reason);

}

#endif
