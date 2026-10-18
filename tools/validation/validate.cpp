// CHESS LEGENDS: ARCANE WARS - content validator (host tool).
//
// Proves the shipped campaign and challenge content with the exact solvers (AND/OR searches over every legal action
// and ability of both sides) and checks every string against the real font metrics and on-screen text boxes:
//   challenges  valid position, forced solution within par, none faster, the stated key move wins by itself, every
//               required ability is necessary, the limit leaves room for the medals
//   encounters  valid position and rules, player to move, objective consistent with the briefing, every sealed or
//               capped rule announced, short puzzle trials proven solvable within their limit
//   text        every character exists in its font, every line fits its box (dialogue, briefings, hints, titles)
// Exit code 0 only when everything passes.
//
// Usage: validate [--quick]      (--quick skips the solver proofs)
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>

#include "cl_campaign.h"
#include "cl_help.h"
#include "cl_movegen.h"
#include "cl_notation.h"
#include "cl_progress.h"
#include "cl_solver.h"
#include "cl_text_layout.h"
#include "cl_tutorial.h"
#include "cl_ui_text.h"

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
    int checks = 0;
    int failures = 0;
    bool quick = false;

    void check(bool ok, const char* where, const char* format, ...)
    {
        ++checks;

        if(! ok)
        {
            ++failures;
            std::printf("  FAIL [%s] ", where);
            va_list args;
            va_start(args, format);
            std::vprintf(format, args);
            va_end(args);
            std::printf("\n");
        }
    }

    // ---------------------------------------------------------------------------------------------- text checks

    void check_text(const char* where, const char* text, cl::FontId font, int width, int max_lines)
    {
        check(text && text[0], where, "empty text");

        if(! text || ! text[0])
        {
            return;
        }

        check(cl::font_supports(font, text), where, "character missing from the font: \"%s\"", text);
        cl::TextSpan spans[16];
        bool overflow = false;
        const int lines = cl::wrap_lines(font, text, int(std::strlen(text)), width, spans, max_lines, &overflow);
        check(! overflow && lines <= max_lines, where, "does not fit %d line(s) of %d px: \"%s\"", max_lines, width,
              text);

        // A wrapped line never splits a word (that would mean a single word wider than the box).
        for(int i = 0; i < lines; ++i)
        {
            const cl::TextSpan& span = spans[i];
            const int end = span.start + span.length;
            const bool split = end < int(std::strlen(text)) && text[end] != ' ' && text[end] != '\n';
            check(! split, where, "word split across lines: \"%s\"", text);
        }
    }

    void check_dialogue(const char* where, const cl::Dialogue& dialogue)
    {
        check(dialogue.lines && dialogue.count > 0, where, "dialogue has no lines");

        for(int i = 0; i < dialogue.count; ++i)
        {
            const cl::DialogueLine& line = dialogue.lines[i];
            char label[96];
            std::snprintf(label, sizeof(label), "%s line %d", where, i + 1);
            check(int(line.speaker) < int(cl::Character::COUNT), label, "invalid speaker");
            check_text(label, line.text, cl::FontId::SMALL, cl::layout::dialogue_text_width, cl::layout::dialogue_lines);
        }
    }

    bool contains(const char* text, const char* word)
    {
        return text && std::strstr(text, word);
    }

    // ---------------------------------------------------------------------------------------------- solver proofs

    struct Memo
    {
        static constexpr uint32_t bits = 22;
        std::vector<uint64_t> keys;
        std::vector<uint8_t> values;
        cl::SolverCache cache;

        Memo() :
            keys(1u << bits),
            values(1u << bits)
        {
            cache.keys = keys.data();
            cache.values = values.data();
            cache.mask = (1u << bits) - 1;
        }
    };

    Memo& memo()
    {
        static Memo instance;
        return instance;
    }

    enum Proof
    {
        PROOF_NO = 0,
        PROOF_YES = 1,
        PROOF_ABORTED = -1,
        PROOF_UNSUPPORTED = -2
    };

    // Can the side to move achieve the objective within `moves` of its moves against every defense? `mask` replaces
    // the mover's ability mask; `key`, if set, restricts the first action to it.
    Proof prove(const cl::Position& start, const cl::ObjectiveSpec& objective, int moves, uint8_t mask,
                cl::Move key = cl::Move(), cl::Move* found = nullptr)
    {
        cl::Position pos = start;
        pos.rules.ability_mask[pos.side] = mask;
        cl::SolverStats stats;
        stats.node_limit = 3000000000ull;
        stats.cache = &memo().cache;
        stats.root_only = key;
        cl::Move first;
        bool ok = false;

        switch(objective.type)
        {
        case cl::Objective::CHECKMATE_WITHIN:
            ok = cl::forced_mate(pos, moves, stats, &first);
            break;

        case cl::Objective::WIN_MATERIAL:
            ok = cl::forced_material_lead(pos, moves, objective.value * 100, stats, &first);
            break;

        case cl::Objective::SURVIVE:
            ok = cl::can_survive(pos, moves, stats, &first);
            break;

        case cl::Objective::PROMOTE:
            ok = cl::forced_promotion(pos, moves, stats, &first);
            break;

        default:
            return PROOF_UNSUPPORTED;
        }

        if(found)
        {
            *found = first;
        }

        return stats.aborted ? PROOF_ABORTED : (ok ? PROOF_YES : PROOF_NO);
    }

    const char* objective_report_name(cl::Objective objective)
    {
        switch(objective)
        {
        case cl::Objective::WIN: return "win";
        case cl::Objective::CHECKMATE_WITHIN: return "mate within";
        case cl::Objective::SURVIVE: return "survive";
        case cl::Objective::PROMOTE: return "promote within";
        case cl::Objective::WIN_MATERIAL: return "material within";
        case cl::Objective::CAPTURE_PAWNS: return "capture pawns within";
        default: return "?";
        }
    }

    void abilities_text(uint8_t mask, char* out, int capacity)
    {
        out[0] = 0;
        int length = 0;

        for(int a = 0; a < cl::ABILITIES_COUNT; ++a)
        {
            if(mask & (1 << a))
            {
                length += std::snprintf(out + length, size_t(capacity - length), "%s%s", length ? "+" : "",
                                        cl::ability_short_name(cl::Ability(a)));
            }
        }

        if(! length)
        {
            std::snprintf(out, size_t(capacity), "-");
        }
    }

    // ---------------------------------------------------------------------------------------------- challenges

    void validate_challenge(int index)
    {
        const cl::Challenge& c = cl::challenge(index);
        char where[64];
        std::snprintf(where, sizeof(where), "challenge %d %s", index + 1, c.title);

        check_text(where, c.title, cl::FontId::SMALL, cl::layout::list_text_width, 1);
        check_text(where, c.theme, cl::FontId::SMALL, cl::layout::list_text_width, 1);
        check_text(where, c.hint, cl::FontId::SMALL, cl::layout::hint_text_width, cl::layout::hint_lines);

        cl::Game game;
        const bool ok = cl::setup_challenge(c, game);
        check(ok, where, "FEN rejected: %s", c.fen);

        if(! ok)
        {
            return;
        }

        cl::Position pos = game.position();
        check(! game.over(), where, "game is already over at setup");
        check(pos.validate(), where, "position fails validation");
        check(c.arcane == (std::strstr(c.fen, "mana=") != nullptr), where, "arcane flag and FEN mana disagree");
        check(c.par >= 1 && c.objective.limit >= c.par, where, "par %d / limit %d inconsistent", c.par,
              c.objective.limit);
        check(c.arcane || c.required_abilities == 0, where, "classic challenge cannot require abilities");
        check(c.objective.type != cl::Objective::SURVIVE || c.par == c.objective.limit, where,
              "survival par must equal its limit");

        cl::Move key = cl::Move();

        if(c.solution)
        {
            key = cl::parse_coordinate(pos, c.solution);
            check(! key.is_null() && cl::is_legal(pos, key), where, "solution '%s' is not a legal action", c.solution);
        }
        else
        {
            check(false, where, "no solution key");
        }

        char required[48];
        abilities_text(c.required_abilities, required, sizeof(required));

        if(quick)
        {
            std::printf("  %-2d %-18s %-15s par %d limit %d  needs %-6s (not proven: --quick)\n", index + 1, c.title,
                        objective_report_name(c.objective.type), c.par, c.objective.limit, required);
            return;
        }

        const uint8_t all = pos.rules.ability_mask[pos.side];
        cl::Move first;
        const Proof solvable = prove(pos, c.objective, c.par, all, cl::Move(), &first);
        check(solvable == PROOF_YES, where, "no forced solution within par %d (proof %d)", c.par, int(solvable));

        if(c.par > 1 && c.objective.type != cl::Objective::SURVIVE)
        {
            const Proof faster = prove(pos, c.objective, c.par - 1, all);
            check(faster == PROOF_NO, where, "solvable faster than par %d (proof %d)", c.par, int(faster));
        }

        if(! key.is_null())
        {
            const Proof key_works = prove(pos, c.objective, c.par, all, key);
            check(key_works == PROOF_YES, where, "key '%s' does not force the solution within par", c.solution);
        }

        for(int a = 0; a < cl::ABILITIES_COUNT; ++a)
        {
            if(c.required_abilities & (1 << a))
            {
                const Proof without = prove(pos, c.objective, c.par, uint8_t(all & ~(1 << a)));
                check(without == PROOF_NO, where, "%s is not required (proof %d)", cl::ability_name(cl::Ability(a)),
                      int(without));
            }
        }

        // Informational: is the challenge solvable within its full limit without any ability?
        const char* classic_note = "";

        if(c.arcane && c.objective.limit <= 3)
        {
            classic_note = prove(pos, c.objective, c.objective.limit, 0) == PROOF_YES ? "  (no-magic route within limit)" :
                                                                                         "  (magic needed within limit)";
        }

        char key_text[24];
        cl::move_to_coordinate(key, key_text, sizeof(key_text));
        std::printf("  %-2d %-18s %-15s par %d limit %d  key %-11s needs %-6s %s%s\n", index + 1, c.title,
                    objective_report_name(c.objective.type), c.par, c.objective.limit, key_text, required,
                    solvable == PROOF_YES ? "PROVEN" : "UNPROVEN", classic_note);
    }

    // ---------------------------------------------------------------------------------------------- encounters

    bool briefing_has_number(const char* briefing, int number)
    {
        char text[8];
        std::snprintf(text, sizeof(text), "%d", number);
        return contains(briefing, text);
    }

    void validate_encounter(int index)
    {
        const cl::Encounter& e = cl::encounter(index);
        char where[64];
        std::snprintf(where, sizeof(where), "encounter %d %s", index + 1, e.title);

        check(e.world == index / 3, where, "world %d out of order", e.world);
        const cl::EncounterType expected = index % 3 == 2 ? cl::EncounterType::BOSS :
                                           (index == 0 ? cl::EncounterType::LESSON : cl::EncounterType::TRIAL);
        check(e.type == expected, where, "encounter type out of place");
        check(index % 3 != 2 || e.opponent == cl::world(e.world).boss, where, "boss encounter is not the world boss");
        check(e.player == cl::WHITE || e.player == cl::BLACK, where, "invalid player color");
        check(e.node_scale >= 25 && e.node_scale <= 200, where, "node scale %d outside 25-200", e.node_scale);
        check(e.max_mana == 0 || (e.max_mana >= 2 && e.max_mana <= 6), where, "max mana %d invalid", e.max_mana);
        check_text(where, e.title, cl::FontId::BIG, cl::layout::title_text_width, 1);
        check_text(where, e.briefing, cl::FontId::SMALL, cl::layout::briefing_text_width, cl::layout::briefing_lines);

        char label[96];
        std::snprintf(label, sizeof(label), "%s intro", where);
        check_dialogue(label, e.intro);
        std::snprintf(label, sizeof(label), "%s victory", where);
        check_dialogue(label, e.victory);
        std::snprintf(label, sizeof(label), "%s defeat", where);
        check_dialogue(label, e.defeat);

        // Every special rule is part of the briefing (design section 48).
        if(e.max_mana)
        {
            check(contains(e.briefing, "mana") && briefing_has_number(e.briefing, e.max_mana), where,
                  "mana cap %d not announced in the briefing", e.max_mana);
        }

        for(int a = 0; a < cl::ABILITIES_COUNT; ++a)
        {
            const bool sealed = ! (e.player_abilities & (1 << a)) || ! (e.enemy_abilities & (1 << a));
            check(! sealed || contains(e.briefing, cl::ability_name(cl::Ability(a))), where,
                  "sealed ability %s not announced", cl::ability_name(cl::Ability(a)));
        }

        if(e.objective.limit)
        {
            check(e.objective.type != cl::Objective::WIN, where, "WIN objective cannot have a move limit");
            check(briefing_has_number(e.briefing, e.objective.limit), where, "move limit %d not in the briefing",
                  e.objective.limit);
        }
        else
        {
            check(e.objective.type == cl::Objective::WIN, where, "objective needs a move limit");
        }

        if(e.objective.type == cl::Objective::WIN_MATERIAL)
        {
            check(e.objective.value > 0 && briefing_has_number(e.briefing, e.objective.value), where,
                  "material target not in the briefing");
        }

        cl::Game game;
        const bool ok = cl::setup_encounter(e, game);
        check(ok, where, "setup failed (FEN: %s)", e.fen ? e.fen : "standard");

        if(! ok)
        {
            return;
        }

        const cl::Position& pos = game.position();
        check(! game.over(), where, "game is already over at setup");
        check(pos.validate(), where, "position fails validation");
        check(pos.side == e.player, where, "the player is not to move at the start");
        check(pos.rules.max_mana == (e.max_mana ? e.max_mana : cl::config::max_mana), where, "mana cap not applied");

        // Puzzle trials (short limits) are proven solvable within their limit.
        const bool puzzle = e.objective.limit && ((e.objective.type == cl::Objective::CHECKMATE_WITHIN &&
                                                   e.objective.limit <= 3) ||
                                                  (e.objective.type == cl::Objective::WIN_MATERIAL &&
                                                   e.objective.limit <= 3) ||
                                                  (e.objective.type == cl::Objective::PROMOTE &&
                                                   e.objective.limit <= 10));
        const char* status = "full game vs AI";
        char detail[96] = "";

        if(puzzle && ! quick)
        {
            const uint8_t mask = pos.rules.ability_mask[pos.side];
            cl::Move first;
            int fastest = 0;

            for(int n = 1; n <= e.objective.limit && ! fastest; ++n)
            {
                if(prove(pos, e.objective, n, mask, cl::Move(), &first) == PROOF_YES)
                {
                    fastest = n;
                }
            }

            check(fastest > 0, where, "puzzle trial not solvable within its limit %d", e.objective.limit);
            status = fastest ? "PROVEN" : "UNPROVEN";

            if(fastest)
            {
                char key_text[24];
                cl::move_to_coordinate(first, key_text, sizeof(key_text));
                uint8_t required = 0;

                for(int a = 0; a < cl::ABILITIES_COUNT; ++a)
                {
                    if((mask & (1 << a)) && prove(pos, e.objective, e.objective.limit, uint8_t(mask & ~(1 << a))) ==
                                                PROOF_NO)
                    {
                        required |= uint8_t(1 << a);
                    }
                }

                char needs[48];
                abilities_text(required, needs, sizeof(needs));
                std::snprintf(detail, sizeof(detail), "in %d, key %s, needs %s within limit", fastest, key_text,
                              needs);
            }
        }
        else if(puzzle)
        {
            status = "puzzle (not proven: --quick)";
        }

        std::printf("  %-2d W%d %-22s %-15s %-3d %-24s %s\n", index + 1, e.world + 1, e.title,
                    objective_report_name(e.objective.type), e.objective.limit, status, detail);
    }

    void validate_story_text()
    {
        check_dialogue("prologue", cl::prologue());
        check_dialogue("epilogue", cl::epilogue());

        for(int w = 0; w < cl::worlds_count; ++w)
        {
            const cl::World& world = cl::world(w);
            char where[64];
            std::snprintf(where, sizeof(where), "world %d", w + 1);
            check_text(where, world.name, cl::FontId::BIG, cl::layout::title_text_width, 1);
            check_text(where, world.subtitle, cl::FontId::SMALL, cl::layout::screen_text_width, 1);
            check(world.faction >= 1 && world.faction <= 6, where, "faction palette out of range");
            check_dialogue(where, world.introduction);
        }

        for(int c = 0; c < int(cl::Character::NARRATOR); ++c)
        {
            const cl::Character character = cl::Character(c);
            check_text("character name", cl::character_name(character), cl::FontId::SMALL, cl::layout::name_tag_width, 1);
            check_text("character title", cl::character_title(character), cl::FontId::SMALL,
                       cl::layout::screen_text_width, 1);
        }
    }
}

