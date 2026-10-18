// CHESS LEGENDS: ARCANE WARS - static position evaluation (ARM code in IWRAM on the console).
#include "cl_eval.h"

#include "cl_psqt.h"
#include "cl_tables.h"

namespace cl
{

namespace
{
    struct Score
    {
        int mg = 0;
        int eg = 0;

        void add(int m, int e)
        {
            mg += m;
            eg += e;
        }
    };

    struct PawnInfo
    {
        int8_t count[2][8];       // pawns per file
        int8_t min_rank[2][8];    // lowest rank of a pawn on the file (8 if none)
        int8_t max_rank[2][8];    // highest rank of a pawn on the file (-1 if none)
    };

    // Passed pawn bonus by relative rank (index 1 = second rank ... 6 = seventh rank).
    constexpr int16_t passed_mg[8] = { 0, 5, 8, 15, 28, 45, 70, 0 };
    constexpr int16_t passed_eg[8] = { 0, 10, 15, 28, 50, 85, 135, 0 };

    // Mana store value (index = mana), concave: the last point before the cap is worth the least.
    constexpr int16_t mana_table[7] = { 0, 9, 17, 24, 30, 35, 38 };

    // King attack weights by attacker type (tropism), and the non-linear multiplier by attacker count (/128).
    constexpr int16_t tropism_weight[7] = { 0, 0, 3, 2, 3, 5, 0 };
    constexpr int16_t attacker_scale[8] = { 0, 0, 64, 96, 113, 120, 124, 128 };

    // value * percent / 100 without a division (error below 0.1 %).
    [[nodiscard]] inline int pct(int value, int percent)
    {
        return (value * percent * 41) >> 12;
    }

    [[nodiscard]] inline int taper(int mg, int eg, int phase)
    {
        return div24(mg * phase + eg * (max_phase - phase));
    }

    [[nodiscard]] inline int abs_value(int value)
    {
        return value < 0 ? -value : value;
    }

    [[nodiscard]] inline uint32_t mix_hash(uint64_t hash, uint32_t seed)
    {
        uint32_t x = uint32_t(hash) ^ uint32_t(hash >> 32) ^ seed;
        x ^= x >> 16;
        x *= 0x7FEB352Du;
        x ^= x >> 15;
        x *= 0x846CA68Bu;
        x ^= x >> 16;
        return x;
    }

    [[nodiscard]] inline bool knight_attacks(int from, int target)
    {
        return (tables.mask[target - from + 119] & ATK_KNIGHT) != 0;
    }

    [[nodiscard]] inline bool pawn_protected(const Piece* board, int sq, Color c)
    {
        const int back = c == WHITE ? -16 : 16;
        const Piece own_pawn = make_piece(c, PAWN);
        return (on_board(sq + back - 1) && board[sq + back - 1] == own_pawn) ||
               (on_board(sq + back + 1) && board[sq + back + 1] == own_pawn);
    }

    // Squares a slider reaches (empty squares plus a first enemy piece) along the given directions.
    [[nodiscard]] inline int slide_count(const Piece* board, int sq, const int8_t* steps, int count, Color c)
    {
        int moves = 0;

        for(int d = 0; d < count; ++d)
        {
            const int step = steps[d];
            int t = sq + step;

            while(on_board(t))
            {
                const Piece p = board[t];

                if(p != EMPTY)
                {
                    moves += color_of(p) != c;
                    break;
                }

                ++moves;
                t += step;
            }
        }

        return moves;
    }

    CL_IWRAM_CODE void build_pawn_info(const Position& pos, PawnInfo& info)
    {
        for(int c = 0; c < 2; ++c)
        {
            for(int f = 0; f < 8; ++f)
            {
                info.count[c][f] = 0;
                info.min_rank[c][f] = 8;
                info.max_rank[c][f] = -1;
            }

            const uint8_t* list = pos.plist[c];
            const int count = pos.pcount[c];

            for(int i = 1; i < count; ++i)
            {
                const int sq = list[i];

                if(type_of(pos.board[sq]) != PAWN)
                {
                    continue;
                }

                const int f = file_of(sq);
                const int r = rank_of(sq);
                ++info.count[c][f];

                if(r < info.min_rank[c][f])
                {
                    info.min_rank[c][f] = int8_t(r);
                }

                if(r > info.max_rank[c][f])
                {
                    info.max_rank[c][f] = int8_t(r);
                }
            }
        }
    }

