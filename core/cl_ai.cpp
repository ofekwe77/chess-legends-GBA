// CHESS LEGENDS: ARCANE WARS - computer opponents.
#include "cl_ai.h"

#include "cl_notation.h"

namespace cl
{

namespace
{
    // Style tags for opening candidates: a = aggressive, d = defensive, p = positional, t = tactical,
    // b = balanced (any personality may pick it).
    struct BookCandidate
    {
        const char* move;
        const char* styles;
    };

    struct BookLine
    {
        const char* line;   // moves from the start position, space separated
        BookCandidate candidates[4];
    };

    constexpr BookLine book[] = {
        { "", { { "e2e4", "atb" }, { "d2d4", "dpb" }, { "c2c4", "p" }, { "g1f3", "dp" } } },
        { "e2e4", { { "e7e5", "bd" }, { "c7c5", "at" }, { "e7e6", "d" }, { "c7c6", "dp" } } },
        { "d2d4", { { "d7d5", "bd" }, { "g8f6", "pt" }, { "e7e6", "d" }, { nullptr, nullptr } } },
        { "c2c4", { { "e7e5", "a" }, { "g8f6", "b" }, { "c7c5", "p" }, { nullptr, nullptr } } },
        { "g1f3", { { "d7d5", "b" }, { "g8f6", "b" }, { "c7c5", "a" }, { nullptr, nullptr } } },
        { "e2e4 e7e5", { { "g1f3", "b" }, { "f1c4", "a" }, { "b1c3", "p" }, { nullptr, nullptr } } },
        { "e2e4 c7c5", { { "g1f3", "b" }, { "b1c3", "p" }, { "c2c3", "d" }, { nullptr, nullptr } } },
        { "e2e4 e7e6", { { "d2d4", "b" }, { nullptr, nullptr }, { nullptr, nullptr }, { nullptr, nullptr } } },
        { "e2e4 c7c6", { { "d2d4", "b" }, { "b1c3", "p" }, { nullptr, nullptr }, { nullptr, nullptr } } },
        { "d2d4 d7d5", { { "c2c4", "bp" }, { "g1f3", "d" }, { "c1f4", "d" }, { nullptr, nullptr } } },
        { "d2d4 g8f6", { { "c2c4", "b" }, { "g1f3", "d" }, { nullptr, nullptr }, { nullptr, nullptr } } },
        { "c2c4 e7e5", { { "b1c3", "b" }, { "g2g3", "p" }, { nullptr, nullptr }, { nullptr, nullptr } } },
        { "g1f3 d7d5", { { "d2d4", "b" }, { "g2g3", "p" }, { nullptr, nullptr }, { nullptr, nullptr } } },
        { "e2e4 e7e5 g1f3", { { "b8c6", "b" }, { "g8f6", "d" }, { nullptr, nullptr }, { nullptr, nullptr } } },
        { "e2e4 e7e5 f1c4", { { "g8f6", "b" }, { "f8c5", "a" }, { nullptr, nullptr }, { nullptr, nullptr } } },
        { "d2d4 d7d5 c2c4", { { "e7e6", "d" }, { "c7c6", "b" }, { "d5c4", "a" }, { nullptr, nullptr } } },
        { "e2e4 e7e5 g1f3 b8c6", { { "f1b5", "p" }, { "f1c4", "a" }, { "d2d4", "at" }, { nullptr, nullptr } } },
        { "e2e4 c7c5 g1f3", { { "d7d6", "b" }, { "b8c6", "b" }, { "e7e6", "d" }, { nullptr, nullptr } } },
    };

    [[nodiscard]] char personality_style(Personality personality)
    {
        switch(personality)
        {
        case Personality::AGGRESSIVE: return 'a';
        case Personality::DEFENSIVE: return 'd';
        case Personality::POSITIONAL: return 'p';
        case Personality::TACTICAL: return 't';
        case Personality::TRICKSTER: return 't';
        default: return 'b';
        }
    }

    [[nodiscard]] bool contains(const char* text, char c)
    {
        for(; *text; ++text)
        {
            if(*text == c)
            {
                return true;
            }
        }

        return false;
    }

    [[nodiscard]] bool equal(const char* a, const char* b)
    {
        while(*a && *b)
        {
            if(*a++ != *b++)
            {
                return false;
            }
        }

        return *a == *b;
    }