namespace
{
    void validate_tutorial()
    {
        cl::Game game;

        for(int i = 0; i < cl::lessons_count; ++i)
        {
            const cl::Lesson& l = cl::lesson(i);
            char where[48];
            std::snprintf(where, sizeof(where), "lesson %d %s", i + 1, l.title);
            check_text(where, l.title, cl::FontId::BIG, cl::layout::title_text_width, 1);
            check_text(where, l.title, cl::FontId::SMALL, cl::layout::list_text_width, 1);
            check(l.pages[0] != nullptr, where, "no instruction page");

            for(const char* page : l.pages)
            {
                if(page)
                {
                    check_text(where, page, cl::FontId::SMALL, cl::layout::card_text_width, 4);
                }
            }

            check_text(where, l.task, cl::FontId::SMALL, cl::layout::panel_text_width, cl::layout::panel_lines);
            check_text(where, l.success, cl::FontId::SMALL, 184, 4);
            const bool ok = cl::setup_lesson(l, game);
            check(ok, where, "position rejected");

            if(! ok)
            {
                continue;
            }

            cl::Position pos = game.position();
            check(! game.over() && pos.side == cl::WHITE, where, "the learner (White) is not to move");
            cl::MoveList legal;
            cl::generate_legal(pos, legal);
            bool achievable = false;
            const char* how = "";

            switch(l.goal)
            {
            case cl::LessonGoal::SELECT_SQUARE:
                achievable = pos.board[l.from] != cl::EMPTY && cl::color_of(pos.board[l.from]) == pos.side;
                how = "own piece on the square";
                break;

            case cl::LessonGoal::MOVE:
                for(int m = 0; m < legal.count; ++m)
                {
                    achievable |= legal.moves[m].from() == l.from && legal.moves[m].to() == l.to;
                }
                how = "legal action";
                break;

            case cl::LessonGoal::CAPTURE:
                for(int m = 0; m < legal.count; ++m)
                {
                    const cl::Move move = legal.moves[m];
                    achievable |= move.is_capture() && cl::type_of(pos.board[move.to()]) == cl::PieceType(l.value);
                }
                how = "legal capture";
                break;

            case cl::LessonGoal::CAST:
                for(int m = 0; m < legal.count; ++m)
                {
                    achievable |= legal.moves[m].is_magic() && int(legal.moves[m].ability()) == l.value;
                }
                how = "legal cast";
                break;

            case cl::LessonGoal::REACH_MANA:
                achievable = pos.rules.arcane && legal.count > 0 &&
                             pos.mana[pos.side] + pos.rules.regeneration >= l.value;
                how = "regeneration";
                break;

            default:
                achievable = quick || prove(pos, l.objective, l.objective.limit, pos.rules.ability_mask[pos.side]) ==
                                      PROOF_YES;
                how = quick ? "objective (not proven: --quick)" : "objective PROVEN";
                break;
            }

            check(achievable, where, "the lesson goal cannot be achieved");
            std::printf("  %-2d %-20s %s\n", i + 1, l.title, how);
        }
    }

