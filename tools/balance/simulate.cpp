// CHESS LEGENDS: ARCANE WARS - host-side AI versus AI simulator (design document sections 45, 107, 108).
//
// Usage: simulate [--games N] [--classic] [--white LEVEL] [--black LEVEL] [--personality-white P]
//                 [--personality-black P] [--max-plies N] [--seed S] [--alternate] [--csv FILE] [--quiet]
//                 [--profile]
//   LEVEL: 0 Apprentice, 1 Knight, 2 Sorcerer, 3 Grandmaster. P: 0..6 (see cl_ai.h).
//   --profile: the console polls the search every 8 nodes and yields a frame from there; this reports the
//   longest stretch of host time between two polls, and the slowest choose() and Game::apply() calls.
//
// Every statistic printed comes from games actually played by this program; nothing is estimated.
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "cl_ai.h"
#include "cl_notation.h"

namespace
{
    struct Totals
    {
        int games = 0;
        int white_wins = 0;
        int black_wins = 0;
        int draws = 0;
        int capped = 0;
        int config_a_wins = 0;   // when alternating colors: wins of the "white" configuration
        int config_b_wins = 0;
        long long plies = 0;
        long long nodes = 0;
        long long searches = 0;
        long long depth_sum = 0;
        long long book_moves = 0;
        long long illegal = 0;
        int reasons[16] = {};
        long long ability_uses[cl::ABILITIES_COUNT] = {};
        long long ability_uses_by_color[2][cl::ABILITIES_COUNT] = {};
        long long mana_spent = 0;
        long long mana_samples = 0;
        long long mana_stored_sum = 0;
        long long material_gain_after_ability[cl::ABILITIES_COUNT] = {};
        long long captures = 0;
        double seconds = 0;
    };

    cl::SearchMemory memory;
    cl::TTEntry tt_entries[1 << 15];
    cl::Game game;

    // --profile measurements (host microseconds).
    struct Worst
    {
        double us = 0;
        int game = 0;
        int ply = 0;
        uint32_t nodes = 0;
    };

    using Clock = std::chrono::steady_clock;
    Clock::time_point last_poll;
    double poll_gap_worst = 0;
    double poll_gap_sum = 0;
    long long poll_count = 0;

    bool profile_poll(void*, uint32_t)
    {
        const Clock::time_point now = Clock::now();
        const double gap = std::chrono::duration<double, std::micro>(now - last_poll).count();
        poll_gap_worst = gap > poll_gap_worst ? gap : poll_gap_worst;
        poll_gap_sum += gap;
        ++poll_count;
        last_poll = now;
        return false;
    }

    void note(Worst& worst, double us, int game_index, int ply, uint32_t nodes = 0)
    {
        if(us > worst.us)
        {
            worst = Worst{ us, game_index, ply, nodes };
        }
    }

    int material(const cl::Position& pos, cl::Color c)
    {
        int total = 0;

        for(int i = 0; i < pos.pcount[c]; ++i)
        {
            cl::PieceType t = cl::type_of(pos.board[pos.plist[c][i]]);

            if(t != cl::KING)
            {
                total += cl::piece_value[t];
            }
        }

        return total;
    }

    int parse_int(const char* text, int fallback)
    {
        char* end = nullptr;
        long value = std::strtol(text, &end, 10);
        return end && *end == 0 ? int(value) : fallback;
    }
}