    [[nodiscard]] inline bool is_passed(const PawnInfo& info, Color c, int sq)
    {
        const int f = file_of(sq);
        const int r = rank_of(sq);
        const Color them = opposite(c);
        const int first = f > 0 ? f - 1 : 0;
        const int last = f < 7 ? f + 1 : 7;

        for(int ff = first; ff <= last; ++ff)
        {
            if(c == WHITE ? info.max_rank[them][ff] > r : (info.count[them][ff] && info.min_rank[them][ff] < r))
            {
                return false;
            }
        }

        return true;
    }

    // True when no enemy pawn can ever attack the square (outpost test).
    [[nodiscard]] inline bool safe_from_pawns(const PawnInfo& info, Color c, int sq)
    {
        const int f = file_of(sq);
        const int r = rank_of(sq);
        const Color them = opposite(c);

        for(int ff = f - 1; ff <= f + 1; ff += 2)
        {
            if(ff < 0 || ff > 7 || ! info.count[them][ff])
            {
                continue;
            }

            if(c == WHITE ? info.max_rank[them][ff] > r : info.min_rank[them][ff] < r)
            {
                return false;
            }
        }

        return true;
    }

    CL_IWRAM_CODE void evaluate_pawns(const Position& pos, const PawnInfo& info, const EvalProfile& profile,
                                      Score& out)
    {
        for(int c = 0; c < 2; ++c)
        {
            const Color color = Color(c);
            Score s;

            for(int f = 0; f < 8; ++f)
            {
                const int count = info.count[c][f];

                if(! count)
                {
                    continue;
                }

                if(count > 1)
                {
                    s.add(-12 * (count - 1), -22 * (count - 1));
                }

                const bool left = f > 0 && info.count[c][f - 1];
                const bool right = f < 7 && info.count[c][f + 1];

                if(! left && ! right)
                {
                    s.add(-12 * count, -16 * count);
                }
            }

            Score passed;
            const uint8_t* list = pos.plist[c];
            const int count = pos.pcount[c];
            const int own_king = pos.king_square(color);
            const int enemy_king = pos.king_square(opposite(color));

            for(int i = 1; i < count; ++i)
            {
                const int sq = list[i];

                if(type_of(pos.board[sq]) != PAWN || ! is_passed(info, color, sq))
                {
                    continue;
                }

                const int rel = relative_rank(color, sq);
                int bonus_mg = passed_mg[rel];
                int bonus_eg = passed_eg[rel];

                if(pawn_protected(pos.board, sq, color))
                {
                    bonus_mg += bonus_mg >> 2;
                    bonus_eg += bonus_eg >> 2;
                }

                const int front = sq + (c == WHITE ? 16 : -16);

                if(on_board(front) && pos.board[front] != EMPTY)
                {
                    bonus_eg -= (bonus_eg * 85) >> 8;
                }

                // Endgame king proximity: own king close, enemy king far from the promotion square.
                const int promote_sq = make_square(file_of(sq), c == WHITE ? 7 : 0);
                bonus_eg += (square_distance(enemy_king, promote_sq) - square_distance(own_king, promote_sq)) * rel * 2;
                passed.add(bonus_mg, bonus_eg);
            }

            const int mg = pct(s.mg, profile.pawn_structure) + pct(passed.mg, profile.passed_pawns);
            const int eg = pct(s.eg, profile.pawn_structure) + pct(passed.eg, profile.passed_pawns);

            if(c == WHITE)
            {
                out.add(mg, eg);
            }
            else
            {
                out.add(-mg, -eg);
            }
        }
    }