    void validate_gauntlet()
    {
        cl::Game game;

        for(int i = 0; i < cl::gauntlet_stages; ++i)
        {
            const cl::GauntletStage& stage = cl::gauntlet_stage(i);
            char where[48];
            std::snprintf(where, sizeof(where), "gauntlet stage %d", i + 1);
            check(stage.boss == cl::world(i).boss, where, "the stage is not the ruler of world %d", i + 1);
            check(cl::setup_gauntlet_stage(stage, game), where, "opening rejected: %s", stage.fen ? stage.fen : "-");
            check(! game.over() && game.position().side == cl::WHITE, where, "the player (White) is not to move");
            check(game.position().validate(), where, "position fails validation");
            check_text(where, stage.opening, cl::FontId::SMALL, cl::layout::screen_text_width, 1);
            check_text(where, stage.taunt, cl::FontId::SMALL, cl::layout::dialogue_text_width,
                       cl::layout::dialogue_lines);
            check(i == 0 || int(stage.difficulty) >= int(cl::gauntlet_stage(i - 1).difficulty), where,
                  "difficulty decreases");
            std::printf("  %d %-12s %-26s %-12s %s %d%%\n", i + 1, cl::character_name(stage.boss), stage.opening,
                        cl::difficulty_name(stage.difficulty), cl::personality_name(stage.personality),
                        stage.node_scale);
        }
    }