    [[nodiscard]] uint32_t next_random(uint32_t& state)
    {
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    // Avalanche mixer: nearby seeds (consecutive matches) must still produce unrelated choices.
    [[nodiscard]] uint32_t mix_seed(uint32_t x)
    {
        x ^= x >> 16;
        x *= 0x7FEB352Du;
        x ^= x >> 15;
        x *= 0x846CA68Bu;
        x ^= x >> 16;
        return x ? x : 0x9E3779B9u;
    }

    void scale_profile(EvalProfile& eval, int mobility, int king_safety, int pawns, int passed, int development,
                       int king_attack, int activity, int mana, int threats)
    {
        eval.mobility = int16_t(mobility);
        eval.king_safety = int16_t(king_safety);
        eval.pawn_structure = int16_t(pawns);
        eval.passed_pawns = int16_t(passed);
        eval.development = int16_t(development);
        eval.king_attack = int16_t(king_attack);
        eval.piece_activity = int16_t(activity);
        eval.mana = int16_t(mana);
        eval.magic_threats = int16_t(threats);
    }
}

const char* difficulty_name(Difficulty difficulty)
{
    switch(difficulty)
    {
    case Difficulty::APPRENTICE: return "APPRENTICE";
    case Difficulty::KNIGHT: return "KNIGHT";
    case Difficulty::SORCERER: return "SORCERER";
    case Difficulty::GRANDMASTER: return "GRANDMASTER";
    default: return "?";
    }
}

const char* personality_name(Personality personality)
{
    switch(personality)
    {
    case Personality::BALANCED: return "Balanced";
    case Personality::AGGRESSIVE: return "Aggressive";
    case Personality::DEFENSIVE: return "Defensive";
    case Personality::TACTICAL: return "Tactical";
    case Personality::POSITIONAL: return "Positional";
    case Personality::TRICKSTER: return "Trickster";
    case Personality::MASTER: return "Master";
    default: return "?";
    }
}

AiProfile make_ai_profile(const AiSettings& settings)
{
    AiProfile profile;
    SearchLimits& limits = profile.limits;
    EvalProfile& eval = profile.eval;

    // ---- difficulty: search budget, evaluation depth, controlled imperfection ----
    // Budgets are node counts calibrated on the console (about 1,000-1,500 nodes per second while the interface
    // keeps animating): roughly 0.3 s, 1.3 s, 3 s and 6 s per move. Node budgets keep the AI deterministic, so the
    // host simulator reproduces console play exactly.
    switch(settings.difficulty)
    {
    case Difficulty::APPRENTICE:
        limits.max_nodes = 350;
        limits.max_depth = 2;
        limits.max_quiescence_depth = 4;
        limits.quiescence_magic_checks = 2;
        eval.full = false;
        eval.noise = 40;
        profile.imperfection_percent = 22;
        profile.imperfection_margin = 90;
        break;

    case Difficulty::KNIGHT:
        limits.max_nodes = 1400;
        limits.max_depth = 4;
        limits.max_quiescence_depth = 6;
        limits.quiescence_magic_checks = 4;
        eval.noise = 10;
        profile.imperfection_percent = 6;
        profile.imperfection_margin = 35;
        break;

    case Difficulty::SORCERER:
        limits.max_nodes = 3200;
        limits.max_depth = 8;
        limits.max_quiescence_depth = 8;
        limits.quiescence_magic_checks = 6;
        break;

    case Difficulty::GRANDMASTER:
    default:
        limits.max_nodes = 6000;
        limits.max_depth = 16;
        limits.max_quiescence_depth = 8;
        limits.quiescence_magic_checks = 8;
        break;
    }

    if(settings.node_scale_percent != 100 && settings.node_scale_percent > 0)
    {
        // 32-bit arithmetic on purpose: a 64-bit division would link a 3 KB routine into the console's IWRAM.
        int percent = settings.node_scale_percent > 400 ? 400 : settings.node_scale_percent;
        limits.max_nodes = limits.max_nodes / 100u * uint32_t(percent);
    }

    // ---- personality: bounded evaluation preferences ----
    switch(settings.personality)
    {
    case Personality::AGGRESSIVE:
        scale_profile(eval, 120, 85, 95, 100, 110, 150, 110, 90, 100);
        limits.draw_score = -25;
        break;

    case Personality::DEFENSIVE:
        scale_profile(eval, 90, 150, 130, 105, 105, 80, 100, 115, 140);
        limits.draw_score = 0;
        break;

    case Personality::TACTICAL:
        scale_profile(eval, 110, 100, 95, 100, 100, 120, 110, 100, 120);
        limits.quiescence_magic_checks += 2;
        limits.draw_score = -10;
        break;

    case Personality::POSITIONAL:
        scale_profile(eval, 130, 105, 120, 115, 110, 90, 130, 100, 100);
        break;

    case Personality::TRICKSTER:
        // Cheaper mana valuation: more willing to spend energy on (sound) magical repositioning.
        scale_profile(eval, 110, 95, 100, 100, 100, 110, 110, 80, 120);
        limits.quiescence_magic_checks += 2;
        limits.draw_score = -15;
        break;

    case Personality::MASTER:
        scale_profile(eval, 110, 110, 105, 105, 105, 105, 105, 105, 110);
        limits.draw_score = -10;
        break;

    case Personality::BALANCED:
    default:
        break;
    }

    eval.noise_seed = settings.seed * 2654435761u;
    return profile;
}

Move Ai::_book_move(Game& game, const AiSettings& settings) const
{
    const int played = game.plies_played();

    if(played >= 5 || played != game.log_count())
    {
        return Move();
    }

    // Build the move line from the game log.
    char line[64];
    int length = 0;
    line[0] = 0;

    for(int i = 0; i < game.log_count(); ++i)
    {
        char text[16];
        move_to_coordinate(game.log_move(i), text, int(sizeof(text)));

        if(i)
        {
            line[length++] = ' ';
        }

        for(const char* c = text; *c && length < 62; ++c)
        {
            line[length++] = *c;
        }

        line[length] = 0;
    }

    const char style = personality_style(settings.personality);

    for(const BookLine& entry : book)
    {
        if(! equal(entry.line, line))
        {
            continue;
        }

        // Candidates matching the personality style first; balanced ('b') candidates are acceptable to everyone.
        const BookCandidate* options[4];
        int count = 0;

        for(const BookCandidate& candidate : entry.candidates)
        {
            if(candidate.move && contains(candidate.styles, style))
            {
                options[count++] = &candidate;
            }
        }

        if(! count)
        {
            for(const BookCandidate& candidate : entry.candidates)
            {
                if(candidate.move && contains(candidate.styles, 'b'))
                {
                    options[count++] = &candidate;
                }
            }
        }

        if(! count)
        {
            return Move();
        }

        uint32_t state = mix_seed(settings.seed ^ (uint32_t(played) * 0x9E3779B9u) ^ 0x1234567u);
        const BookCandidate* chosen = options[(next_random(state) >> 8) % uint32_t(count)];
        Position pos = game.position();
        return parse_coordinate(pos, chosen->move);
    }

    return Move();
}

AiDecision Ai::choose(Game& game, const AiSettings& settings, bool standard_start, PollFunction poll,
                      void* poll_user)
{
    AiDecision decision;

    if(game.over())
    {
        return decision;
    }

    // Opening choices (only from the standard start, and only with normal moves).
    if(standard_start)
    {
        Move book_move = _book_move(game, settings);

        if(! book_move.is_null())
        {
            decision.move = book_move;
            decision.from_book = true;
            return decision;
        }
    }

    AiProfile profile = make_ai_profile(settings);
    profile.limits.poll_mask = settings.poll_mask;

    // Positions before the root, for repetition awareness (the last window entry is the root itself).
    const uint64_t* hashes = game.window_hashes();
    const int history_count = game.window_count();
    SearchResult result = _searcher.search(game.position(), hashes, history_count, profile.limits, profile.eval,
                                           poll, poll_user);

    decision.move = result.best;
    decision.score = result.score;
    decision.depth = result.depth;
    decision.seldepth = result.seldepth;
    decision.nodes = result.nodes;
    decision.aborted = result.aborted;

    // Controlled imperfection (Apprentice / Knight): occasionally choose another move whose shallow score is close
    // to the best shallow score. Mates found by the search are never thrown away.
    if(! result.aborted && profile.imperfection_percent > 0 && result.score < SCORE_MATE_BOUND &&
       result.score > -SCORE_MATE_BOUND)
    {
        uint32_t state = mix_seed(settings.seed ^ uint32_t(game.position().hash) ^
                                  (uint32_t(game.plies_played()) * 0x85EBCA6Bu));

        if(int((next_random(state) >> 8) % 100u) < profile.imperfection_percent)
        {
            static Move moves[MAX_MOVES];
            static int scores[MAX_MOVES];
            SearchLimits quick = profile.limits;
            quick.max_quiescence_depth = 4;
            int count = _searcher.score_root_moves(game.position(), quick, profile.eval, moves, scores, MAX_MOVES);
            int best = -SCORE_INFINITE;

            for(int i = 0; i < count; ++i)
            {
                if(scores[i] > best)
                {
                    best = scores[i];
                }
            }

            static int candidates[MAX_MOVES];
            int candidate_count = 0;

            for(int i = 0; i < count; ++i)
            {
                if(scores[i] >= best - profile.imperfection_margin && scores[i] > -SCORE_MATE_BOUND)
                {
                    candidates[candidate_count++] = i;
                }
            }

            if(candidate_count > 1)
            {
                Move pick = moves[candidates[(next_random(state) >> 8) % uint32_t(candidate_count)]];

                if(pick != decision.move)
                {
                    decision.move = pick;
                    decision.imperfect = true;
                }
            }
        }
    }

    return decision;
}

bool Ai::should_claim_draw(const Game& game, const AiSettings& settings, int last_score)
{
    if(! game.can_claim_threefold() && ! game.can_claim_fifty())
    {
        return false;
    }

    return last_score <= make_ai_profile(settings).claim_draw_below;
}

}