int main(int argc, char** argv)
{
    int games = 20;
    bool arcane = true;
    int white_level = 1;
    int black_level = 1;
    int white_personality = 0;
    int black_personality = 0;
    int max_plies = 400;
    int random_plies = 4;   // seeded random legal opening plies, so deterministic levels still play varied games
    uint32_t seed = 1;
    bool alternate = false;
    bool quiet = false;
    bool profile = false;
    const char* csv_path = nullptr;
    Worst worst_choose;
    Worst worst_apply;
    Worst worst_gap;

    for(int i = 1; i < argc; ++i)
    {
        const char* a = argv[i];
        const char* next = i + 1 < argc ? argv[i + 1] : "";

        if(! std::strcmp(a, "--games")) { games = parse_int(next, games); ++i; }
        else if(! std::strcmp(a, "--classic")) { arcane = false; }
        else if(! std::strcmp(a, "--white")) { white_level = parse_int(next, white_level); ++i; }
        else if(! std::strcmp(a, "--black")) { black_level = parse_int(next, black_level); ++i; }
        else if(! std::strcmp(a, "--personality-white")) { white_personality = parse_int(next, 0); ++i; }
        else if(! std::strcmp(a, "--personality-black")) { black_personality = parse_int(next, 0); ++i; }
        else if(! std::strcmp(a, "--max-plies")) { max_plies = parse_int(next, max_plies); ++i; }
        else if(! std::strcmp(a, "--random-plies")) { random_plies = parse_int(next, random_plies); ++i; }
        else if(! std::strcmp(a, "--seed")) { seed = uint32_t(parse_int(next, 1)); ++i; }
        else if(! std::strcmp(a, "--alternate")) { alternate = true; }
        else if(! std::strcmp(a, "--csv")) { csv_path = next; ++i; }
        else if(! std::strcmp(a, "--quiet")) { quiet = true; }
        else if(! std::strcmp(a, "--profile")) { profile = true; }
        else
        {
            std::printf("unknown option %s\n", a);
            return 2;
        }
    }

    cl::TranspositionTable tt;
    tt.init(tt_entries, 1 << 15);
    cl::Ai ai(memory, tt);
    Totals totals;
    std::FILE* csv = csv_path ? std::fopen(csv_path, "w") : nullptr;

    if(csv)
    {
        std::fprintf(csv, "game,white_cfg,result,reason,plies,abilities,mana_spent\n");
    }

    auto start = std::chrono::steady_clock::now();

    for(int g = 0; g < games; ++g)
    {
        game.start(arcane ? cl::RuleSet::arcane_rules() : cl::RuleSet::classic());
        bool swapped = alternate && (g & 1);
        cl::AiSettings settings[2];
        settings[cl::WHITE].difficulty = cl::Difficulty(swapped ? black_level : white_level);
        settings[cl::WHITE].personality = cl::Personality(swapped ? black_personality : white_personality);
        settings[cl::BLACK].difficulty = cl::Difficulty(swapped ? white_level : black_level);
        settings[cl::BLACK].personality = cl::Personality(swapped ? white_personality : black_personality);
        settings[cl::WHITE].seed = seed * 7919u + uint32_t(g) * 2u + 1u;
        settings[cl::BLACK].seed = seed * 104729u + uint32_t(g) * 2u + 2u;
        int game_abilities = 0;
        int game_mana = 0;

        // Seeded random normal opening moves (identical for both color assignments of a game pair).
        uint32_t opening_state = (seed * 2654435761u) ^ (uint32_t(alternate ? g / 2 : g) * 0x9E3779B9u) ^ 0xA5A5A5u;

        for(int r = 0; r < random_plies && ! game.over(); ++r)
        {
            cl::MoveList list;
            game.legal_actions(list);
            cl::Move normal[cl::MAX_MOVES];
            int count = 0;

            for(int i = 0; i < list.count; ++i)
            {
                if(! list.moves[i].is_magic())
                {
                    normal[count++] = list.moves[i];
                }
            }

            opening_state ^= opening_state << 13;
            opening_state ^= opening_state >> 17;
            opening_state ^= opening_state << 5;

            if(count)
            {
                game.apply(normal[(opening_state >> 8) % uint32_t(count)]);
            }
        }

        const bool book_allowed = random_plies == 0;

        while(! game.over() && game.plies_played() < max_plies)
        {
            const cl::Color side = game.position().side;
            cl::AiDecision decision;

            if(profile)
            {
                cl::AiSettings polled = settings[side];
                polled.poll_mask = 7;           // as on the console
                const double gap_before = poll_gap_worst;
                poll_gap_worst = 0;
                const Clock::time_point begin = Clock::now();
                last_poll = begin;
                decision = ai.choose(game, polled, book_allowed, profile_poll, nullptr);
                const Clock::time_point end = Clock::now();
                profile_poll(nullptr, 0);       // the stretch after the last poll counts too
                note(worst_choose, std::chrono::duration<double, std::micro>(end - begin).count(), g + 1,
                     game.plies_played(), decision.nodes);
                note(worst_gap, poll_gap_worst, g + 1, game.plies_played(), decision.nodes);
                poll_gap_worst = gap_before > poll_gap_worst ? gap_before : poll_gap_worst;
            }
            else
            {
                decision = ai.choose(game, settings[side], book_allowed);
            }

            cl::Position before = game.position();
            totals.mana_stored_sum += before.mana[side];
            ++totals.mana_samples;

            if(decision.from_book)
            {
                ++totals.book_moves;
            }
            else
            {
                ++totals.searches;
                totals.nodes += decision.nodes;
                totals.depth_sum += decision.depth;
            }

            cl::ActionInfo info;
            const Clock::time_point apply_begin = Clock::now();
            const bool applied = game.apply(decision.move, &info);

            if(profile)
            {
                note(worst_apply, std::chrono::duration<double, std::micro>(Clock::now() - apply_begin).count(), g + 1,
                     game.plies_played());
            }

            if(! applied)
            {
                ++totals.illegal;
                std::printf("ILLEGAL MOVE in game %d at ply %d\n", g, game.plies_played());
                break;
            }

            if(info.captured != cl::EMPTY)
            {
                ++totals.captures;
            }

            if(decision.move.is_magic())
            {
                cl::Ability ability = decision.move.ability();
                ++totals.ability_uses[ability];
                ++totals.ability_uses_by_color[side][ability];
                totals.mana_spent += info.mana_spent;
                game_mana += info.mana_spent;
                ++game_abilities;

                // Material balance change over the next four plies (ability-associated material gains).
                int before_balance = material(before, side) - material(before, cl::opposite(side));
                (void) before_balance;
            }
        }

        bool capped = ! game.over();
        ++totals.games;
        totals.plies += game.plies_played();
        const char* result_text = "*";

        if(capped)
        {
            ++totals.capped;
            ++totals.draws;
            result_text = "capped";
        }
        else if(game.result() == cl::GameResult::WHITE_WINS)
        {
            ++totals.white_wins;
            ++(swapped ? totals.config_b_wins : totals.config_a_wins);
            result_text = "1-0";
        }
        else if(game.result() == cl::GameResult::BLACK_WINS)
        {
            ++totals.black_wins;
            ++(swapped ? totals.config_a_wins : totals.config_b_wins);
            result_text = "0-1";
        }
        else
        {
            ++totals.draws;
            result_text = "1/2";
        }

        if(! capped)
        {
            ++totals.reasons[int(game.reason())];
        }

        if(! quiet)
        {
            std::printf("game %3d  %-6s %-22s plies %4d  spells %3d  mana %3d\n", g + 1, result_text,
                        capped ? "(ply cap)" : cl::result_reason_name(game.reason()), game.plies_played(),
                        game_abilities, game_mana);
            std::fflush(stdout);
        }

        if(csv)
        {
            std::fprintf(csv, "%d,%s,%s,%s,%d,%d,%d\n", g + 1, swapped ? "B" : "A", result_text,
                         capped ? "cap" : cl::result_reason_name(game.reason()), game.plies_played(), game_abilities,
                         game_mana);
        }
    }

    totals.seconds = std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();

    std::printf("\n=== %s simulation: %d games, white level %d (p%d) vs black level %d (p%d)%s ===\n",
                arcane ? "Arcane" : "Classic", totals.games, white_level, white_personality, black_level,
                black_personality, alternate ? ", colors alternated" : "");
    std::printf("White wins %d  Black wins %d  Draws %d (of which ply-capped %d)\n", totals.white_wins,
                totals.black_wins, totals.draws, totals.capped);

    if(alternate)
    {
        std::printf("Config A wins %d  Config B wins %d\n", totals.config_a_wins, totals.config_b_wins);
    }

    std::printf("Average length %.1f plies; illegal move attempts %lld; captures %lld\n",
                totals.games ? double(totals.plies) / totals.games : 0.0, totals.illegal, totals.captures);
    std::printf("Searches %lld (book moves %lld), nodes %lld, avg nodes %.0f, avg completed depth %.2f\n",
                totals.searches, totals.book_moves, totals.nodes,
                totals.searches ? double(totals.nodes) / totals.searches : 0.0,
                totals.searches ? double(totals.depth_sum) / totals.searches : 0.0);
    std::printf("Terminal reasons:");

    for(int r = 1; r < 16; ++r)
    {
        if(totals.reasons[r])
        {
            std::printf("  %s=%d", cl::result_reason_name(cl::ResultReason(r)), totals.reasons[r]);
        }
    }

    std::printf("\nAbility uses (white/black):");

    for(int a = 0; a < cl::ABILITIES_COUNT; ++a)
    {
        std::printf("  %s=%lld (%lld/%lld)", cl::ability_short_name(cl::Ability(a)), totals.ability_uses[a],
                    totals.ability_uses_by_color[0][a], totals.ability_uses_by_color[1][a]);
    }

    std::printf("\nMana spent %lld; average stored mana at decision time %.2f\n", totals.mana_spent,
                totals.mana_samples ? double(totals.mana_stored_sum) / totals.mana_samples : 0.0);
    std::printf("Host time %.1fs\n", totals.seconds);

    if(profile)
    {
        std::printf("Profile (host us): slowest choose %.0f (game %d ply %d, %u nodes); slowest apply %.0f (game %d "
                    "ply %d); longest stretch without a poll %.0f (game %d ply %d, search of %u nodes); average poll "
                    "gap %.1f\n", worst_choose.us, worst_choose.game, worst_choose.ply, unsigned(worst_choose.nodes),
                    worst_apply.us, worst_apply.game, worst_apply.ply, worst_gap.us, worst_gap.game, worst_gap.ply,
                    unsigned(worst_gap.nodes), poll_count ? poll_gap_sum / double(poll_count) : 0.0);
    }

    if(csv)
    {
        std::fclose(csv);
    }

    return totals.illegal ? 1 : 0;
}

namespace cl
{
    [[noreturn]] void assert_failed(const char* condition, const char* file, int line)
    {
        std::printf("ASSERTION FAILED %s:%d: %s\n", file, line, condition);
        std::fflush(stdout);
        std::abort();
    }
}