    // Match side panel (84 px) and status line: every fixed string and the worst case of every formatted line.
    void validate_match_panel()
    {
        const int width = cl::layout::panel_text_width;
        const auto fits = [](const char* where, const char* text, int max_width)
        {
            check_text(where, text, cl::FontId::SMALL, max_width, 1);
        };

        for(const char* text : { cl::ui::panel_empty_square, cl::ui::panel_threats_note, cl::ui::panel_captured,
                                 cl::ui::panel_sealed, cl::ui::panel_pick_target, cl::ui::panel_cast,
                                 cl::ui::panel_escape_ready, cl::ui::panel_escape_used, cl::ui::panel_check })
        {
            fits("panel text", text, width);
        }

        char line[64];
        std::snprintf(line, sizeof(line), "%sh8", cl::ui::panel_threats);
        fits("panel threats header", line, width);
        std::snprintf(line, sizeof(line), "%src:e2f2@d1", cl::ui::panel_last);      // longest coordinate notation
        fits("panel last move", line, width);

        for(const char* side : { "White: 16", "Black: 16", "WHITE (AI) ◆", "BLACK (AI) ◆", "Arden ◆" })
        {
            fits("panel side line", side, width);
        }

        for(int c = 0; c < int(cl::Character::NARRATOR); ++c)
        {
            // Scenario opponents are shown by the first word of their name.
            const char* name = cl::character_name(cl::Character(c));
            int length = 0;

            while(name[length] && name[length] != ' ')
            {
                ++length;
            }

            std::snprintf(line, sizeof(line), "%.*s ◆", length, name);
            fits("panel opponent", line, width);
        }

        // Piece header: upper-case chess name, then the class right-aligned with at least a small gap.
        for(int t = cl::PAWN; t <= cl::KING; ++t)
        {
            const cl::PieceType type = cl::PieceType(t);
            char title[16] = {};

            for(int i = 0; cl::piece_type_name(type)[i] && i < 15; ++i)
            {
                const char c = cl::piece_type_name(type)[i];
                title[i] = c >= 'a' && c <= 'z' ? char(c - 'a' + 'A') : c;
            }

            const int header = cl::text_pixel_width(cl::FontId::SMALL, title) + cl::ui::panel_header_gap +
                               cl::text_pixel_width(cl::FontId::SMALL, cl::piece_class_name(type));
            check(header <= width, title, "panel header too wide (%d px)", header);

            // Battle caption "<class> captures <class>", centred at x = 112 left of the skip hint (x >= 190).
            for(int v = cl::PAWN; v < cl::KING; ++v)
            {
                std::snprintf(line, sizeof(line), "%s captures %s", cl::piece_class_name(type),
                              cl::piece_class_name(cl::PieceType(v)));
                check_text("battle caption", line, cl::FontId::SMALL, 156, 1);
            }
        }

        for(int a = 0; a < cl::ABILITIES_COUNT; ++a)
        {
            fits("panel ability", cl::ability_name(cl::Ability(a)), width);
        }

        fits("panel cost", "Cost 9 ◆", width - cl::ui::panel_icon_text_offset);
        fits("panel mana need", "Need 9 mana", width);
        fits("panel moves left", "25 moves left", width);

        for(int o = int(cl::Objective::CHECKMATE_WITHIN); o <= int(cl::Objective::CAPTURE_PAWNS); ++o)
        {
            const char* name = cl::objective_name(cl::Objective(o));
            fits("panel objective", name, width);
            // The status line falls back to the short form when the long one does not fit.
            std::snprintf(line, sizeof(line), "%s: 25 left", name);
            fits("status objective", line, cl::ui::status_text_width);
        }
    }

