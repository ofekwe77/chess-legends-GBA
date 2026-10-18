// CHESS LEGENDS: ARCANE WARS - the match scene.
#include "cl_scene_match.h"

#include "bn_blending.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_string.h"

#include "cl_ai_runner.h"
#include "cl_audio.h"
#include "cl_help.h"
#include "cl_notation.h"
#include "cl_panel_tiles.h"
#include "cl_storage.h"
#include "cl_tables.h"
#include "cl_test_commands.h"
#include "cl_text.h"
#include "cl_text_layout.h"
#include "cl_tutorial.h"

#include "bn_sprite_items_ability_icons.h"

namespace cl::gba
{

namespace
{
    constexpr int panel_x = 152;      // left edge of panel text (pixels)
    constexpr int panel_tile_x = 18;  // panel window column

    constexpr const char* theme_names[board_themes_count] = {
        "ROYAL", "VERDANT", "INFERNO", "FROST", "UMBRAL", "STORM", "NEXUS", "CONTRAST"
    };

    constexpr const char* animation_names[] = { "FULL", "FAST", "MINIMAL", "OFF" };

    constexpr int help_hint_card = ABILITIES_COUNT;   // help state showing the challenge hint

    const char* side_name(Color c)
    {
        return c == WHITE ? "WHITE" : "BLACK";
    }

    Ability ability_for(PieceType type)
    {
        switch(type)
        {
        case PAWN: return ABILITY_SHADOW_STEP;
        case KNIGHT: return ABILITY_PHASE_SHIFT;
        case BISHOP: return ABILITY_GHOST_WALK;
        case ROOK: return ABILITY_CASTLE_SWAP;
        case QUEEN: return ABILITY_ROYAL_COMMAND;
        default: return ABILITY_ROYAL_ESCAPE;
        }
    }

    const char* piece_title(PieceType type)
    {
        switch(type)
        {
        case PAWN: return "PAWN - SOLDIER";
        case KNIGHT: return "KNIGHT - CAVALRY";
        case BISHOP: return "BISHOP - MYSTIC";
        case ROOK: return "ROOK - GUARDIAN";
        case QUEEN: return "QUEEN - SORCERESS";
        case KING: return "KING - SOVEREIGN";
        default: return "";
        }
    }

    char piece_letter(PieceType type)
    {
        constexpr char letters[] = { ' ', 'P', 'N', 'B', 'R', 'Q', 'K' };
        return letters[type];
    }

    const char* objective_label(const ObjectiveSpec& spec)
    {
        switch(spec.type)
        {
        case Objective::CHECKMATE_WITHIN: return "Checkmate";
        case Objective::SURVIVE: return "Survive";
        case Objective::PROMOTE: return "Promote";
        case Objective::WIN_MATERIAL: return "Win material";
        case Objective::CAPTURE_PAWNS: return "Take all pawns";
        default: return "Checkmate";
        }
    }

    bool think_frame_trampoline(void* user)
    {
        return static_cast<MatchScene*>(user)->think_frame();
    }

    int material(const Position& pos, Color c)
    {
        int total = 0;

        for(int i = 1; i < pos.pcount[c]; ++i)
        {
            total += piece_value[type_of(pos.board[pos.plist[c][i]])];
        }

        return total;
    }

    // Squares of the pieces of `by` that attack `sq` (blockers considered).
    int attacker_squares(const Position& pos, int sq, Color by, int* out)
    {
        int count = 0;

        for(int i = 0; i < pos.pcount[by]; ++i)
        {
            const int from = pos.plist[by][i];
            const Piece piece = pos.board[from];

            if(from == sq || ! attacks_ignoring_blockers(piece, from, sq))
            {
                continue;
            }

            const PieceType type = type_of(piece);

            if(type == BISHOP || type == ROOK || type == QUEEN)
            {
                const int step = tables.step[sq - from + 119];
                int s = from + step;

                while(s != sq && pos.board[s] == EMPTY)
                {
                    s += step;
                }

                if(s != sq)
                {
                    continue;
                }
            }

            out[count++] = from;
        }

        return count;
    }

