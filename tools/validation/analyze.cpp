// CHESS LEGENDS: ARCANE WARS - puzzle candidate analyzer (host tool).
//
// Usage: analyze "<fen>" [max_moves]       -- shortest forced mate with all abilities, then without each ability
//        analyze --material "<fen>" moves  -- forced material gain
//        analyze --survive "<fen>" moves   -- can the side to move survive (with and without Royal Escape)
//        analyze --batch file              -- lines "mate|fen|moves", "material|fen|moves", "survive|fen|moves"
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "cl_notation.h"
#include "cl_solver.h"

namespace cl
{
    [[noreturn]] void assert_failed(const char* condition, const char* file, int line)
    {
        std::printf("ASSERTION FAILED %s:%d: %s\n", file, line, condition);
        std::abort();
    }
}

namespace
{
    bool setup(cl::Position& pos, const char* fen, uint8_t mask)
    {
        const bool arcane = std::strstr(fen, "mana=") != nullptr;
        cl::RuleSet rules = arcane ? cl::RuleSet::arcane_rules() : cl::RuleSet::classic();
        cl::FenError error = pos.set_fen(fen, rules);

        if(error != cl::FenError::NONE)
        {
            std::printf("FEN error %d\n", int(error));
            return false;
        }

        // The mask restricts the side to move (the solver's attacker / survivor).
        pos.rules.ability_mask[pos.side] = mask;
        pos.rules.ability_mask[cl::opposite(pos.side)] = cl::ABILITY_MASK_ALL;
        return true;
    }

    int shortest_mate(const char* fen, uint8_t mask, int max_moves, cl::Move* first)
    {
        for(int n = 1; n <= max_moves; ++n)
        {
            cl::Position pos;

            if(! setup(pos, fen, mask))
            {
                return -2;
            }

            cl::SolverStats stats;
            stats.node_limit = 400000000;

            if(cl::forced_mate(pos, n, stats, first))
            {
                return n;
            }

            if(stats.aborted)
            {
                return -1;
            }
        }

        return 0;
    }

    void describe(cl::Move move, char* text)
    {
        cl::move_to_coordinate(move, text, 24);
    }

    void analyze_mate(const char* fen, int max_moves)
    {
        cl::Move first;
        char text[24];
        const int all = shortest_mate(fen, cl::ABILITY_MASK_ALL, max_moves, &first);
        describe(first, text);
        std::printf("  all abilities: %s %d (first %s)\n", all > 0 ? "mate in" : "result", all, all > 0 ? text : "-");

        if(std::strstr(fen, "mana="))
        {
            std::printf("  no abilities: %d\n", shortest_mate(fen, 0, max_moves, nullptr));

            for(int a = 0; a < cl::ABILITIES_COUNT; ++a)
            {
                const int without = shortest_mate(fen, uint8_t(cl::ABILITY_MASK_ALL & ~(1 << a)), max_moves, nullptr);
                std::printf("    without %-14s %d\n", cl::ability_name(cl::Ability(a)), without);
            }
        }
    }

    void analyze_material(const char* fen, int moves)
    {
        char text[24];

        for(int pass = 0; pass < 2; ++pass)
        {
            cl::Position pos;

            if(! setup(pos, fen, pass == 0 ? cl::ABILITY_MASK_ALL : 0))
            {
                return;
            }

            cl::SolverStats stats;
            cl::Move first;
            const int gain = cl::forced_material_gain(pos, moves, stats, &first);
            describe(first, text);
            std::printf("  %s: forced gain %d in %d moves (first %s, nodes %llu%s)\n",
                        pass == 0 ? "all abilities" : "no abilities", gain, moves, text,
                        (unsigned long long) stats.nodes, stats.aborted ? ", ABORTED" : "");

            if(! std::strstr(fen, "mana="))
            {
                break;
            }
        }
    }