    void validate_ui_text()
    {
        for(int a = 0; a < cl::ABILITIES_COUNT; ++a)
        {
            check_text("rule card", cl::ability_rule_text(cl::Ability(a)), cl::FontId::SMALL,
                       cl::layout::card_text_width, cl::layout::card_lines);
            check_text("ability summary", cl::ability_summary(cl::Ability(a)), cl::FontId::SMALL,
                       cl::layout::screen_text_width - 24, 1);
        }

        for(int p = 0; p < cl::help_pages_count; ++p)
        {
            const cl::HelpPage& page = cl::help_page(p);
            check_text("help title", page.title, cl::FontId::BIG, cl::layout::title_text_width, 1);
            int rows = 0;

            for(int i = 0; i < page.count; ++i)
            {
                const cl::HelpLine& line = page.lines[i];
                const bool keyed = line.key[0] != 0;

                if(keyed)
                {
                    check_text("help key", line.key, cl::FontId::SMALL, cl::layout::help_key_width, 1);
                }

                const int width = keyed ? cl::layout::help_text_width :
                                          cl::layout::help_key_width + cl::layout::help_text_width;
                check_text("help text", line.text, cl::FontId::SMALL, width, cl::layout::help_lines);
                cl::TextSpan spans[8];
                rows += cl::wrap_lines(cl::FontId::SMALL, line.text, int(std::strlen(line.text)), width, spans, 8);
            }

            // The page body (from y = 36, 10 px rows, gaps between entries) must end above the hint line.
            check(36 + rows * 10 + (page.count - 1) * 3 <= 144, page.title, "help page too long (%d rows)", rows);
        }

        for(int i = 0; i < cl::achievements_count; ++i)
        {
            const cl::AchievementInfo& info = cl::achievement_info(cl::Achievement(i));
            check_text("achievement title", info.title, cl::FontId::SMALL, cl::layout::list_text_width, 1);
            check_text("achievement description", info.description, cl::FontId::SMALL,
                       cl::layout::screen_text_width - 24, 2);
        }

        for(int c = 0; c < cl::gallery_characters; ++c)
        {
            check_text("character lore", cl::character_lore(cl::Character(c)), cl::FontId::SMALL,
                       cl::layout::card_text_width, cl::layout::card_lines);

            if(c >= 2)
            {
                check_text("gallery hint", cl::gallery_unlock_hint(cl::GallerySection::CHARACTERS, c),
                           cl::FontId::SMALL, cl::layout::card_text_width, 2);
            }
        }

        for(int w = 0; w < cl::gallery_kingdoms; ++w)
        {
            check_text("kingdom lore", cl::kingdom_lore(w), cl::FontId::SMALL, cl::layout::card_text_width,
                       cl::layout::card_lines);
        }

        for(int t = 0; t < cl::gallery_music; ++t)
        {
            check_text("music title", cl::music_title(t), cl::FontId::SMALL, cl::layout::list_text_width, 1);
        }

        validate_match_panel();

        int credits_count = 0;
        const char* const* credits = cl::credits_lines(credits_count);

        for(int i = 0; i < credits_count; ++i)
        {
            const char* line = credits[i][0] == '#' ? credits[i] + 1 : credits[i];

            if(line[0])
            {
                // The roll is drawn inside a 192 px wide panel.
                check_text("credits", line, cl::FontId::SMALL, 184, 1);
            }
        }
    }
}

int main(int argc, char** argv)
{
    for(int i = 1; i < argc; ++i)
    {
        if(! std::strcmp(argv[i], "--quick"))
        {
            quick = true;
        }
    }

    std::printf("CHESS LEGENDS content validation%s\n\n", quick ? " (quick: no solver proofs)" : "");
    std::printf("Challenges (%d):\n", cl::challenges_count);

    for(int i = 0; i < cl::challenges_count; ++i)
    {
        validate_challenge(i);
    }

    std::printf("\nStory encounters (%d):\n", cl::campaign_encounters);

    for(int i = 0; i < cl::campaign_encounters; ++i)
    {
        validate_encounter(i);
    }

    std::printf("\nTutorial (%d lessons):\n", cl::lessons_count);
    validate_tutorial();
    std::printf("\nBoss gauntlet (%d):\n", cl::gauntlet_stages);
    validate_gauntlet();
    std::printf("\nStory text, worlds and characters:\n");
    validate_story_text();
    std::printf("Help, achievements and gallery text:\n");
    validate_ui_text();
    std::printf("\n%d checks, %d failures\n", checks, failures);
    return failures ? 1 : 0;
}