    // First word of a character name ("Rowan Thorn" -> "Rowan").
    bn::string<16> short_name(const char* name)
    {
        bn::string<16> result;

        for(const char* c = name; *c && *c != ' ' && result.size() < 15; ++c)
        {
            result.push_back(*c);
        }

        return result;
    }
}

// ------------------------------------------------------------------------------------------------ setup

MatchScene::MatchScene() :
    _g(globals()),
    _game(globals().game),
    _view(globals().setup.board_theme, (globals().setup.human_mask & 3) == 2)
{
    set_current_scene(SceneId::MATCH);
    _start_match();
}

MatchScene::~MatchScene()
{
    test_interface().match_active = 0;
    bn::blending::set_transparency_alpha(1);
}

void MatchScene::_setup_mode()
{
    MatchSetup& setup = _g.setup;
    const bool resume = setup.resume_suspended;
    _scenario = false;
    _opponent_label.clear();
    bool fen_start = false;

    switch(setup.mode)
    {
    case MatchMode::STORY:
    {
        const Encounter& e = encounter(setup.encounter);

        if(! resume)
        {
            [[maybe_unused]] const bool ok = setup_encounter(e, _game);
            CL_ASSERT(ok);
        }

        _scenario = true;
        setup.arcane = true;
        setup.human_mask = uint8_t(1 << e.player);
        setup.difficulty = e.difficulty;
        setup.personality = e.personality;
        setup.node_scale = e.node_scale;
        setup.board_theme = world(e.world).board_theme;
        setup.opponent_faction = world(e.world).faction;
        _tracker.start(e.objective, e.player, _game.position());
        _opponent_label = short_name(character_name(e.opponent));
        fen_start = e.fen != nullptr;
        break;
    }

    case MatchMode::CHALLENGE:
    {
        const Challenge& c = challenge(setup.challenge);

        if(! resume)
        {
            [[maybe_unused]] const bool ok = setup_challenge(c, _game);
            CL_ASSERT(ok);
        }

        _scenario = true;
        setup.arcane = c.arcane;
        setup.human_mask = 1;
        setup.difficulty = c.defender;
        setup.personality = Personality::BALANCED;
        setup.node_scale = 100;
        setup.opponent_faction = 0;
        _tracker.start(c.objective, WHITE, _game.position());
        fen_start = true;
        break;
    }

    case MatchMode::GAUNTLET:
    {
        // The six rulers in order, each from an opening that suits them, with rising difficulty.
        const int index = setup.gauntlet_stage;
        const GauntletStage& stage = gauntlet_stage(index);

        if(! resume)
        {
            [[maybe_unused]] const bool ok = setup_gauntlet_stage(stage, _game);
            CL_ASSERT(ok);
        }

        _scenario = true;
        setup.arcane = true;
        setup.human_mask = 1;
        setup.difficulty = stage.difficulty;
        setup.personality = stage.personality;
        setup.node_scale = stage.node_scale;
        setup.board_theme = world(index).board_theme;
        setup.opponent_faction = world(index).faction;
        _tracker.start(ObjectiveSpec{ Objective::WIN, 0, 0 }, WHITE, _game.position());
        _opponent_label = short_name(character_name(stage.boss));
        fen_start = stage.fen != nullptr;
        break;
    }

    case MatchMode::TUTORIAL:
    {
        const Lesson& l = lesson(setup.lesson);
        [[maybe_unused]] const bool ok = setup_lesson(l, _game);
        CL_ASSERT(ok);
        _scenario = l.goal == LessonGoal::OBJECTIVE;
        setup.arcane = l.arcane;
        setup.human_mask = 1;
        setup.difficulty = l.defender;
        setup.personality = Personality::BALANCED;
        setup.node_scale = 100;
        setup.opponent_faction = 0;

        if(_scenario)
        {
            _tracker.start(l.objective, WHITE, _game.position());
        }

        fen_start = l.fen != nullptr;
        break;
    }

    default:
        if(! resume)
        {
            _game.start(setup.arcane ? RuleSet::arcane_rules() : RuleSet::classic());
        }

        setup.node_scale = 100;
        break;
    }

    _tutorial = setup.mode == MatchMode::TUTORIAL;

    if(_g.profile.settings.high_contrast)
    {
        setup.board_theme = board_themes_count - 1;
    }

    _player = (setup.human_mask & 1) ? WHITE : BLACK;
    _standard_start = ! fen_start;

    switch(setup.mode)
    {
    case MatchMode::STORY:
    {
        const Encounter& e = encounter(setup.encounter);
        play_music(e.type == EncounterType::BOSS ? Music::BOSS : world_music(e.world));
        break;
    }

    case MatchMode::CHALLENGE: play_music(Music::UMBRAL); break;
    case MatchMode::GAUNTLET: play_music(Music::BOSS); break;
    case MatchMode::TUTORIAL: play_music(Music::VERDANT); break;
    default: play_music(Music::BATTLE); break;
    }
}

MatchContext MatchScene::_context() const
{
    MatchContext context;
    const MatchSetup& setup = _g.setup;

    switch(setup.mode)
    {
    case MatchMode::CLASSIC: context.kind = MatchKind::CLASSIC; break;
    case MatchMode::STORY: context.kind = MatchKind::STORY; break;
    case MatchMode::CHALLENGE: context.kind = MatchKind::CHALLENGE; break;
    case MatchMode::GAUNTLET: context.kind = MatchKind::GAUNTLET; break;
    case MatchMode::TUTORIAL: context.kind = MatchKind::TUTORIAL; break;
    default: context.kind = MatchKind::ARCANE; break;
    }

    context.human_mask = setup.human_mask;
    context.difficulty = setup.difficulty;
    context.boss = setup.mode == MatchMode::GAUNTLET ||
                   (setup.mode == MatchMode::STORY && encounter(setup.encounter).type == EncounterType::BOSS);
    return context;
}

void MatchScene::_start_match()
{
    MatchSetup& setup = _g.setup;
    _setup_mode();
    setup.result_ready = false;
    _result_recorded = false;
    _objective_success = false;
    _medal = 0;
    _last_move = Move();
    _info = setup.mode == MatchMode::TUTORIAL ? InfoView::TASK : InfoView::PIECE;

    for(auto& side : _captured)
    {
        for(int& count : side)
        {
            count = 0;
        }
    }

    _recorder.start(_context());

    if(setup.resume_suspended)
    {
        const SuspendedMatch& suspended = _g.suspended;
        _tracker.unpack(suspended.objective_state);
        _recorder.unpack(suspended.recorder);
        _match_seed = suspended.match_seed;
        setup.resume_suspended = false;     // a rematch / retry starts fresh
    }
    else
    {
        _match_seed = _g.seed ^ (_g.frame_counter * 2654435761u);
        _g.seed = _g.seed * 1664525u + 1013904223u;
    }

    _view.set_theme(setup.board_theme);
    _view.set_flipped(setup.human_mask == 2);
    _view.set_coordinates_visible(_g.profile.settings.show_coordinates);
    _view.set_dark_faction(setup.opponent_faction);
    _view.set_last_move(NO_SQUARE, NO_SQUARE);
    _view.rebuild_pieces(_game.position());
    _panel.clear(0);
    _draw_panel_frame();
    _view.set_cursor(_human_to_move() && _game.position().side == BLACK ? 0x64 : 0x14);
    _view.commit();
    _begin_turn();

    // A lesson opens with its instruction pages (not when it restarts after a missed task).
    if(_tutorial && _lesson_page == 0)
    {
        _open_lesson();
    }
}

bool MatchScene::_human_to_move() const
{
    return (_g.setup.human_mask >> _game.position().side) & 1;
}

SceneId MatchScene::_exit_scene() const
{
    switch(_g.setup.mode)
    {
    case MatchMode::STORY: return SceneId::STORY_MAP;
    case MatchMode::CHALLENGE: return SceneId::CHALLENGES;
    case MatchMode::GAUNTLET: return SceneId::GAUNTLET;
    case MatchMode::TUTORIAL: return SceneId::TUTORIAL;
    default: return SceneId::MAIN_MENU;
    }
}

// ------------------------------------------------------------------------------------------------ turn flow

const char* MatchScene::_idle_hint() const
{
    return _game.position().rules.arcane ? "Ⓐ Select  Ⓡ View  SELECT Rules  START Menu" :
                                           "Ⓐ Select  Ⓡ View  START Menu";
}

void MatchScene::_begin_turn()
{
    if(_tutorial && _lesson_retry && ! _game.over())
    {
        // The learner's action did not do what the lesson asked: back to the lesson position.
        _lesson_retry = false;
        _start_match();
        _flash("Not quite. Try again.");
        return;
    }

    _selected = NO_SQUARE;
    _command_queen = NO_SQUARE;
    _command_ally = NO_SQUARE;
    _view.set_selected(NO_SQUARE);
    _view.clear_markers();
    _threat_markers = false;

    const Position& pos = _game.position();
    _view.set_check(! _game.over() && pos.in_check() ? pos.king_square(pos.side) : NO_SQUARE);
    _view.commit();
    _panel_dirty = true;
    _set_status();

    if(_game.over())
    {
        _open_game_over();
        return;
    }

    _game.legal_actions(_legal);

    if(_human_to_move())
    {
        if(_g.setup.human_mask == 3)
        {
            _open_handoff();
            return;
        }

        _state = State::HUMAN;
        _view.set_cursor_visible(true);
        _set_hint(_idle_hint());

        if(_info == InfoView::THREATS)
        {
            _show_threat_markers();
        }
    }
    else
    {
        _state = State::AI;
        _view.set_cursor_visible(false);
        _set_hint("The opponent is thinking...");
    }
}

void MatchScene::_after_action(const ActionInfo& info)
{
    _recorder.on_action(info, _game);
    grant_achievements(_recorder.earned());

    if(_tutorial)
    {
        _lesson_check(info);
    }

    if(! _scenario)
    {
        return;
    }

    _tracker.on_action(info, _game);
    const ObjectiveStatus status = _tracker.status(_game);

    if(status == ObjectiveStatus::ONGOING)
    {
        return;
    }

    const bool success = status == ObjectiveStatus::SUCCESS;
    const GameResult won = _player == WHITE ? GameResult::WHITE_WINS : GameResult::BLACK_WINS;

    // A running game ends with the objective; a draw that meets the objective (survival, a material lead that
    // left a dead position) becomes an objective victory.
    if(! _game.over() || (success && _game.result() != won))
    {
        _game.finish_objective(success, _player);
    }
}

void MatchScene::_lesson_check(const ActionInfo& info)
{
    const Lesson& l = lesson(_g.setup.lesson);

    switch(l.goal)
    {
    case LessonGoal::MOVE:
    case LessonGoal::CAPTURE:
    case LessonGoal::CAST:
        if(info.mover == _player && ! _game.over())
        {
            if(lesson_action_completes(l, info))
            {
                _game.finish_objective(true, _player);
            }
            else
            {
                _lesson_retry = true;
            }
        }
        break;

    case LessonGoal::REACH_MANA:
        // Mana is regenerated at the start of the learner's turn, right after the computer's reply.
        if(info.mover != _player && ! _game.over() && _game.position().mana[_player] >= l.value)
        {
            _game.finish_objective(true, _player);
        }
        break;

    default:
        break;
    }
}

void MatchScene::_open_lesson()
{
    _state_before_pause = _state;
    _state = State::LESSON;
    _lesson_page = 0;
    _overlay_open = false;
    _view.set_cursor_visible(false);
}

void MatchScene::_update_lesson()
{
    const Lesson& l = lesson(_g.setup.lesson);
    int pages = 0;

    while(pages < 3 && l.pages[pages])
    {
        ++pages;
    }

    bool redraw = ! _overlay_open;

    if(bn::keypad::a_pressed())
    {
        if(++_lesson_page >= pages)
        {
            _close_overlay();
            _state = _state_before_pause;
            _view.set_cursor_visible(_state != State::AI);
            return;
        }

        redraw = true;
    }
    else if(bn::keypad::b_pressed() || bn::keypad::start_pressed())
    {
        _lesson_page = pages;
        _close_overlay();
        _state = _state_before_pause;
        _view.set_cursor_visible(_state != State::AI);
        return;
    }

    if(! redraw)
    {
        return;
    }

    _overlay_text.clear();
    _panel_text.clear();
    _icons.clear();
    _panel.window(1, 3, 28, 14);
    bn::string<40> header;
    bn::ostringstream header_stream(header);
    header_stream << "LESSON " << (_g.setup.lesson + 1) << " OF " << lessons_count;
    print(_overlay_text, 120, 32, header, TextColor::CYAN, Align::CENTER);
    print_big(_overlay_text, 120, 44, l.title, TitleColor::GOLD, Align::CENTER);
    const char* text = l.pages[_lesson_page];
    TextSpan spans[4];
    const int lines = wrap_lines(FontId::SMALL, text, bn::string_view(text).size(), layout::card_text_width, spans, 4);

    for(int i = 0; i < lines; ++i)
    {
        print(_overlay_text, 120, 70 + i * 11, bn::string_view(text + spans[i].start, spans[i].length),
              TextColor::WHITE, Align::CENTER);
    }

    bn::string<32> footer;
    bn::ostringstream footer_stream(footer);
    footer_stream << "◆ " << (_lesson_page + 1) << " / " << pages << "   " << (_lesson_page + 1 < pages ? "Ⓐ Next" :
                                                                                                         "Ⓐ Start");
    print(_overlay_text, 120, 118, footer, TextColor::GOLD, Align::CENTER);
    _overlay_open = true;
}

void MatchScene::_record_result()
{
    if(_result_recorded)
    {
        return;
    }

    _result_recorded = true;
    Profile& profile = _g.profile;
    const GameResult result = _game.result();
    bool success;

    if(_scenario)
    {
        success = _tracker.status(_game) == ObjectiveStatus::SUCCESS;
    }
    else
    {
        success = _g.setup.human_mask != 3 && ((result == GameResult::WHITE_WINS && _player == WHITE) ||
                                               (result == GameResult::BLACK_WINS && _player == BLACK));
    }

    _objective_success = success;
    _g.setup.last_success = success;
    _g.setup.result_ready = true;
    _recorder.on_finish(_game, success);
    _g.toasts |= _recorder.commit(profile);

    switch(_g.setup.mode)
    {
    case MatchMode::STORY:
        if(success)
        {
            record_encounter_cleared(profile.progress, _g.setup.encounter, _tracker.player_moves());
        }
        break;

    case MatchMode::CHALLENGE:
        if(success)
        {
            const Challenge& c = challenge(_g.setup.challenge);
            _medal = challenge_medal(c, _tracker.player_moves());
            record_challenge_medal(profile.progress, _g.setup.challenge, _medal);
        }
        break;

    case MatchMode::TUTORIAL:
        if(success)
        {
            profile.progress.tutorial_lessons |= uint16_t(1u << _g.setup.lesson);
        }
        break;

    case MatchMode::GAUNTLET:
        if(success)
        {
            const int defeated = _g.setup.gauntlet_stage + 1;

            if(defeated > profile.progress.gauntlet_best)
            {
                profile.progress.gauntlet_best = uint8_t(defeated);
            }

            if(defeated == worlds_count)
            {
                profile.progress.gauntlet_cleared = true;
            }
        }
        break;

    default:
        break;
    }

    save_profile_now();
}

void MatchScene::_suspend_and_quit()
{
    // Every field is assigned below (no reset through a 2.6 KB temporary on the IWRAM stack).
    SuspendedMatch& s = _g.suspended;
    capture_game(_game, s);
    const MatchSetup& setup = _g.setup;
    s.mode = uint8_t(setup.mode);
    s.human_mask = setup.human_mask;
    s.difficulty = uint8_t(setup.difficulty);
    s.personality = uint8_t(setup.personality);
    s.board_theme = setup.board_theme;
    s.opponent_faction = setup.opponent_faction;
    s.encounter = setup.encounter;
    s.challenge = setup.challenge;
    s.gauntlet_stage = setup.gauntlet_stage;
    s.match_seed = _match_seed;
    s.objective_state = _tracker.pack();
    _recorder.pack(s.recorder);

    if(! save_suspended_match(s))
    {
        _close_overlay();
        _open_pause();
        _flash("Save failed. Try again.");
        return;
    }

    test_interface().suspend_present = 1;
    save_profile_now();
    _next_scene = SceneId::MAIN_MENU;
}

// ------------------------------------------------------------------------------------------------ update

bn::optional<SceneId> MatchScene::update()
{
    _input.update();

    switch(_state)
    {
    case State::HUMAN:
    case State::SELECTED:
    case State::MAGIC:
    case State::COMMAND:
        _update_human();
        break;

    case State::PROMOTION:
        _update_promotion();
        break;

    case State::ANIMATING:
        _update_animation();
        break;

    case State::AI:
        _update_ai();
        break;

    case State::HANDOFF:
        _update_handoff();
        break;

    case State::PAUSE:
        _update_pause();
        break;

    case State::SETTINGS:
        _update_settings();
        break;

    case State::HELP:
        _update_help();
        break;

    case State::CONFIRM:
        _update_confirm();
        break;

    case State::GAME_OVER:
        _update_game_over();
        break;

    case State::LESSON:
        _update_lesson();
        break;

    default:
        break;
    }

    if(_message_frames > 0 && --_message_frames == 0)
    {
        _set_status();
    }

    if(_panel_dirty && ! _overlay_open)
    {
        _refresh_panel();
    }

    _toast.update();
    _view.update();
    _view.commit();
    _panel.commit();
    _publish_state();

    bn::optional<SceneId> next = _next_scene;
    _next_scene.reset();
    return next;
}

// ------------------------------------------------------------------------------------------------ human input

Ability MatchScene::_ability_of(int sq) const
{
    return ability_for(type_of(_game.position().board[sq]));
}

void MatchScene::_move_cursor()
{
    int sq = _view.cursor();
    int file = file_of(sq);
    int rank = rank_of(sq);
    const int dir = _view.flipped() ? -1 : 1;

    if(_input.left())
    {
        file -= dir;
    }
    else if(_input.right())
    {
        file += dir;
    }

    if(_input.up())
    {
        rank += dir;
    }
    else if(_input.down())
    {
        rank -= dir;
    }

    file = file < 0 ? 0 : (file > 7 ? 7 : file);
    rank = rank < 0 ? 0 : (rank > 7 ? 7 : rank);
    const int target = make_square(file, rank);

    if(target != sq)
    {
        play_sound(Sound::CURSOR);
        _view.set_cursor(target);
        _panel_dirty = true;

        if(_threat_markers)
        {
            _show_threat_markers();
        }
    }
}

void MatchScene::_update_human()
{
    // Automated tests may post an action; it goes through the same legal-action path as a player's move.
    if(const uint32_t posted = take_test_move())
    {
        for(int i = 0; i < _legal.count; ++i)
        {
            if(_legal.moves[i].data() == posted)
            {
                _execute(_legal.moves[i]);
                return;
            }
        }
    }

    if(bn::keypad::start_pressed())
    {
        _open_pause();
        return;
    }

    _move_cursor();
    const int cursor = _view.cursor();
    const Position& pos = _game.position();
    const Piece piece = pos.board[cursor];
    const bool own_piece = piece != EMPTY && color_of(piece) == pos.side;

    if(bn::keypad::r_pressed())
    {
        _info = InfoView((int(_info) + 1) % 4);

        if(_info == InfoView::TASK && ! _tutorial)
        {
            _info = InfoView::PIECE;
        }

        _panel_dirty = true;

        if(_state == State::HUMAN)
        {
            if(_info == InfoView::THREATS)
            {
                _show_threat_markers();
            }
            else if(_threat_markers)
            {
                _view.clear_markers();
                _threat_markers = false;
            }
        }

        return;
    }

    if(bn::keypad::select_pressed())
    {
        const Piece focus = pos.board[_selected != NO_SQUARE ? _selected : cursor];
        _open_help(pos.rules.arcane && focus != EMPTY ? int(ability_for(type_of(focus))) : -1);
        return;
    }

    if(bn::keypad::b_pressed())
    {
        if(_state == State::COMMAND)
        {
            _command_ally = NO_SQUARE;
            _state = State::MAGIC;
            _show_magic_targets();
        }
        else if(_state != State::HUMAN)
        {
            _deselect();
        }

        return;
    }

    if(bn::keypad::l_pressed() && pos.rules.arcane)
    {
        if(_state == State::SELECTED)
        {
            _enter_magic();
        }
        else if(_state == State::MAGIC || _state == State::COMMAND)
        {
            _state = State::SELECTED;
            _command_ally = NO_SQUARE;
            _show_normal_targets();
        }
        else if(_state == State::HUMAN && own_piece)
        {
            _select(cursor);
            _enter_magic();
        }

        return;
    }

    if(bn::keypad::a_pressed())
    {
        if(_state == State::HUMAN)
        {
            if(own_piece)
            {
                _select(cursor);
            }
            else
            {
                _flash(piece == EMPTY ? "Choose one of your pieces." : "That piece is not yours.");
            }

            return;
        }

        const Marker marker = _view.marker(cursor);

        if(marker != Marker::NONE && marker != Marker::TARGET && marker != Marker::HINT)
        {
            _try_destination(cursor);
            return;
        }

        if(_state == State::MAGIC && marker == Marker::TARGET)
        {
            // Royal Command: the commanded ally was chosen.
            _command_ally = cursor;
            _state = State::COMMAND;
            _show_command_destinations();
            return;
        }

        if(own_piece && cursor != _selected)
        {
            _select(cursor);
            return;
        }

        if(cursor == _selected)
        {
            _deselect();
            return;
        }

        _flash("That square is not reachable.");
    }
}

void MatchScene::_select(int sq)
{
    if(_tutorial)
    {
        const Lesson& l = lesson(_g.setup.lesson);

        if(l.goal == LessonGoal::SELECT_SQUARE && sq == l.from && ! _game.over())
        {
            _game.finish_objective(true, _player);
            _begin_turn();
            return;
        }
    }

    _threat_markers = false;
    _selected = sq;
    _command_queen = NO_SQUARE;
    _command_ally = NO_SQUARE;
    _view.set_selected(sq);
    _state = State::SELECTED;
    play_sound(Sound::PIECE_SELECT);
    _show_normal_targets();
}

void MatchScene::_deselect()
{
    _selected = NO_SQUARE;
    _command_queen = NO_SQUARE;
    _command_ally = NO_SQUARE;
    _view.set_selected(NO_SQUARE);
    _view.clear_markers();
    _state = State::HUMAN;
    _panel_dirty = true;
    _set_hint(_idle_hint());

    if(_info == InfoView::THREATS)
    {
        _show_threat_markers();
    }
}

void MatchScene::_show_threat_markers()
{
    // Marks the opponent's pieces that attack the hovered square (inspection only; never selectable).
    _view.clear_markers();
    const Position& pos = _game.position();
    int squares[16];
    const int count = attacker_squares(pos, _view.cursor(), opposite(pos.side), squares);

    for(int i = 0; i < count; ++i)
    {
        _view.set_marker(squares[i], Marker::HINT);
    }

    _threat_markers = true;
}

void MatchScene::_show_normal_targets()
{
    _view.clear_markers();
    int count = 0;

    for(int i = 0; i < _legal.count; ++i)
    {
        const Move m = _legal.moves[i];

        if(m.is_magic() || m.from() != _selected)
        {
            continue;
        }

        _view.set_marker(m.to(), m.is_capture() ? Marker::CAPTURE : Marker::MOVE);
        ++count;
    }

    _panel_dirty = true;

    if(_game.position().rules.arcane)
    {
        _set_hint(count ? "Ⓐ Move  Ⓛ Magic  Ⓑ Cancel" : "No moves.  Ⓛ Magic  Ⓑ Cancel");
    }
    else
    {
        _set_hint(count ? "Ⓐ Move  Ⓑ Cancel" : "No legal moves.  Ⓑ Cancel");
    }
}

void MatchScene::_enter_magic()
{
    const Position& pos = _game.position();
    const Ability ability = _ability_of(_selected);

    if(! (pos.rules.ability_mask[pos.side] & (1 << ability)))
    {
        _flash("That ability is sealed here.");
        return;
    }

    if(pos.mana[pos.side] < pos.rules.cost[ability])
    {
        bn::string<48> text;
        bn::ostringstream stream(text);
        stream << ability_name(ability) << " needs " << int(pos.rules.cost[ability]) << " mana.";
        _flash(text.c_str());
        return;
    }

    if(ability == ABILITY_ROYAL_ESCAPE && ! pos.escapes[pos.side])
    {
        _flash("Royal Escape was already used.");
        return;
    }

    if(ability == ABILITY_ROYAL_ESCAPE && ! pos.in_check())
    {
        _flash("Royal Escape needs check.");
        return;
    }

    _state = State::MAGIC;

    if(ability == ABILITY_ROYAL_COMMAND)
    {
        _command_queen = _selected;
    }

    _show_magic_targets();
}

void MatchScene::_show_magic_targets()
{
    _view.clear_markers();
    const Ability ability = _ability_of(_selected);
    const MoveKind kind = ability_move_kind(ability);
    int count = 0;

    for(int i = 0; i < _legal.count; ++i)
    {
        const Move m = _legal.moves[i];

        if(m.kind() != kind)
        {
            continue;
        }

        if(kind == MK_ROYAL_COMMAND)
        {
            if(m.aux() == _selected)
            {
                _view.set_marker(m.from(), Marker::TARGET);
                ++count;
            }
        }
        else if(m.from() == _selected)
        {
            const bool special = kind == MK_CASTLE_SWAP || kind == MK_ROYAL_ESCAPE;
            _view.set_marker(m.to(), special ? Marker::SPECIAL : Marker::MAGIC);
            ++count;
        }
    }

    _panel_dirty = true;

    if(! count)
    {
        _flash("No legal target for this spell.");
        _state = State::SELECTED;
        _show_normal_targets();
        return;
    }

    _set_hint(kind == MK_ROYAL_COMMAND ? "Ⓐ Choose ally  Ⓛ Normal  Ⓑ Back" : "Ⓐ Cast  Ⓛ Normal moves  Ⓑ Back");
}

void MatchScene::_show_command_destinations()
{
    _view.clear_markers();
    _view.set_marker(_command_ally, Marker::TARGET);

    for(int i = 0; i < _legal.count; ++i)
    {
        const Move m = _legal.moves[i];

        if(m.kind() == MK_ROYAL_COMMAND && m.aux() == _command_queen && m.from() == _command_ally)
        {
            _view.set_marker(m.to(), Marker::MAGIC);
        }
    }

    _panel_dirty = true;
    _set_hint("Ⓐ Command  Ⓑ Choose another ally");
}

void MatchScene::_try_destination(int sq)
{
    _promotion_count = 0;
    Move chosen;

    for(int i = 0; i < _legal.count; ++i)
    {
        const Move m = _legal.moves[i];
        bool matches = false;

        switch(_state)
        {
        case State::SELECTED:
            matches = ! m.is_magic() && m.from() == _selected && m.to() == sq;
            break;

        case State::MAGIC:
            matches = m.is_magic() && m.kind() == ability_move_kind(_ability_of(_selected)) && m.from() == _selected &&
                      m.to() == sq;
            break;

        case State::COMMAND:
            matches = m.kind() == MK_ROYAL_COMMAND && m.aux() == _command_queen && m.from() == _command_ally &&
                      m.to() == sq;
            break;

        default:
            break;
        }

        if(! matches)
        {
            continue;
        }

        if(m.kind() == MK_PROMOTION)
        {
            if(_promotion_count < 4)
            {
                _promotions[_promotion_count++] = m;
            }
        }
        else
        {
            chosen = m;
        }
    }

    if(_promotion_count)
    {
        _open_promotion(sq);
        return;
    }

    if(chosen.is_null())
    {
        _flash("Illegal action.");
        return;
    }

    _execute(chosen);
}

// ------------------------------------------------------------------------------------------------ actions

int MatchScene::_animation_length(const Move& move) const
{
    int base;

    switch(animation_mode())
    {
    case AnimationMode::FULL: base = 14; break;
    case AnimationMode::FAST: base = 9; break;
    case AnimationMode::MINIMAL: base = 5; break;
    default: return 0;
    }

    if(move.is_magic())
    {
        base += base / 2;
    }

    return base;
}

void MatchScene::_execute(Move move)
{
    ActionInfo info;

    // The single state transition of the match.
    if(! _game.apply(move, &info))
    {
        _flash("Illegal action.");
        return;
    }

    test_interface().moves_applied += 1;
    test_interface().last_move = move.data();
    _last_move = move;

    // Sound: the game result first, then check, then what the action was.
    if(_game.over() && _game.reason() == ResultReason::CHECKMATE)
    {
        play_sound(Sound::CHECKMATE);
    }
    else if(_game.over() && _game.reason() == ResultReason::STALEMATE)
    {
        play_sound(Sound::STALEMATE);
    }
    else if(info.check)
    {
        play_sound(Sound::CHECK);
    }
    else if(move.is_magic())
    {
        play_sound(spell_sound(move.ability()));
    }
    else if(move.kind() == MK_PROMOTION)
    {
        play_sound(Sound::PROMOTION);
    }
    else
    {
        play_sound(info.captured != EMPTY ? Sound::CAPTURE : Sound::PIECE_MOVE);
    }

    if(info.captured != EMPTY)
    {
        ++_captured[info.mover][type_of(info.captured)];
        test_interface().captures += 1;
    }

    if(move.is_magic())
    {
        test_interface().spells += 1;
    }

    _after_action(info);

    _view.clear_markers();
    _threat_markers = false;
    _view.set_selected(NO_SQUARE);
    _view.set_check(NO_SQUARE);
    _view.set_cursor_visible(false);

    _anim = Animation();
    _anim.move = move;
    _anim.info = info;
    _anim.length = _animation_length(move);
    _anim.mover_sq = move.from();
    _anim.mover_from = _view.square_center(move.from());
    _anim.mover_to = _view.square_center(move.to());
    _anim.victim_sq = info.captured_sq;

    switch(move.kind())
    {
    case MK_CASTLE_KING:
        _anim.partner_sq = move.from() + 3;
        _anim.partner_from = _view.square_center(move.from() + 3);
        _anim.partner_to = _view.square_center(move.from() + 1);
        break;

    case MK_CASTLE_QUEEN:
        _anim.partner_sq = move.from() - 4;
        _anim.partner_from = _view.square_center(move.from() - 4);
        _anim.partner_to = _view.square_center(move.from() - 1);
        break;

    case MK_CASTLE_SWAP:
        _anim.partner_sq = move.to();
        _anim.partner_from = _anim.mover_to;
        _anim.partner_to = _anim.mover_from;
        break;

    default:
        break;
    }

    if(bn::sprite_ptr* mover = _view.sprite_at(_anim.mover_sq))
    {
        mover->set_z_order(-5);

        if(move.kind() == MK_GHOST_WALK)
        {
            mover->set_blending_enabled(true);
            bn::blending::set_transparency_alpha(0.5);
        }
    }

    _state = State::ANIMATING;
    _set_hint("");

    if(_anim.length == 0)
    {
        _finish_animation();
    }
}

void MatchScene::_update_animation()
{
    if(bn::keypad::a_pressed() || bn::keypad::b_pressed())
    {
        // Skipping only shortens the presentation: the game state was already applied.
        _finish_animation();
        return;
    }

    ++_anim.frame;
    const bn::fixed t = bn::fixed(_anim.frame) / _anim.length;
    const MoveKind kind = _anim.move.kind();
    bn::sprite_ptr* mover = _view.sprite_at(_anim.mover_sq);

    if(mover)
    {
        bn::fixed_point position = _anim.mover_from + (_anim.mover_to - _anim.mover_from) * t;

        if(kind == MK_ROYAL_ESCAPE)
        {
            // leap in an arc
            position.set_y(position.y() - t * (1 - t) * 40);
        }
        else if(kind == MK_PHASE_SHIFT)
        {
            // vanish, then reappear at the destination
            position = t < bn::fixed(0.5) ? _anim.mover_from : _anim.mover_to;
            mover->set_visible((_anim.frame & 2) || t > bn::fixed(0.8) || t < bn::fixed(0.2));
        }

        mover->set_position(position);
    }

    if(_anim.partner_sq != NO_SQUARE)
    {
        if(bn::sprite_ptr* partner = _view.sprite_at(_anim.partner_sq))
        {
            if(partner != mover)
            {
                partner->set_position(_anim.partner_from + (_anim.partner_to - _anim.partner_from) * t);
            }
        }
    }

    if(_anim.victim_sq != NO_SQUARE && _anim.victim_sq != _anim.mover_sq)
    {
        if(bn::sprite_ptr* victim = _view.sprite_at(_anim.victim_sq))
        {
            if(victim != mover)
            {
                victim->set_visible(t < bn::fixed(0.6) && (_anim.frame & 1));
            }
        }
    }

    if(_anim.frame >= _anim.length)
    {
        _finish_animation();
    }
}

void MatchScene::_finish_animation()
{
    bn::blending::set_transparency_alpha(1);
    _view.rebuild_pieces(_game.position());
    const Move move = _anim.move;
    _view.set_last_move(move.from(), move.to());
    _view.set_cursor(move.to());
    _begin_turn();
}

// ------------------------------------------------------------------------------------------------ AI

AiSettings MatchScene::_ai_settings() const
{
    AiSettings settings;
    settings.difficulty = _g.setup.difficulty;
    settings.personality = _g.setup.personality;
    settings.node_scale_percent = _g.setup.node_scale;
    settings.seed = _match_seed ^ (uint32_t(_game.position().side) * 0x9E3779B9u);
    return settings;
}

bool MatchScene::think_frame()
{
    ++_think_phase;

    if((_think_phase & 7) == 0)
    {
        _panel_dirty = true;
    }

    if(_panel_dirty)
    {
        _refresh_panel();
    }

    if(bn::keypad::start_pressed())
    {
        _pause_requested = true;
    }

    test_interface().ai_thinking = 1;
    _toast.update();
    _view.update();
    _view.commit();
    _panel.commit();
    _publish_state();
    frame_end();
    return _pause_requested;
}

void MatchScene::_update_ai()
{
    _pause_requested = false;
    const AiSettings settings = _ai_settings();
    ThinkReport report = ai_think(_game, settings, _standard_start, think_frame_trampoline, this);
    test_interface().ai_thinking = 0;
    test_interface().ai_nodes = report.decision.nodes;
    test_interface().ai_depth = uint32_t(report.decision.depth);
    test_interface().ai_frames = uint32_t(report.frames);
    test_interface().ai_moves += 1;

    if(report.cancelled)
    {
        // Thinking interrupted by the pause menu: the search restarts (deterministically) on resume.
        _open_pause();
        return;
    }

    _ai_last_score = report.decision.score;

    if(Ai::should_claim_draw(_game, settings, report.decision.score) && _game.claim_draw())
    {
        _begin_turn();
        return;
    }

    if(report.decision.move.is_null())
    {
        // No move (should not happen in a running game): resign safely instead of continuing in an invalid state.
        _game.resign(_game.position().side);
        _begin_turn();
        return;
    }

    _execute(report.decision.move);
}

// ------------------------------------------------------------------------------------------------ overlays

void MatchScene::_close_overlay()
{
    _overlay_text.clear();
    _overlay_open = false;
    _panel.clear(0);
    _draw_panel_frame();
    _panel_dirty = true;
}

void MatchScene::_resume_from_pause()
{
    _close_overlay();
    _state = _state_before_pause;
    _view.set_cursor_visible(_state != State::AI);

    if(_settings_changed)
    {
        _settings_changed = false;
        save_profile_now();     // settings changed in the menu are kept
    }
}

void MatchScene::_draw_menu(const char* title, const char* const* items, int count, int selected, int y)
{
    _overlay_text.clear();
    _panel_text.clear();
    _icons.clear();
    const int height_tiles = 4 + (count * 13 + 7) / 8;
    const int top_tile = y / 8;
    _panel.window(4, top_tile, 22, height_tiles);
    print_big(_overlay_text, 120, y + 6, title, TitleColor::GOLD, Align::CENTER);

    for(int i = 0; i < count; ++i)
    {
        const bool current = i == selected;
        print(_overlay_text, 120, y + 28 + i * 13, items[i], current ? TextColor::GOLD : TextColor::WHITE,
              Align::CENTER);

        if(current)
        {
            print(_overlay_text, 44, y + 28 + i * 13, "→", TextColor::GOLD, Align::LEFT);
        }
    }

    _overlay_open = true;
    _menu_count = count;
}

namespace
{
    constexpr int pause_max_items = 9;