    void analyze_promote(const char* fen, int moves)
    {
        static uint64_t keys[1 << 20];
        static uint8_t values[1 << 20];
        char text[24];

        for(int pass = -2; pass < cl::ABILITIES_COUNT; ++pass)
        {
            uint8_t mask = pass == -2 ? cl::ABILITY_MASK_ALL : (pass == -1 ? 0 : uint8_t(cl::ABILITY_MASK_ALL & ~(1 << pass)));
            cl::Position pos;

            if(! setup(pos, fen, mask))
            {
                return;
            }

            for(uint64_t& key : keys)
            {
                key = 0;
            }

            cl::SolverCache cache;
            cache.keys = keys;
            cache.values = values;
            cache.mask = (1 << 20) - 1;
            int found = 0;
            cl::Move first;
            cl::SolverStats stats;
            stats.cache = &cache;
            stats.node_limit = 2000000000;

            for(int n = 1; n <= moves && ! found && ! stats.aborted; ++n)
            {
                if(cl::forced_promotion(pos, n, stats, &first))
                {
                    found = n;
                }
            }

            describe(first, text);
            const char* label = pass == -2 ? "all abilities" : (pass == -1 ? "no abilities" : cl::ability_name(cl::Ability(pass)));
            std::printf("  %s%-14s: promotion in %d (first %s, nodes %llu%s)\n", pass >= 0 ? "without " : "", label,
                        found, found ? text : "-", (unsigned long long) stats.nodes, stats.aborted ? ", ABORTED" : "");

            if(! std::strstr(fen, "mana=") && pass == -2)
            {
                break;
            }
        }
    }

    void analyze_survive(const char* fen, int moves)
    {
        char text[24];

        for(int pass = 0; pass < 3; ++pass)
        {
            uint8_t mask = cl::ABILITY_MASK_ALL;

            if(pass == 1)
            {
                mask = uint8_t(cl::ABILITY_MASK_ALL & ~(1 << cl::ABILITY_ROYAL_ESCAPE));
            }
            else if(pass == 2)
            {
                mask = 0;
            }

            cl::Position pos;

            if(! setup(pos, fen, mask))
            {
                return;
            }

            cl::SolverStats stats;
            cl::Move first;
            const bool ok = cl::can_survive(pos, moves, stats, &first);
            describe(first, text);
            const char* label = pass == 0 ? "all abilities" : (pass == 1 ? "no Royal Escape" : "no abilities");
            std::printf("  %s: survive %d moves: %s (first %s, nodes %llu%s)\n", label, moves, ok ? "YES" : "NO",
                        ok ? text : "-", (unsigned long long) stats.nodes, stats.aborted ? ", ABORTED" : "");

            if(! std::strstr(fen, "mana="))
            {
                break;
            }
        }
    }
}

int main(int argc, char** argv)
{
    if(argc >= 3 && ! std::strcmp(argv[1], "--batch"))
    {
        std::FILE* file = std::fopen(argv[2], "r");
        char line[512];

        while(file && std::fgets(line, sizeof(line), file))
        {
            if(line[0] == '#' || line[0] == '\n' || line[0] == '\r')
            {
                continue;
            }

            char* kind = std::strtok(line, "|");
            char* fen = std::strtok(nullptr, "|");
            char* moves = std::strtok(nullptr, "|\r\n");

            if(! kind || ! fen || ! moves)
            {
                continue;
            }

            std::printf("=== %s | %s | %s\n", kind, fen, moves);

            if(! std::strcmp(kind, "mate"))
            {
                analyze_mate(fen, std::atoi(moves));
            }
            else if(! std::strcmp(kind, "material"))
            {
                analyze_material(fen, std::atoi(moves));
            }
            else
            {
                analyze_survive(fen, std::atoi(moves));
            }

            std::fflush(stdout);
        }

        return 0;
    }

    if(argc >= 4 && ! std::strcmp(argv[1], "--material"))
    {
        analyze_material(argv[2], std::atoi(argv[3]));
        return 0;
    }

    if(argc >= 4 && ! std::strcmp(argv[1], "--survive"))
    {
        analyze_survive(argv[2], std::atoi(argv[3]));
        return 0;
    }

    if(argc >= 4 && ! std::strcmp(argv[1], "--promote"))
    {
        analyze_promote(argv[2], std::atoi(argv[3]));
        return 0;
    }

    if(argc >= 2)
    {
        analyze_mate(argv[1], argc >= 3 ? std::atoi(argv[2]) : 3);
        return 0;
    }

    std::printf("usage: analyze FEN [max] | --material FEN moves | --survive FEN moves | --batch file\n");
    return 2;
}