    // Pieces: mobility, activity, king tropism. Also returns per-side king attack pressure.
    CL_IWRAM_CODE void evaluate_pieces(const Position& pos, const PawnInfo& info, const EvalProfile& profile,
                                       Score& out, int attack_pressure[2])
    {
        const Piece* board = pos.board;

        for(int c = 0; c < 2; ++c)
        {
            const Color color = Color(c);
            const Color them = opposite(color);
            const int enemy_king = pos.king_square(them);
            const uint8_t* list = pos.plist[c];
            const int count = pos.pcount[c];
            int mobility_mg = 0;
            int mobility_eg = 0;
            int activity_mg = 0;
            int activity_eg = 0;
            int tropism = 0;
            int attackers = 0;

            for(int i = 1; i < count; ++i)
            {
                const int sq = list[i];
                const PieceType type = type_of(board[sq]);

                switch(type)
                {

                case KNIGHT:
                    {
                        int moves = 0;

                        for(int8_t step : knight_offsets)
                        {
                            const int t = sq + step;

                            if(on_board(t) && (board[t] == EMPTY || color_of(board[t]) != color))
                            {
                                ++moves;
                            }
                        }

                        mobility_mg += 4 * (moves - 4);
                        mobility_eg += 4 * (moves - 4);

                        // Outpost: relative ranks 4-6, never attackable by enemy pawns (better when pawn-protected).
                        const int rel = relative_rank(color, sq);

                        if(rel >= 3 && rel <= 5 && safe_from_pawns(info, color, sq))
                        {
                            const bool protected_by_pawn = pawn_protected(board, sq, color);
                            activity_mg += protected_by_pawn ? 20 : 8;
                            activity_eg += protected_by_pawn ? 12 : 4;
                        }
                    }
                    break;

                case BISHOP:
                    {
                        const int moves = slide_count(board, sq, bishop_offsets, 4, color);
                        mobility_mg += 5 * (moves - 6);
                        mobility_eg += 5 * (moves - 6);
                    }
                    break;

                case ROOK:
                    {
                        const int moves = slide_count(board, sq, rook_offsets, 4, color);
                        mobility_mg += 2 * (moves - 7);
                        mobility_eg += 4 * (moves - 7);
                        const int f = file_of(sq);

                        if(! info.count[c][f])
                        {
                            activity_mg += info.count[them][f] ? 10 : 22;
                            activity_eg += info.count[them][f] ? 5 : 10;
                        }

                        if(relative_rank(color, sq) == 6 && relative_rank(color, enemy_king) == 7)
                        {
                            activity_mg += 20;
                            activity_eg += 30;
                        }
                    }
                    break;

                case QUEEN:
                    {
                        const int moves = slide_count(board, sq, king_offsets, 8, color);
                        mobility_mg += moves - 13;
                        mobility_eg += 2 * (moves - 13);
                    }
                    break;

                default:
                    continue;   // pawns
                }

                const int distance = square_distance(sq, enemy_king);
                attackers += distance <= 3;
                tropism += tropism_weight[type] * (7 - distance);
            }

            int pressure = (tropism * attacker_scale[attackers > 7 ? 7 : attackers]) >> 7;

            if(pos.piece_count[make_piece(color, QUEEN)] == 0)
            {
                pressure >>= 1;
            }

            attack_pressure[c] = pressure;
            const int mg = pct(mobility_mg, profile.mobility) + pct(activity_mg, profile.piece_activity) +
                           pct(pressure, profile.king_attack);
            const int eg = pct(mobility_eg, profile.mobility) + pct(activity_eg, profile.piece_activity);

            if(c == WHITE)
            {
                out.add(mg, eg);
            }
            else
            {
                out.add(-mg, -eg);
            }
        }
    }