    enum PauseItem : uint8_t
    {
        PAUSE_RESUME,
        PAUSE_SETTINGS,
        PAUSE_HELP,
        PAUSE_HINT,
        PAUSE_CLAIM,
        PAUSE_OFFER,
        PAUSE_SAVE_QUIT,
        PAUSE_RESIGN,
        PAUSE_QUIT
    };
}

void MatchScene::_open_pause()
{
    play_sound(Sound::MENU_SELECT);
    if(_state != State::PAUSE && _state != State::SETTINGS && _state != State::HELP && _state != State::CONFIRM)
    {
        _state_before_pause = _state;
    }

    _state = State::PAUSE;
    _menu_index = 0;
    _overlay_open = false;
    _view.set_cursor_visible(false);
}

void MatchScene::_update_pause()
{
    PauseItem items[pause_max_items];
    const char* labels[pause_max_items];
    int count = 0;
    const MatchMode mode = _g.setup.mode;

    items[count] = PAUSE_RESUME;
    labels[count++] = "RESUME";
    items[count] = PAUSE_SETTINGS;
    labels[count++] = "SETTINGS";
    items[count] = PAUSE_HELP;
    labels[count++] = "CONTROLS & RULES";

    if(mode == MatchMode::CHALLENGE)
    {
        items[count] = PAUSE_HINT;
        labels[count++] = "HINT";
    }

    const bool human_turn = _state_before_pause != State::AI;

    if(human_turn && (_game.can_claim_threefold() || _game.can_claim_fifty()))
    {
        items[count] = PAUSE_CLAIM;
        labels[count++] = "CLAIM DRAW";
    }

    if(_g.setup.human_mask == 3)
    {
        items[count] = PAUSE_OFFER;
        labels[count++] = "OFFER DRAW";
    }

    if(mode != MatchMode::TUTORIAL)
    {
        items[count] = PAUSE_SAVE_QUIT;
        labels[count++] = "SAVE & QUIT";
    }

    if(mode == MatchMode::TUTORIAL)
    {
        items[count] = PAUSE_RESIGN;
        labels[count++] = "RESTART LESSON";
    }
    else
    {
        items[count] = PAUSE_RESIGN;
        labels[count++] = "RESIGN";
    }

    items[count] = PAUSE_QUIT;
    labels[count++] = mode == MatchMode::STORY ? "QUIT TO MAP" : (mode == MatchMode::TUTORIAL ? "QUIT TO LESSONS" :
                                                                                               "QUIT");

    if(_menu_index >= count)
    {
        _menu_index = count - 1;
    }

    bool redraw = ! _overlay_open;

    if(_input.up())
    {
        play_sound(Sound::MENU_MOVE);
        _menu_index = (_menu_index + count - 1) % count;
        redraw = true;
    }
    else if(_input.down())
    {
        play_sound(Sound::MENU_MOVE);
        _menu_index = (_menu_index + 1) % count;
        redraw = true;
    }

    const PauseItem item = items[_menu_index];

    if(bn::keypad::a_pressed())
    {
        play_sound(Sound::MENU_SELECT);
    }

    if(bn::keypad::a_pressed())
    {
        switch(item)
        {
        case PAUSE_RESUME:
            _resume_from_pause();
            return;

        case PAUSE_SETTINGS:
            _open_settings();
            return;

        case PAUSE_HELP:
            _open_help(-1);
            return;

        case PAUSE_HINT:
            _open_help(help_hint_card);
            return;

        case PAUSE_CLAIM:
            _close_overlay();
            _game.claim_draw();
            _begin_turn();
            return;

        case PAUSE_OFFER:
            _open_confirm(Confirm::OFFER_DRAW);
            return;

        case PAUSE_SAVE_QUIT:
            _suspend_and_quit();
            return;

        case PAUSE_RESIGN:
            if(_tutorial)
            {
                _close_overlay();
                _lesson_page = 0;
                _start_match();
                return;
            }

            _open_confirm(Confirm::RESIGN);
            return;

        case PAUSE_QUIT:
            _open_confirm(Confirm::QUIT);
            return;

        default:
            break;
        }
    }
    else if(bn::keypad::b_pressed() || bn::keypad::start_pressed())
    {
        _resume_from_pause();
        return;
    }

    if(redraw)
    {
        _draw_menu("PAUSED", labels, count, _menu_index, 16);
    }
}

void MatchScene::_open_settings()
{
    _state = State::SETTINGS;
    _menu_index = 0;
    _overlay_open = false;
}

void MatchScene::_update_settings()
{
    SaveSettings& settings = _g.profile.settings;
    constexpr int count = 5;
    bool redraw = ! _overlay_open;

    if(_input.up())
    {
        _menu_index = (_menu_index + count - 1) % count;
        redraw = true;
    }
    else if(_input.down())
    {
        _menu_index = (_menu_index + 1) % count;
        redraw = true;
    }

    const bool change = bn::keypad::a_pressed() || _input.left() || _input.right();
    const int delta = _input.left() ? -1 : 1;

    if(change && _menu_index < count - 1)
    {
        _settings_changed = true;
    }

    if(change)
    {
        switch(_menu_index)
        {
        case 0:
        {
            // Only unlocked boards; the choice is kept for quick matches.
            int theme = _g.setup.board_theme;

            for(int i = 0; i < board_themes_count; ++i)
            {
                theme = (theme + board_themes_count + delta) % board_themes_count;

                if(board_theme_unlocked(_g.profile.progress, theme))
                {
                    break;
                }
            }

            _g.setup.board_theme = uint8_t(theme);

            if(_g.setup.mode == MatchMode::CLASSIC || _g.setup.mode == MatchMode::ARCANE)
            {
                settings.board_theme = uint8_t(theme);
            }

            _view.set_theme(theme);
            break;
        }

        case 1:
            settings.animations = uint8_t((settings.animations + 4 + delta) % 4);
            break;

        case 2:
            settings.music_on = ! settings.music_on;
            apply_audio_settings();
            break;

        case 3:
            settings.sfx_on = ! settings.sfx_on;
            play_sound(Sound::MENU_SELECT);
            break;

        default:
            if(bn::keypad::a_pressed())
            {
                _open_pause();
                return;
            }
            break;
        }

        redraw = true;
    }

    if(bn::keypad::b_pressed())
    {
        _open_pause();
        return;
    }

    if(redraw)
    {
        bn::string<24> board;
        bn::string<24> anim;
        bn::string<24> music;
        bn::string<24> sound;
        bn::ostringstream board_stream(board);
        board_stream << "BOARD: " << theme_names[_g.setup.board_theme % board_themes_count];
        bn::ostringstream anim_stream(anim);
        anim_stream << "ANIMATIONS: " << animation_names[settings.animations & 3];
        bn::ostringstream music_stream(music);
        music_stream << "MUSIC: " << (settings.music_on ? "ON" : "OFF");
        bn::ostringstream sound_stream(sound);
        sound_stream << "SOUND: " << (settings.sfx_on ? "ON" : "OFF");
        const char* labels[count] = { board.c_str(), anim.c_str(), music.c_str(), sound.c_str(), "BACK" };
        _draw_menu("SETTINGS", labels, count, _menu_index, 24);
    }
}

void MatchScene::_open_help(int ability)
{
    _help_from_pause = _state == State::PAUSE;

    if(! _help_from_pause)
    {
        _state_before_pause = _state;
    }

    _state = State::HELP;
    _help_ability = ability;
    _help_page = 0;
    _overlay_open = false;
    _view.set_cursor_visible(false);
}

void MatchScene::_update_help()
{
    const bool card = _help_ability >= 0;
    bool redraw = ! _overlay_open;

    if(! card && (_input.right() || bn::keypad::a_pressed()))
    {
        if(_help_page + 1 < help_pages_count)
        {
            ++_help_page;
            redraw = true;
        }
        else if(bn::keypad::a_pressed())
        {
            _help_page = help_pages_count;   // past the last page: close
        }
    }
    else if(! card && _input.left() && _help_page > 0)
    {
        --_help_page;
        redraw = true;
    }

    const bool close = bn::keypad::b_pressed() || bn::keypad::select_pressed() || (card && bn::keypad::a_pressed()) ||
                       _help_page >= help_pages_count;

    if(close)
    {
        // Opened from the pause menu: back to it; opened with SELECT: back to the board.
        if(_help_from_pause)
        {
            _open_pause();
        }
        else
        {
            _resume_from_pause();
        }

        return;
    }

    if(! redraw)
    {
        return;
    }

    _overlay_text.clear();
    _panel_text.clear();
    _icons.clear();
    _panel.window(1, 2, 28, 17);

    if(_help_ability == help_hint_card)
    {
        const Challenge& c = challenge(_g.setup.challenge);
        print_big(_overlay_text, 120, 26, "HINT", TitleColor::ARCANE, Align::CENTER);
        print(_overlay_text, 120, 46, c.title, TextColor::CYAN, Align::CENTER);
        TextSpan spans[layout::hint_lines];
        const int lines = wrap_lines(FontId::SMALL, c.hint, bn::string_view(c.hint).size(), layout::hint_text_width,
                                     spans, layout::hint_lines);

        for(int i = 0; i < lines; ++i)
        {
            print(_overlay_text, 120, 66 + i * 11, bn::string_view(c.hint + spans[i].start, spans[i].length),
                  TextColor::WHITE, Align::CENTER);
        }

        print(_overlay_text, 120, 136, "Ⓐ / Ⓑ Close", TextColor::GRAY, Align::CENTER);
    }
    else if(card)
    {
        const Ability ability = Ability(_help_ability);
        const Position& pos = _game.position();
        print_big(_overlay_text, 120, 26, ability_name(ability), TitleColor::ARCANE, Align::CENTER);
        bn::string<40> status;
        bn::ostringstream stream(status);
        stream << "Cost " << int(pos.rules.cost[ability]) << " mana";

        if(! (pos.rules.ability_mask[pos.side] & (1 << ability)))
        {
            stream << "  -  sealed here";
        }

        print(_overlay_text, 120, 46, status, TextColor::CYAN, Align::CENTER);
        const char* text = ability_rule_text(ability);
        TextSpan spans[layout::card_lines];
        const int lines = wrap_lines(FontId::SMALL, text, bn::string_view(text).size(), layout::card_text_width,
                                     spans, layout::card_lines);

        for(int i = 0; i < lines; ++i)
        {
            print(_overlay_text, 16, 62 + i * 11, bn::string_view(text + spans[i].start, spans[i].length));
        }

        print(_overlay_text, 120, 136, "Ⓐ / Ⓑ Close", TextColor::GRAY, Align::CENTER);
    }
    else
    {
        const HelpPage& page = help_page(_help_page);
        print_big(_overlay_text, 120, 22, page.title, TitleColor::GOLD, Align::CENTER);
        int y = 40;

        for(int i = 0; i < page.count; ++i)
        {
            const HelpLine& line = page.lines[i];
            const bool keyed = line.key[0] != 0;
            const int x = keyed ? 16 + layout::help_key_width : 16;
            const int width = keyed ? layout::help_text_width : layout::help_key_width + layout::help_text_width;

            if(keyed)
            {
                print(_overlay_text, 16, y, line.key, TextColor::GOLD);
            }

            TextSpan spans[layout::help_lines];
            const int lines = wrap_lines(FontId::SMALL, line.text, bn::string_view(line.text).size(), width,
                                         spans, layout::help_lines);

            for(int l = 0; l < lines; ++l)
            {
                print(_overlay_text, x, y, bn::string_view(line.text + spans[l].start, spans[l].length));
                y += 10;
            }

            y += 3;
        }

        bn::string<32> footer;
        bn::ostringstream footer_stream(footer);
        footer_stream << "◆ " << (_help_page + 1) << " / " << help_pages_count << "   Ⓑ Back";
        print(_overlay_text, 120, 140, footer, TextColor::GRAY, Align::CENTER);
    }

    _overlay_open = true;
}

void MatchScene::_open_confirm(Confirm confirm)
{
    _confirm = confirm;
    _state = State::CONFIRM;
    _menu_index = 1;
    _overlay_open = false;
}

void MatchScene::_update_confirm()
{
    bool redraw = ! _overlay_open;

    if(_input.up() || _input.down())
    {
        _menu_index ^= 1;
        redraw = true;
    }

    if(bn::keypad::a_pressed())
    {
        if(_menu_index == 0)
        {
            _close_overlay();

            switch(_confirm)
            {
            case Confirm::RESIGN:
            {
                // In hot-seat the player to move resigns; against the AI, the human side resigns.
                const Color loser = _g.setup.human_mask == 3 ? _game.position().side : _player;
                _game.resign(loser);
                _begin_turn();
                return;
            }

            case Confirm::QUIT:
                save_profile_now();
                _next_scene = _exit_scene();
                return;

            case Confirm::OFFER_DRAW:
                _game.agree_draw();
                _begin_turn();
                return;

            default:
                break;
            }
        }

        _open_pause();
        return;
    }

    if(bn::keypad::b_pressed())
    {
        _open_pause();
        return;
    }

    if(redraw)
    {
        constexpr const char* yes_no[] = { "YES", "NO" };
        const char* title = _confirm == Confirm::RESIGN ? "RESIGN?" :
                            _confirm == Confirm::QUIT ? "QUIT MATCH?" : "ACCEPT DRAW?";
        _draw_menu(title, yes_no, 2, _menu_index, 48);

        if(_confirm == Confirm::QUIT)
        {
            print(_overlay_text, 120, 104, "This match will be lost.", TextColor::GRAY, Align::CENTER);
        }
    }
}

void MatchScene::_open_promotion(int to)
{
    (void) to;
    _state = State::PROMOTION;
    _menu_index = 0;
    _overlay_open = false;
}

void MatchScene::_update_promotion()
{
    constexpr const char* labels[] = { "QUEEN", "ROOK", "BISHOP", "KNIGHT" };
    constexpr PieceType order[] = { QUEEN, ROOK, BISHOP, KNIGHT };
    bool redraw = ! _overlay_open;

    if(_input.up())
    {
        _menu_index = (_menu_index + 3) % 4;
        redraw = true;
    }
    else if(_input.down())
    {
        _menu_index = (_menu_index + 1) % 4;
        redraw = true;
    }

    if(bn::keypad::a_pressed())
    {
        const PieceType choice = order[_menu_index];

        for(int i = 0; i < _promotion_count; ++i)
        {
            if(_promotions[i].promotion() == choice)
            {
                const Move move = _promotions[i];
                _close_overlay();
                _execute(move);
                return;
            }
        }
    }
    else if(bn::keypad::b_pressed())
    {
        _close_overlay();
        _state = State::SELECTED;
        _show_normal_targets();
        return;
    }

    if(redraw)
    {
        _draw_menu("PROMOTE TO", labels, 4, _menu_index, 32);
    }
}

void MatchScene::_open_handoff()
{
    _state = State::HANDOFF;
    _overlay_open = false;
    _view.set_cursor_visible(false);

    // Each player sees their own army at the bottom.
    const bool flip = _game.position().side == BLACK;

    if(_view.flipped() != flip)
    {
        _view.set_flipped(flip);
        _view.rebuild_pieces(_game.position());
        _panel_dirty = true;
    }
}

void MatchScene::_update_handoff()
{
    if(! _overlay_open)
    {
        _overlay_text.clear();
        _panel_text.clear();
        _icons.clear();
        _panel.window(5, 7, 20, 6);
        bn::string<24> title;
        bn::ostringstream stream(title);
        stream << side_name(_game.position().side) << "'S TURN";
        print_big(_overlay_text, 120, 64, title, TitleColor::SILVER, Align::CENTER);
        print(_overlay_text, 120, 84, "Press Ⓐ when ready", TextColor::WHITE, Align::CENTER);
        _overlay_open = true;
    }

    if(bn::keypad::a_pressed())
    {
        _close_overlay();
        _state = State::HUMAN;
        _view.set_cursor_visible(true);
        _view.set_cursor(_game.position().king_square(_game.position().side));
        _set_hint(_idle_hint());
    }
}

// ------------------------------------------------------------------------------------------------ game over

void MatchScene::_open_game_over()
{
    _record_result();

    if(_g.setup.human_mask == 3)
    {
        play_music(_game.result() == GameResult::DRAW ? Music::DEFEAT : Music::VICTORY, false);
    }
    else if(_game.result() == GameResult::DRAW && ! _scenario)
    {
        stop_music();
    }
    else
    {
        play_music(_objective_success ? Music::VICTORY : Music::DEFEAT, false);
    }
    _state = State::GAME_OVER;
    _menu_index = 0;
    _overlay_open = false;
    _view.set_cursor_visible(false);

    _end_count = 0;

    switch(_g.setup.mode)
    {
    case MatchMode::STORY:
        if(! _objective_success)
        {
            _end_options[_end_count++] = EndOption::RETRY;
        }

        _end_options[_end_count++] = EndOption::CONTINUE;
        break;

    case MatchMode::CHALLENGE:
        if(_objective_success && _g.setup.challenge + 1 < challenges_count)
        {
            _end_options[_end_count++] = EndOption::NEXT_CHALLENGE;
        }

        _end_options[_end_count++] = EndOption::RETRY;
        _end_options[_end_count++] = EndOption::CHALLENGE_LIST;
        break;

    case MatchMode::GAUNTLET:
        _end_options[_end_count++] = EndOption::GAUNTLET_NEXT;
        break;

    case MatchMode::TUTORIAL:
        if(_objective_success && _g.setup.lesson + 1 < lessons_count)
        {
            _end_options[_end_count++] = EndOption::NEXT_LESSON;
        }

        if(! _objective_success)
        {
            _end_options[_end_count++] = EndOption::RETRY;
        }

        _end_options[_end_count++] = EndOption::LESSON_LIST;
        break;

    default:
        _end_options[_end_count++] = EndOption::REMATCH;
        _end_options[_end_count++] = EndOption::MAIN_MENU;
        break;
    }
}

void MatchScene::_update_game_over()
{
    bool redraw = ! _overlay_open;

    if(_end_count > 1)
    {
        if(_input.up())
        {
            _menu_index = (_menu_index + _end_count - 1) % _end_count;
            redraw = true;
        }
        else if(_input.down())
        {
            _menu_index = (_menu_index + 1) % _end_count;
            redraw = true;
        }
    }

    if(bn::keypad::a_pressed())
    {
        const EndOption option = _end_options[_menu_index];

        switch(option)
        {
        case EndOption::REMATCH:
        case EndOption::RETRY:
            _close_overlay();
            _lesson_page = 0;
            _start_match();
            return;

        case EndOption::NEXT_LESSON:
            _close_overlay();
            _g.setup.lesson = int8_t(_g.setup.lesson + 1);
            _lesson_page = 0;
            _start_match();
            return;

        case EndOption::LESSON_LIST:
            _next_scene = SceneId::TUTORIAL;
            return;

        case EndOption::NEXT_CHALLENGE:
            _close_overlay();
            _g.setup.challenge = int8_t(_g.setup.challenge + 1);
            _start_match();
            return;

        case EndOption::CONTINUE:
            _g.story.step = _objective_success ? StoryStep::VICTORY : StoryStep::DEFEAT;
            _g.story.encounter = _g.setup.encounter;
            _g.story.world = int8_t(encounter(_g.setup.encounter).world);
            _next_scene = SceneId::STORY_DIALOGUE;
            return;

        case EndOption::CHALLENGE_LIST:
            _next_scene = SceneId::CHALLENGES;
            return;

        case EndOption::GAUNTLET_NEXT:
            _next_scene = SceneId::GAUNTLET;
            return;

        default:
            _next_scene = SceneId::MAIN_MENU;
            return;
        }
    }

    if(redraw)
    {
        _draw_game_over();
    }
}

void MatchScene::_draw_game_over()
{
    _overlay_text.clear();
    _panel_text.clear();
    _icons.clear();
    _panel.window(3, 3, 24, 15);
    const GameResult result = _game.result();
    const MatchMode mode = _g.setup.mode;
    const char* title;
    TitleColor color = TitleColor::GOLD;
    const bool scenario_goal = _scenario && _tracker.spec().type != Objective::WIN;

    if(mode == MatchMode::TUTORIAL)
    {
        title = _objective_success ? "LESSON COMPLETE" : "TRY AGAIN";
        color = _objective_success ? TitleColor::GOLD : TitleColor::RUBY;
    }
    else if(mode == MatchMode::CHALLENGE)
    {
        title = _objective_success ? "SOLVED!" : "NOT SOLVED";
        color = _objective_success ? TitleColor::GOLD : TitleColor::RUBY;
    }
    else if(scenario_goal)
    {
        title = _objective_success ? "OBJECTIVE COMPLETE" : "OBJECTIVE FAILED";
        color = _objective_success ? TitleColor::GOLD : TitleColor::RUBY;
    }
    else if(result == GameResult::DRAW)
    {
        title = "DRAW";
        color = TitleColor::SILVER;
    }
    else
    {
        const Color winner = result == GameResult::WHITE_WINS ? WHITE : BLACK;

        if(_g.setup.human_mask == 3)
        {
            title = winner == WHITE ? "WHITE WINS" : "BLACK WINS";
        }
        else if((_g.setup.human_mask >> winner) & 1)
        {
            title = "VICTORY!";
        }
        else
        {
            title = "DEFEAT";
            color = TitleColor::RUBY;
        }
    }

    print_big(_overlay_text, 120, 34, title, color, Align::CENTER);

    if(mode == MatchMode::TUTORIAL)
    {
        const Lesson& l = lesson(_g.setup.lesson);
        const char* text = _objective_success ? l.success : l.task;
        TextSpan spans[4];
        const int lines = wrap_lines(FontId::SMALL, text, bn::string_view(text).size(), 184, spans, 4);

        for(int i = 0; i < lines; ++i)
        {
            print(_overlay_text, 120, 54 + i * 10, bn::string_view(text + spans[i].start, spans[i].length),
                  TextColor::WHITE, Align::CENTER);
        }

        _draw_end_options();
        return;
    }

    print(_overlay_text, 120, 56, result_reason_name(_game.reason()), TextColor::WHITE, Align::CENTER);

    bn::string<40> detail;
    bn::ostringstream stream(detail);

    if(_scenario)
    {
        stream << "Your moves: " << _tracker.player_moves();

        if(mode == MatchMode::CHALLENGE)
        {
            stream << "  (par " << int(challenge(_g.setup.challenge).par) << ")";
        }
    }
    else
    {
        stream << "Moves: " << _game.position().fullmove;
    }

    print(_overlay_text, 120, 68, detail, TextColor::GRAY, Align::CENTER);

    if(mode == MatchMode::CHALLENGE && _objective_success)
    {
        constexpr const char* medals[] = { "", "★ BRONZE MEDAL", "★★ SILVER MEDAL", "★★★ GOLD MEDAL" };
        print(_overlay_text, 120, 80, medals[_medal & 3], TextColor::GOLD, Align::CENTER);
    }
    else if(mode == MatchMode::GAUNTLET)
    {
        bn::string<40> line;
        bn::ostringstream line_stream(line);
        line_stream << "Rulers defeated: " << (_g.setup.gauntlet_stage + (_objective_success ? 1 : 0)) << " / "
                    << worlds_count;
        print(_overlay_text, 120, 80, line, TextColor::GOLD, Align::CENTER);
    }
    else if(_g.save_failed)
    {
        print(_overlay_text, 120, 80, "Progress could not be saved!", TextColor::RED, Align::CENTER);
    }

    _draw_end_options();
}

void MatchScene::_draw_end_options()
{
    for(int i = 0; i < _end_count; ++i)
    {
        const char* label;

        switch(_end_options[i])
        {
        case EndOption::REMATCH: label = "REMATCH"; break;
        case EndOption::NEXT_LESSON: label = "NEXT LESSON"; break;
        case EndOption::LESSON_LIST: label = "LESSONS"; break;
        case EndOption::CONTINUE: label = "CONTINUE"; break;
        case EndOption::RETRY: label = "RETRY"; break;
        case EndOption::NEXT_CHALLENGE: label = "NEXT CHALLENGE"; break;
        case EndOption::CHALLENGE_LIST: label = "CHALLENGES"; break;
        case EndOption::GAUNTLET_NEXT: label = _objective_success && _g.setup.gauntlet_stage + 1 < worlds_count ?
                                                   "NEXT RULER" : "RESULTS"; break;
        default: label = "MAIN MENU"; break;
        }

        const int y = 96 + i * 13;
        print(_overlay_text, 120, y, label, i == _menu_index ? TextColor::GOLD : TextColor::WHITE, Align::CENTER);

        if(i == _menu_index)
        {
            print(_overlay_text, 56, y, "→", TextColor::GOLD);
        }
    }

    _overlay_open = true;
    _menu_count = _end_count;
}

// ------------------------------------------------------------------------------------------------ panel

bn::string<20> MatchScene::_side_label(Color side) const
{
    bn::string<20> name;
    bn::ostringstream stream(name);
    const bool human = (_g.setup.human_mask >> side) & 1;
    const MatchMode mode = _g.setup.mode;

    if(mode == MatchMode::STORY || mode == MatchMode::GAUNTLET)
    {
        stream << (human ? "Arden" : _opponent_label.c_str());
    }
    else
    {
        stream << side_name(side) << (human || _g.setup.human_mask == 3 ? "" : " (AI)");
    }

    return name;
}

void MatchScene::_draw_panel_frame()
{
    _panel.window(panel_tile_x, 0, 12, 20);
    _panel.divider(panel_tile_x + 1, 6, 10);
    _panel.divider(panel_tile_x + 1, 13, 10);
}

void MatchScene::_refresh_panel()
{
    _panel_dirty = false;
    _panel_text.clear();
    _icons.clear();

    const Position& pos = _game.position();
    const bool arcane = pos.rules.arcane;
    const Color top_side = _view.flipped() ? WHITE : BLACK;
    const Color bottom_side = opposite(top_side);

    auto side_block = [&](Color side, int y, int gem_row)
    {
        bn::string<24> name = _side_label(side);
        const bool to_move = pos.side == side && ! _game.over();
        const bool checked = to_move && pos.in_check();

        if(to_move)
        {
            name.append(" ◆");
        }

        print(_panel_text, panel_x, y, name, checked ? TextColor::RED : (to_move ? TextColor::GOLD : TextColor::WHITE));

        if(arcane)
        {
            _panel.gems(panel_tile_x + 1, gem_row, pos.mana[side], pos.rules.max_mana);
            bn::string<16> mana;
            bn::ostringstream mana_stream(mana);
            mana_stream << pos.mana[side] << "/" << int(pos.rules.max_mana);
            print(_panel_text, panel_x + 56, gem_row * 8, mana, TextColor::CYAN);

            if(checked)
            {
                print(_panel_text, panel_x, gem_row * 8 + 10, "CHECK!", TextColor::RED);
            }
            else
            {
                print(_panel_text, panel_x, gem_row * 8 + 10, pos.escapes[side] ? "Escape ready" : "Escape used",
                      pos.escapes[side] ? TextColor::GRAY : TextColor::RED);
            }
        }
        else
        {
            _panel.fill(panel_tile_x + 1, gem_row, 10, 1, panel_tiles::win_fill);
            const int diff = material(pos, side) - material(pos, opposite(side));
            bn::string<16> text;
            bn::ostringstream mat(text);

            if(diff > 0)
            {
                mat << "+" << diff / 100;
            }
            else
            {
                mat << "=";
            }

            print(_panel_text, panel_x, gem_row * 8, text, TextColor::GRAY);

            if(checked)
            {
                print(_panel_text, panel_x, gem_row * 8 + 10, "CHECK!", TextColor::RED);
            }
        }
    };

    side_block(top_side, 8, 2);
    side_block(bottom_side, 120, 16);

    // Middle block: the AI's thinking state, or the selected information view.
    if(_state == State::AI)
    {
        constexpr const char* dots[] = { "Thinking", "Thinking.", "Thinking..", "Thinking..." };
        print(_panel_text, panel_x, 60, dots[(_think_phase >> 3) & 3], TextColor::CYAN);
        print(_panel_text, panel_x, 74, difficulty_name(_g.setup.difficulty), TextColor::GRAY);
        return;
    }

    switch(_info)
    {
    case InfoView::THREATS:
        _panel_threats(pos);
        break;

    case InfoView::GAME:
        _panel_game(pos);
        break;

    case InfoView::TASK:
        _panel_task();
        break;

    default:
        _panel_piece(pos);
        break;
    }
}

void MatchScene::_panel_task()
{
    const Lesson& l = lesson(_g.setup.lesson);
    bn::string<24> header;
    bn::ostringstream stream(header);
    stream << "LESSON " << (_g.setup.lesson + 1);
    print(_panel_text, panel_x, 52, header, TextColor::CYAN);
    TextSpan spans[layout::panel_lines];
    const int lines = wrap_lines(FontId::SMALL, l.task, bn::string_view(l.task).size(), layout::panel_text_width,
                                 spans, layout::panel_lines);

    for(int i = 0; i < lines; ++i)
    {
        print(_panel_text, panel_x, 62 + i * 9, bn::string_view(l.task + spans[i].start, spans[i].length),
              TextColor::GOLD);
    }
}

void MatchScene::_panel_piece(const Position& pos)
{
    const int focus = _selected != NO_SQUARE ? _selected : _view.cursor();
    const Piece piece = pos.board[focus];
    const bool arcane = pos.rules.arcane;

    if(piece == EMPTY)
    {
        char square[3];
        square_name(focus, square);
        print(_panel_text, panel_x, 56, "PIECE  ◇ Ⓡ", TextColor::GRAY);
        print(_panel_text, panel_x, 70, square, TextColor::GRAY);
        return;
    }

    print(_panel_text, panel_x, 56, piece_title(type_of(piece)), TextColor::WHITE);

    if(! arcane)
    {
        return;
    }

    const Ability ability = ability_for(type_of(piece));
    const Color owner = color_of(piece);
    const bool own = owner == pos.side;
    const bool affordable = pos.mana[owner] >= pos.rules.cost[ability];
    const bool sealed = ! (pos.rules.ability_mask[owner] & (1 << ability));
    bool usable = own && affordable && ! sealed;

    if(ability == ABILITY_ROYAL_ESCAPE)
    {
        usable = usable && pos.escapes[owner] && pos.in_check();
    }

    bn::sprite_ptr icon = bn::sprite_items::ability_icons.create_sprite(panel_x + 8 - 120, 72 + 8 - 80,
                                                                         int(ability) + (usable ? 0 : 6));
    icon.set_bg_priority(0);
    _icons.push_back(bn::move(icon));
    print(_panel_text, panel_x + 20, 70, ability_name(ability), usable ? TextColor::CYAN : TextColor::GRAY);

    bn::string<20> cost;
    bn::ostringstream stream(cost);
    stream << "Cost " << int(pos.rules.cost[ability]) << " ◆";
    print(_panel_text, panel_x + 20, 80, cost, affordable ? TextColor::WHITE : TextColor::RED);

    if(sealed)
    {
        print(_panel_text, panel_x, 94, "Sealed here", TextColor::RED);
    }
    else if(_state == State::MAGIC || _state == State::COMMAND)
    {
        print(_panel_text, panel_x, 94, "Choose a target", TextColor::GOLD);
    }
    else if(own && ! affordable)
    {
        bn::string<20> need;
        bn::ostringstream need_stream(need);
        need_stream << "Need " << int(pos.rules.cost[ability]) << " mana";
        print(_panel_text, panel_x, 94, need, TextColor::RED);
    }
    else if(own && usable && _state == State::SELECTED)
    {
        print(_panel_text, panel_x, 94, "Ⓛ to cast", TextColor::GOLD);
    }
}

void MatchScene::_panel_threats(const Position& pos)
{
    const int sq = _view.cursor();
    char square[3];
    square_name(sq, square);
    bn::string<24> header;
    bn::ostringstream header_stream(header);
    header_stream << "THREATS  " << square;
    print(_panel_text, panel_x, 56, header, TextColor::CYAN);
    int squares[16];

    for(int c = 0; c < 2; ++c)
    {
        const Color side = Color(c);
        const int count = attacker_squares(pos, sq, side, squares);
        bn::string<24> line;
        bn::ostringstream line_stream(line);
        line_stream << side_name(side) << ": " << count << (count == 1 ? " attacker" : " attackers");
        print(_panel_text, panel_x, 70 + c * 10, line, count ? TextColor::WHITE : TextColor::GRAY);
    }

    print(_panel_text, panel_x, 94, "Enemy attackers marked", TextColor::GRAY);
}

void MatchScene::_panel_game(const Position& pos)
{
    print(_panel_text, panel_x, 56, "GAME", TextColor::CYAN);

    for(int c = 0; c < 2; ++c)
    {
        bn::string<24> line;
        bn::ostringstream stream(line);
        stream << (c == WHITE ? "W took: " : "B took: ");
        int shown = 0;

        for(int type = QUEEN; type >= PAWN; --type)
        {
            for(int n = 0; n < _captured[c][type] && shown < 8; ++n, ++shown)
            {
                stream << piece_letter(PieceType(type));
            }
        }

        if(! shown)
        {
            stream << "-";
        }

        print(_panel_text, panel_x, 66 + c * 10, line, TextColor::WHITE);
    }

    if(_scenario && _tracker.spec().limit)
    {
        bn::string<24> goal;
        bn::ostringstream goal_stream(goal);
        goal_stream << objective_label(_tracker.spec()) << ": " << _tracker.moves_left() << " left";
        print(_panel_text, panel_x, 88, goal, TextColor::GOLD);
    }
    else if(! _last_move.is_null())
    {
        (void) pos;
        char coordinate[16];
        move_to_coordinate(_last_move, coordinate, int(sizeof(coordinate)));
        bn::string<24> last;
        bn::ostringstream last_stream(last);
        last_stream << "Last: " << coordinate;
        print(_panel_text, panel_x, 88, last, TextColor::GRAY);
    }
}

void MatchScene::_set_status()
{
    _status_text.clear();

    if(_message_frames > 0)
    {
        // Long flash texts (challenge hints) wrap over the top line and the board edge.
        TextSpan spans[2];
        const int lines = wrap_lines(FontId::SMALL, _message.c_str(), _message.size(), 136, spans, 2);

        for(int i = 0; i < lines; ++i)
        {
            print(_status_text, 4, i * 8, bn::string_view(_message.c_str() + spans[i].start, spans[i].length),
                  TextColor::RED);
        }

        return;
    }

    const Position& pos = _game.position();

    if(_game.over())
    {
        print(_status_text, 4, 0, result_reason_name(_game.reason()), TextColor::GOLD);
        return;
    }

    bn::string<40> text;
    bn::ostringstream stream(text);

    if(_scenario && _tracker.spec().limit)
    {
        const int left = _tracker.moves_left();
        stream << objective_label(_tracker.spec()) << ": " << left << (left == 1 ? " move left" : " moves left");
        print(_status_text, 4, 0, text, TextColor::GOLD);
        return;
    }

    stream << "Move " << pos.fullmove << "  " << side_name(pos.side) << " to play";
    print(_status_text, 4, 0, text, TextColor::WHITE);
}

void MatchScene::_set_hint(const char* text)
{
    _hint_text.clear();

    if(text[0])
    {
        print(_hint_text, 4, 152, text, TextColor::GRAY);
    }
}

void MatchScene::_flash(const char* text)
{
    play_sound(Sound::INVALID);
    _message = text;
    _message_frames = 120;
    _set_status();
}

void MatchScene::_publish_state()
{
    cl_test_interface& ti = test_interface();
    const Position& pos = _game.position();
    ti.match_active = 1;
    ti.mode = uint32_t(_g.setup.mode);
    ti.rules_arcane = pos.rules.arcane;
    ti.human_mask = _g.setup.human_mask;
    ti.difficulty = uint32_t(_g.setup.difficulty);
    ti.side_to_move = pos.side;
    ti.mana_white = pos.mana[WHITE];
    ti.mana_black = pos.mana[BLACK];
    ti.escapes_white = pos.escapes[WHITE];
    ti.escapes_black = pos.escapes[BLACK];
    ti.result = uint32_t(_game.result());
    ti.reason = uint32_t(_game.reason());
    ti.plies = pos.ply;
    ti.halfmove = pos.halfmove;
    ti.in_check = pos.in_check();
    ti.hash_lo = uint32_t(pos.hash);
    ti.hash_hi = uint32_t(pos.hash >> 32);
    ti.cursor_file = uint32_t(file_of(_view.cursor()));
    ti.cursor_rank = uint32_t(rank_of(_view.cursor()));
    ti.selected_square = uint32_t(_selected);
    ti.ui_state = uint32_t(_state);
    ti.board_theme = _g.setup.board_theme;
    ti.flipped = _view.flipped();
    ti.menu_index = uint32_t(_menu_index);
    ti.menu_count = uint32_t(_menu_count);
    ti.animating = _state == State::ANIMATING;
    ti.promotion_menu = _state == State::PROMOTION;
    ti.pause_menu = _state == State::PAUSE;
    ti.encounter = uint32_t(int32_t(_g.setup.encounter));
    ti.challenge = uint32_t(int32_t(_g.setup.challenge));
    ti.gauntlet_stage = uint32_t(int32_t(_g.setup.gauntlet_stage));
    ti.moves_left = uint32_t(_scenario ? _tracker.moves_left() : -1);
    ti.objective_status = uint32_t(_scenario ? int(_tracker.status(_game)) : 0);
    ti.anim_mode = uint32_t(animation_mode());
    ti.achievements_lo = uint32_t(_g.profile.progress.achievements);
    ti.achievements_hi = uint32_t(_g.profile.progress.achievements >> 32);
}

}
