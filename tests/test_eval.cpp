// Evaluation sanity: color symmetry, material awareness, mana valuation.
#include "test_util.h"

#include <cctype>
#include <cstring>
#include <string>

#include "cl_ai.h"
#include "cl_eval.h"

namespace
{
    // Mirrors a FEN vertically and swaps colors (including the Arcane fields).
    std::string mirror_fen(const char* fen)
    {
        std::string text(fen);
        std::string fields[12];
        int count = 0;
        size_t start = 0;

        while(start < text.size() && count < 12)
        {
            size_t end = text.find(' ', start);

            if(end == std::string::npos)
            {
                end = text.size();
            }

            fields[count++] = text.substr(start, end - start);
            start = end + 1;
        }

        // Board: reverse rank order, swap case.
        std::string ranks[8];
        int r = 0;
        size_t s = 0;

        while(r < 8)
        {
            size_t e = fields[0].find('/', s);

            if(e == std::string::npos)
            {
                e = fields[0].size();
            }

            ranks[r++] = fields[0].substr(s, e - s);
            s = e + 1;
        }

        std::string board;

        for(int i = 7; i >= 0; --i)
        {
            for(char c : ranks[i])
            {
                board += char(std::isupper(c) ? std::tolower(c) : std::isalpha(c) ? std::toupper(c) : c);
            }

            if(i)
            {
                board += '/';
            }
        }

        std::string side = fields[1] == "w" ? "b" : "w";
        std::string castling;

        for(char c : fields[2])
        {
            castling += char(c == '-' ? '-' : std::isupper(c) ? std::tolower(c) : std::toupper(c));
        }

        // Keep FEN castling order KQkq.
        std::string ordered;

        for(char c : std::string("KQkq"))
        {
            if(castling.find(c) != std::string::npos)
            {
                ordered += c;
            }
        }

        if(ordered.empty())
        {
            ordered = "-";
        }

        std::string ep = fields[3];

        if(ep != "-")
        {
            ep[1] = char('1' + ('8' - ep[1]));
        }

        std::string result = board + " " + side + " " + ordered + " " + ep + " " + fields[4] + " " + fields[5];

        for(int i = 6; i < count; ++i)
        {
            std::string f = fields[i];
            size_t eq = f.find('=');
            size_t slash = f.find('/');
            std::string key = f.substr(0, eq + 1);
            std::string a = f.substr(eq + 1, slash - eq - 1);
            std::string b = f.substr(slash + 1);
            result += " " + key + b + "/" + a;
        }

        return result;
    }
}

TEST_CASE("eval", evaluation_is_color_symmetric)
{
    const char* fens[] = {
        "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
        "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1",
        "r4rk1/1pp1qppp/p1np1n2/2b1p1B1/2B1P1b1/P1NP1N2/1PP1QPPP/R4RK1 w - - 0 10",
        "8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1",
        "4k3/8/8/8/8/8/8/R3K3 w - - 0 1",
        "6k1/5ppp/8/8/8/8/5PPP/3RR1K1 b - - 0 1",
    };
    const char* mana_suffix[] = { "", " mana=3/1", " mana=6/2 escape=0/1", " mana=5/5" };
    cl::EvalProfile profile;

    for(const char* fen : fens)
    {
        for(int arcane = 0; arcane < 2; ++arcane)
        {
            for(const char* suffix : mana_suffix)
            {
                if(! arcane && suffix[0])
                {
                    continue;
                }

                std::string original = std::string(fen) + suffix;
                std::string mirrored = mirror_fen(original.c_str());
                cl::RuleSet rules = arcane ? cl::RuleSet::arcane_rules() : cl::RuleSet::classic();
                cl::Position a;
                cl::Position b;
                CHECK(a.set_fen(original.c_str(), rules) == cl::FenError::NONE);
                CHECK_MSG(b.set_fen(mirrored.c_str(), rules) == cl::FenError::NONE, mirrored.c_str());
                int ea = cl::evaluate(a, profile);
                int eb = cl::evaluate(b, profile);

                if(ea != eb)
                {
                    std::printf("    asymmetric: %s -> %d   mirrored %s -> %d\n", original.c_str(), ea,
                                mirrored.c_str(), eb);
                }

                CHECK_EQ(ea, eb);
            }
        }
    }
}

TEST_CASE("eval", material_and_mana_are_valued)
{
    cl::EvalProfile profile;
    cl::Position pos;
    CHECK(pos.set_fen("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBN1 w Qkq - 0 1", cl::RuleSet::classic()) ==
          cl::FenError::NONE);
    CHECK(cl::evaluate(pos, profile) < -300);   // White is a rook down

    // More mana is better; ability thresholds add bumps, but the last point before the cap is worth the least
    // (regeneration beyond the cap is wasted, so hoarding at the cap must not look attractive).
    cl::RuleSet arcane = cl::RuleSet::arcane_rules();
    int scores[7];

    for(int m = 0; m <= 6; ++m)
    {
        char fen[128];
        std::snprintf(fen, sizeof(fen), "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 mana=%d/3", m);
        CHECK(pos.set_fen(fen, arcane) == cl::FenError::NONE);
        scores[m] = cl::evaluate(pos, profile);

        if(m)
        {
            CHECK(scores[m] > scores[m - 1]);
        }
    }

    for(int m = 1; m < 6; ++m)
    {
        CHECK(scores[6] - scores[5] <= scores[m] - scores[m - 1]);
    }

    // An unused Royal Escape is worth something.
    cl::Position with_escape;
    cl::Position without_escape;
    CHECK(with_escape.set_fen("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1 mana=5/5", arcane) == cl::FenError::NONE);
    CHECK(without_escape.set_fen("4k3/pppppppp/8/8/8/8/PPPPPPPP/4K3 w - - 0 1 mana=5/5 escape=0/1", arcane) ==
          cl::FenError::NONE);
    CHECK(cl::evaluate(with_escape, profile) > cl::evaluate(without_escape, profile));
}

TEST_CASE("eval", lazy_evaluation_matches_inside_window)
{
    cl::EvalProfile profile;
    cl::Position pos;
    CHECK(pos.set_fen("r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 mana=3/3",
                      cl::RuleSet::arcane_rules()) == cl::FenError::NONE);
    int full = cl::evaluate(pos, profile);
    CHECK_EQ(cl::evaluate_lazy(pos, profile, full - 50, full + 50), full);
    // Far outside the window the cheap estimate is returned, on the correct side of the window.
    CHECK(cl::evaluate_lazy(pos, profile, full + 2000, full + 2001) <= full + 2000);
    CHECK(cl::evaluate_lazy(pos, profile, full - 2001, full - 2000) >= full - 2000);
}
