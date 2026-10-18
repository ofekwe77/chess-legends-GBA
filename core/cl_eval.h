// CHESS LEGENDS: ARCANE WARS - static position evaluation.
#ifndef CL_EVAL_H
#define CL_EVAL_H

#include "cl_position.h"

namespace cl
{

// Evaluation feature weights in percent (100 = baseline). AI personalities and difficulty levels change these
// within bounded ranges; they never change the rules or the material values.
struct EvalProfile
{
    int16_t mobility = 100;
    int16_t king_safety = 100;
    int16_t pawn_structure = 100;
    int16_t passed_pawns = 100;
    int16_t development = 100;
    int16_t king_attack = 100;
    int16_t piece_activity = 100;    // rooks on open files, outposts, seventh rank
    int16_t mana = 100;              // value of stored mana and of Royal Escape readiness
    int16_t magic_threats = 100;     // vulnerability to enemy magical repositioning
    int16_t noise = 0;               // deterministic pseudo-random noise amplitude (centipawns)
    uint32_t noise_seed = 0;
    bool full = true;                // false: material, tables and simple king safety only (Apprentice)
};

// Score in centipawns from the side to move's point of view.
[[nodiscard]] int evaluate(const Position& pos, const EvalProfile& profile);

// x / 24 rounded toward zero, without a division instruction (the ARM7 has none; Thumb code would call a slow
// library routine). Exact for |x| < 2^19 except for rounding in the last unit; antisymmetric, so mirrored
// positions keep exactly opposite scores.
[[nodiscard]] inline int div24(int x)
{
    return x >= 0 ? (x * 2731) >> 16 : -((-x * 2731) >> 16);
}

// Material + piece-square tables + bishop pair + tempo, from the side to move's point of view: the cheap part of
// the evaluation (also the first term of evaluate()).
[[nodiscard]] inline int evaluate_quick(const Position& pos)
{
    constexpr int full_phase = 24;
    const int phase = pos.phase > full_phase ? full_phase : pos.phase;
    int mg = pos.score_mg + (pos.side == WHITE ? 10 : -10);
    int eg = pos.score_eg + (pos.side == WHITE ? 4 : -4);

    if(pos.piece_count[W_BISHOP] >= 2)
    {
        mg += 28;
        eg += 48;
    }

    if(pos.piece_count[B_BISHOP] >= 2)
    {
        mg -= 28;
        eg -= 48;
    }

    int score = div24(mg * phase + eg * (full_phase - phase));
    return pos.side == WHITE ? score : -score;
}

// Same score as evaluate(), but returns the cheap estimate when it is far outside [alpha, beta] (quiescence stand
// pat). Inline so the cheap path runs inside the caller's fast code.
[[nodiscard]] inline int evaluate_lazy(const Position& pos, const EvalProfile& profile, int alpha, int beta)
{
    const int quick = evaluate_quick(pos);
    const int phase = pos.phase > 24 ? 24 : pos.phase;

    // Positional terms rarely exceed this margin; passed pawns and mop-up grow in the endgame.
    const int margin = 260 + (24 - phase) * 10;

    if(quick - margin >= beta || quick + margin <= alpha)
    {
        return quick;
    }

    return evaluate(pos, profile);
}

// Detailed breakdown for tests / tuning (White's point of view, before tapering where noted).
struct EvalBreakdown
{
    int material_psqt;
    int pawns;
    int pieces;
    int king_safety;
    int mana;
    int total_white;
};

void evaluate_breakdown(const Position& pos, const EvalProfile& profile, EvalBreakdown& output);

// Concave value of stored mana (the sixth point is worth little because regeneration beyond the cap is lost).
[[nodiscard]] int mana_store_value(int mana, int max_mana);

}

#endif