    // Middlegame king shelter and danger; returns per-side danger (used by the Royal Escape valuation).
    CL_IWRAM_CODE void evaluate_king_safety(const Position& pos, const PawnInfo& info, const EvalProfile& profile,
                                            const int attack_pressure[2], Score& out, int danger[2])
    {
        for(int c = 0; c < 2; ++c)
        {
            const Color color = Color(c);
            const Color them = opposite(color);
            const int king = pos.king_square(color);
            const int kf = file_of(king);
            const int rel = relative_rank(color, king);
            int shelter = 0;

            if(rel <= 1)
            {
                const int forward = c == WHITE ? 16 : -16;
                const Piece own_pawn = make_piece(color, PAWN);

                for(int f = kf - 1; f <= kf + 1; ++f)
                {
                    if(f < 0 || f > 7)
                    {
                        continue;
                    }

                    const int sq1 = make_square(f, rank_of(king)) + forward;
                    const int sq2 = sq1 + forward;

                    if(on_board(sq1) && pos.board[sq1] == own_pawn)
                    {
                        shelter += 12;
                    }
                    else if(on_board(sq2) && pos.board[sq2] == own_pawn)
                    {
                        shelter += 6;
                    }
                    else
                    {
                        shelter -= 10;
                    }

                    if(! info.count[c][f])
                    {
                        shelter -= info.count[them][f] ? 8 : 16;
                    }
                }
            }
            else
            {
                // King away from its home ranks in the middlegame.
                shelter -= 18 + rel * 6;
            }

            const int enemy_pressure = attack_pressure[them];
            const int d = enemy_pressure - shelter;
            danger[c] = d > 0 ? d : 0;
            const int value = pct(shelter - (enemy_pressure >> 1), profile.king_safety);
            out.add(c == WHITE ? value : -value, 0);
        }
    }

    CL_IWRAM_CODE void evaluate_development(const Position& pos, const EvalProfile& profile, Score& out)
    {
        for(int c = 0; c < 2; ++c)
        {
            const Color color = Color(c);
            const int back_rank = c == WHITE ? 0 : 7;
            const Piece knight = make_piece(color, KNIGHT);
            const Piece bishop = make_piece(color, BISHOP);
            const Piece* rank = pos.board + back_rank * 16;
            int undeveloped = 0;
            undeveloped += rank[1] == knight || rank[1] == bishop;
            undeveloped += rank[2] == knight || rank[2] == bishop;
            undeveloped += rank[5] == knight || rank[5] == bishop;
            undeveloped += rank[6] == knight || rank[6] == bishop;
            int value = -undeveloped * 10;

            // Early queen adventures while minor pieces are still at home.
            const Piece queen = make_piece(color, QUEEN);

            if(undeveloped >= 2 && pos.piece_count[queen] && rank[3] != queen)
            {
                value -= 14;
            }

            // Castled king bonus / uncastled king without rights.
            const int king = pos.king_square(color);
            const uint8_t rights = c == WHITE ? (CASTLE_WK | CASTLE_WQ) : (CASTLE_BK | CASTLE_BQ);

            if(rank_of(king) == back_rank && (file_of(king) >= 6 || file_of(king) <= 2))
            {
                value += 18;
            }
            else if(! (pos.castling & rights) && file_of(king) >= 3 && file_of(king) <= 5)
            {
                value -= 16;
            }

            value = pct(value, profile.development);
            out.add(c == WHITE ? value : -value, 0);
        }
    }

    CL_IWRAM_CODE void evaluate_arcane(const Position& pos, const EvalProfile& profile, const int danger[2],
                                       Score& out)
    {
        const uint8_t* cost = pos.rules.cost;

        for(int c = 0; c < 2; ++c)
        {
            const Color color = Color(c);
            const Color them = opposite(color);

            // The player not to move regenerates before acting: compare the mana each side will act with.
            int mana = pos.mana[c] + (color != pos.side ? pos.rules.regeneration : 0);

            if(mana > pos.rules.max_mana)
            {
                mana = pos.rules.max_mana;
            }

            int value = mana_table[mana > 6 ? 6 : mana];

            // Abilities that are both affordable and backed by the right pieces.
            const uint8_t mask = pos.rules.ability_mask[c];

            if((mask & (1 << ABILITY_SHADOW_STEP)) && mana >= cost[ABILITY_SHADOW_STEP] &&
               pos.piece_count[make_piece(color, PAWN)])
            {
                value += 2;
            }

            if((mask & (1 << ABILITY_PHASE_SHIFT)) && mana >= cost[ABILITY_PHASE_SHIFT] &&
               pos.piece_count[make_piece(color, KNIGHT)])
            {
                value += 3;
            }

            if((mask & (1 << ABILITY_GHOST_WALK)) && mana >= cost[ABILITY_GHOST_WALK] &&
               pos.piece_count[make_piece(color, BISHOP)])
            {
                value += 3;
            }

            if((mask & (1 << ABILITY_ROYAL_COMMAND)) && mana >= cost[ABILITY_ROYAL_COMMAND] &&
               pos.piece_count[make_piece(color, QUEEN)])
            {
                value += 4;
            }

            // Royal Escape: an unused charge is a defensive resource, worth more when the king is in danger and
            // the mana to use it is (nearly) available.
            if(pos.escapes[c] > 0 && (mask & (1 << ABILITY_ROYAL_ESCAPE)))
            {
                int reserve = 10;
                const int needed = cost[ABILITY_ROYAL_ESCAPE];
                const int d = danger[c] > 120 ? 120 : danger[c];

                if(mana >= needed)
                {
                    reserve += (d * 85) >> 8;
                }
                else if(mana + 1 >= needed)
                {
                    reserve += (d * 43) >> 8;
                }

                value += reserve;
            }

            value = pct(value, profile.mana);

            // Vulnerability to the opponent's Phase Shift forks / checks: enemy knights one teleport away from
            // attacking this side's king or queen.
            if(profile.full && profile.magic_threats && pos.piece_count[make_piece(them, KNIGHT)])
            {
                const int enemy_mana = pos.mana[them] + (them != pos.side ? pos.rules.regeneration : 0);

                if((pos.rules.ability_mask[them] & (1 << ABILITY_PHASE_SHIFT)) &&
                   enemy_mana >= cost[ABILITY_PHASE_SHIFT])
                {
                    const int king = pos.king_square(color);
                    int queen = NO_SQUARE;
                    const Piece own_queen = make_piece(color, QUEEN);

                    for(int i = 1; i < pos.pcount[c]; ++i)
                    {
                        if(pos.board[pos.plist[c][i]] == own_queen)
                        {
                            queen = pos.plist[c][i];
                            break;
                        }
                    }

                    int threats = 0;
                    const Piece enemy_knight = make_piece(them, KNIGHT);

                    for(int i = 1; i < pos.pcount[them]; ++i)
                    {
                        const int knight = pos.plist[them][i];

                        // A teleport moves one square, a knight attacks at distance two: farther knights cannot.
                        if(pos.board[knight] != enemy_knight || square_distance(knight, king) > 3)
                        {
                            continue;
                        }

                        for(int8_t offset : king_offsets)
                        {
                            const int s = knight + offset;

                            if(! on_board(s) || pos.board[s] != EMPTY)
                            {
                                continue;
                            }

                            const bool hits_king = knight_attacks(s, king);
                            const bool hits_queen = queen != NO_SQUARE && knight_attacks(s, queen);
                            threats += hits_king && hits_queen ? 3 : (hits_king || hits_queen ? 1 : 0);
                        }
                    }

                    if(threats > 6)
                    {
                        threats = 6;
                    }

                    value -= pct(threats * 4, profile.magic_threats);
                }
            }

            out.add(c == WHITE ? value : -value, c == WHITE ? value : -value);
        }
    }

    // Mop-up: drive the lone king to the edge and bring the attacking king closer (enables mating with low depth).
    CL_IWRAM_CODE int mop_up(const Position& pos, int material_white, int material_black)
    {
        Color strong;
        const int diff = material_white - material_black;

        if(diff >= 400 && material_black == 0 && pos.pcount[BLACK] == 1)
        {
            strong = WHITE;
        }
        else if(-diff >= 400 && material_white == 0 && pos.pcount[WHITE] == 1)
        {
            strong = BLACK;
        }
        else
        {
            return 0;
        }

        const int weak_king = pos.king_square(opposite(strong));
        const int strong_king = pos.king_square(strong);
        const int wf = file_of(weak_king);
        const int wr = rank_of(weak_king);
        const int center = (wf < 4 ? 3 - wf : wf - 4) + (wr < 4 ? 3 - wr : wr - 4);
        const int distance = abs_value(file_of(strong_king) - wf) + abs_value(rank_of(strong_king) - wr);
        const int bonus = center * 12 + (14 - distance) * 5;
        return strong == WHITE ? bonus : -bonus;
    }

    [[nodiscard]] inline int non_pawn_material(const Position& pos, Color c)
    {
        return pos.piece_count[make_piece(c, KNIGHT)] * 320 + pos.piece_count[make_piece(c, BISHOP)] * 330 +
               pos.piece_count[make_piece(c, ROOK)] * 500 + pos.piece_count[make_piece(c, QUEEN)] * 900;
    }

    CL_IWRAM_CODE int evaluate_white(const Position& pos, const EvalProfile& profile, EvalBreakdown* breakdown)
    {
        const int phase = pos.phase > max_phase ? max_phase : pos.phase;
        PawnInfo info;
        build_pawn_info(pos, info);
        Score pawns;
        Score pieces;
        Score king;
        Score arcane;
        int pressure[2] = { 0, 0 };
        int danger[2] = { 0, 0 };

        evaluate_pawns(pos, info, profile, pawns);

        if(profile.full)
        {
            evaluate_pieces(pos, info, profile, pieces, pressure);

            if(phase >= 12)
            {
                evaluate_development(pos, profile, pieces);
            }
        }

        evaluate_king_safety(pos, info, profile, pressure, king, danger);

        if(pos.rules.arcane)
        {
            evaluate_arcane(pos, profile, danger, arcane);
        }

        const int quick = evaluate_quick(pos);
        int score = pos.side == WHITE ? quick : -quick;
        score += taper(pawns.mg + pieces.mg + king.mg + arcane.mg, pawns.eg + pieces.eg + king.eg + arcane.eg, phase);

        const int material_white = non_pawn_material(pos, WHITE);
        const int material_black = non_pawn_material(pos, BLACK);
        score += mop_up(pos, material_white, material_black);

        // Drawish material: the stronger side has no pawns and less than a rook of extra material.
        if(score > 0 && ! pos.piece_count[W_PAWN] && material_white - material_black < 400)
        {
            score /= 4;
        }
        else if(score < 0 && ! pos.piece_count[B_PAWN] && material_black - material_white < 400)
        {
            score /= 4;
        }

        if(profile.noise)
        {
            const uint32_t r = mix_hash(pos.hash, profile.noise_seed) >> 16;
            score += int((r * uint32_t(2 * profile.noise + 1)) >> 16) - profile.noise;
        }

        if(breakdown)
        {
            breakdown->material_psqt = taper(pos.score_mg, pos.score_eg, phase);
            breakdown->pawns = taper(pawns.mg, pawns.eg, phase);
            breakdown->pieces = taper(pieces.mg, pieces.eg, phase);
            breakdown->king_safety = taper(king.mg, king.eg, phase);
            breakdown->mana = taper(arcane.mg, arcane.eg, phase);
            breakdown->total_white = score;
        }

        return score;
    }
}

int mana_store_value(int mana, int max_mana)
{
    (void) max_mana;

    if(mana <= 0)
    {
        return 0;
    }

    return mana_table[mana > 6 ? 6 : mana];
}

CL_IWRAM_CODE int evaluate(const Position& pos, const EvalProfile& profile)
{
    const int score = evaluate_white(pos, profile, nullptr);
    return pos.side == WHITE ? score : -score;
}

void evaluate_breakdown(const Position& pos, const EvalProfile& profile, EvalBreakdown& output)
{
    evaluate_white(pos, profile, &output);
}

}
