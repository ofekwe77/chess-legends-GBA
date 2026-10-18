// CHESS LEGENDS: ARCANE WARS - the match scene.
#include "cl_scene_match.h"

#include "bn_blending.h"
#include "bn_core.h"
#include "bn_keypad.h"
#include "bn_math.h"
#include "bn_string.h"

#include "cl_ai_runner.h"
#include "cl_notation.h"
#include "cl_panel_tiles.h"
#include "cl_text.h"

#include "bn_sprite_items_ability_icons.h"

namespace cl::gba
{

namespace
{
    constexpr int panel_x = 152;      // left edge of panel text (pixels)
    constexpr int panel_tile_x = 18;  // panel window column

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
}

void MatchScene::_start_match()
{
    const MatchSetup& setup = _g.setup;
    RuleSet rules = setup.arcane ? RuleSet::arcane_rules() : RuleSet::classic();

    if(! setup.resume_suspended)
    {
        _game.start(rules);
    }

    _standard_start = true;
    _match_seed = _g.seed ^ (_g.frame_counter * 2654435761u);
    _g.seed = _g.seed * 1664525u + 1013904223u;
    _view.set_dark_faction(setup.opponent_faction);
    _view.rebuild_pieces(_game.position());
    _draw_panel_frame();
    _view.set_cursor(_human_to_move() && _game.position().side == BLACK ? 0x64 : 0x14);
    _view.commit();
    _begin_turn();
}

bool MatchScene::_human_to_move() const
{
    return (_g.setup.human_mask >> _game.position().side) & 1;
}

// ------------------------------------------------------------------------------------------------ turn flow

void MatchScene::_begin_turn()
{
    _selected = NO_SQUARE;
    _command_queen = NO_SQUARE;
    _command_ally = NO_SQUARE;
    _view.set_selected(NO_SQUARE);
    _view.clear_markers();

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
        _set_hint(pos.rules.arcane ? "Ⓐ Select  Ⓡ Info  START Menu" : "Ⓐ Select  START Menu");
    }
    else
    {
        _state = State::AI;
        _view.set_cursor_visible(false);
        _set_hint("The opponent is thinking...");
    }
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

    case State::CONFIRM:
        _update_confirm();
        break;

    case State::GAME_OVER:
        _update_game_over();
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
        _view.set_cursor(target);
        _panel_dirty = true;
    }
}

void MatchScene::_update_human()
{
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
    _selected = sq;
    _command_queen = NO_SQUARE;
    _command_ally = NO_SQUARE;
    _view.set_selected(sq);
    _state = State::SELECTED;
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
    _set_hint(_game.position().rules.arcane ? "Ⓐ Select  Ⓡ Info  START Menu" : "Ⓐ Select  START Menu");
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
        bn::string<40> text;
        bn::ostringstream stream(text);
        stream << ability_name(ability) << " needs " << int(pos.rules.cost[ability]) << " mana.";
        _flash(text.c_str());
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
            bool special = kind == MK_CASTLE_SWAP || kind == MK_ROYAL_ESCAPE;
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

    if(kind == MK_ROYAL_COMMAND)
    {
        _set_hint("Ⓐ Choose ally  Ⓛ Normal  Ⓑ Back");
    }
    else
    {
        _set_hint("Ⓐ Cast  Ⓛ Normal moves  Ⓑ Back");
    }
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

    switch(_g.settings.animations)
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

    if(! _game.apply(move, &info))
    {
        _flash("Illegal action.");
        return;
    }

    test_interface().moves_applied += 1;
    test_interface().last_move = move.data();

    if(info.captured != EMPTY)
    {
        ++_captured[info.mover][type_of(info.captured)];
        test_interface().captures += 1;
    }

    if(move.is_magic())
    {
        ++_spells_cast;
        test_interface().spells += 1;
    }

    _view.clear_markers();
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

void MatchScene::_draw_menu(const char* title, const char* const* items, int count, int selected, int y)
{
    _overlay_text.clear();
    const int height_tiles = 4 + count * 2;
    const int top_tile = y / 8;
    _panel.window(5, top_tile, 20, height_tiles);
    print_big(_overlay_text, 120, y + 6, title, TitleColor::GOLD, Align::CENTER);

    for(int i = 0; i < count; ++i)
    {
        const bool current = i == selected;
        print(_overlay_text, 120, y + 26 + i * 16, items[i], current ? TextColor::GOLD : TextColor::WHITE,
              Align::CENTER);

        if(current)
        {
            print(_overlay_text, 60, y + 26 + i * 16, "→", TextColor::GOLD, Align::LEFT);
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
        PAUSE_CONTROLS,
        PAUSE_THEME,
        PAUSE_ANIMATIONS,
        PAUSE_CLAIM,
        PAUSE_OFFER,
        PAUSE_RESIGN,
        PAUSE_QUIT
    };
}

void MatchScene::_open_pause()
{
    if(_state != State::PAUSE)
    {
        _state_before_pause = _state;
    }

    _state = State::PAUSE;
    _menu_index = 0;
    _view.set_cursor_visible(false);
    _update_pause();
}

void MatchScene::_update_pause()
{
    PauseItem items[pause_max_items];
    const char* labels[pause_max_items];
    int count = 0;
    static bn::string<24> theme_label;
    static bn::string<24> anim_label;

    items[count] = PAUSE_RESUME;
    labels[count++] = "RESUME";
    items[count] = PAUSE_CONTROLS;
    labels[count++] = "CONTROLS";

    {
        theme_label.clear();
        bn::ostringstream stream(theme_label);
        constexpr const char* names[] = { "ROYAL", "VERDANT", "INFERNO", "FROST", "UMBRAL", "STORM", "NEXUS",
                                          "HIGH CONTRAST" };
        stream << "BOARD: " << names[_g.setup.board_theme % 8];
        items[count] = PAUSE_THEME;
        labels[count++] = theme_label.c_str();
    }

    {
        anim_label.clear();
        bn::ostringstream stream(anim_label);
        constexpr const char* names[] = { "FULL", "FAST", "MINIMAL", "OFF" };
        stream << "ANIMATIONS: " << names[int(_g.settings.animations)];
        items[count] = PAUSE_ANIMATIONS;
        labels[count++] = anim_label.c_str();
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

    items[count] = PAUSE_RESIGN;
    labels[count++] = "RESIGN";
    items[count] = PAUSE_QUIT;
    labels[count++] = "QUIT TO MENU";

    if(_menu_index >= count)
    {
        _menu_index = count - 1;
    }

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

    const PauseItem item = items[_menu_index];

    if(bn::keypad::a_pressed())
    {
        switch(item)
        {
        case PAUSE_RESUME:
            _close_overlay();
            _state = _state_before_pause;
            _view.set_cursor_visible(_state != State::AI);
            return;

        case PAUSE_CONTROLS:
            _flash("D-pad move, A select, L magic.");
            break;

        case PAUSE_THEME:
            _g.setup.board_theme = uint8_t((_g.setup.board_theme + 1) % 8);
            _g.settings.board_theme = _g.setup.board_theme;
            _view.set_theme(_g.setup.board_theme);
            redraw = true;
            break;

        case PAUSE_ANIMATIONS:
            _g.settings.animations = AnimationMode((int(_g.settings.animations) + 1) % 4);
            redraw = true;
            break;

        case PAUSE_CLAIM:
            _close_overlay();
            _game.claim_draw();
            _begin_turn();
            return;

        case PAUSE_OFFER:
            _open_confirm(Confirm::OFFER_DRAW);
            return;

        case PAUSE_RESIGN:
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
        _close_overlay();
        _state = _state_before_pause;
        _view.set_cursor_visible(_state != State::AI);
        return;
    }

    if(redraw)
    {
        _draw_menu("PAUSED", labels, count, _menu_index, 24);
    }
}

void MatchScene::_open_confirm(Confirm confirm)
{
    _confirm = confirm;
    _state = State::CONFIRM;
    _menu_index = 1;
    _overlay_open = false;
    _update_confirm();
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
                    Color loser = _g.setup.human_mask == 3 ? _game.position().side :
                                  ((_g.setup.human_mask & 1) ? WHITE : BLACK);
                    _game.resign(loser);
                    _begin_turn();
                }
                return;

            case Confirm::QUIT:
                _next_scene = SceneId::MAIN_MENU;
                return;

            case Confirm::OFFER_DRAW:
                _game.agree_draw();
                _begin_turn();
                return;

            default:
                break;
            }
        }

        _close_overlay();
        _open_pause();
        return;
    }

    if(bn::keypad::b_pressed())
    {
        _close_overlay();
        _open_pause();
        return;
    }

    if(redraw)
    {
        constexpr const char* yes_no[] = { "YES", "NO" };
        const char* title = _confirm == Confirm::RESIGN ? "RESIGN?" :
                            _confirm == Confirm::QUIT ? "QUIT MATCH?" : "ACCEPT DRAW?";
        _draw_menu(title, yes_no, 2, _menu_index, 48);
    }
}

void MatchScene::_open_promotion(int to)
{
    (void) to;
    _state = State::PROMOTION;
    _menu_index = 0;
    _overlay_open = false;
    _update_promotion();
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

    _update_handoff();
}

void MatchScene::_update_handoff()
{
    if(! _overlay_open)
    {
        _overlay_text.clear();
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
        _set_hint(_game.position().rules.arcane ? "Ⓐ Select  Ⓡ Info  START Menu" : "Ⓐ Select  START Menu");
    }
}

void MatchScene::_open_game_over()
{
    _state = State::GAME_OVER;
    _menu_index = 0;
    _overlay_open = false;
    _view.set_cursor_visible(false);
    _update_game_over();
}

void MatchScene::_update_game_over()
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
            _g.setup.resume_suspended = false;
            _start_match();
        }
        else
        {
            _next_scene = SceneId::MAIN_MENU;
        }

        return;
    }

    if(redraw)
    {
        _overlay_text.clear();
        _panel.window(4, 4, 22, 12);
        const GameResult result = _game.result();
        const char* title;
        TitleColor color = TitleColor::GOLD;

        if(result == GameResult::DRAW)
        {
            title = "DRAW";
            color = TitleColor::SILVER;
        }
        else
        {
            const Color winner = result == GameResult::WHITE_WINS ? WHITE : BLACK;
            const bool human_won = (_g.setup.human_mask >> winner) & 1;

            if(_g.setup.human_mask == 3)
            {
                title = winner == WHITE ? "WHITE WINS" : "BLACK WINS";
            }
            else if(human_won)
            {
                title = "VICTORY!";
            }
            else
            {
                title = "DEFEAT";
                color = TitleColor::RUBY;
            }
        }

        print_big(_overlay_text, 120, 40, title, color, Align::CENTER);
        print(_overlay_text, 120, 62, result_reason_name(_game.reason()), TextColor::WHITE, Align::CENTER);

        bn::string<32> moves;
        bn::ostringstream stream(moves);
        stream << "Moves: " << _game.position().fullmove;
        print(_overlay_text, 120, 76, moves, TextColor::GRAY, Align::CENTER);
        print(_overlay_text, 120, 96, "REMATCH", _menu_index == 0 ? TextColor::GOLD : TextColor::WHITE, Align::CENTER);
        print(_overlay_text, 120, 110, "MAIN MENU", _menu_index == 1 ? TextColor::GOLD : TextColor::WHITE,
              Align::CENTER);
        print(_overlay_text, 72, _menu_index == 0 ? 96 : 110, "→", TextColor::GOLD);
        _overlay_open = true;
    }
}

// ------------------------------------------------------------------------------------------------ panel

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
        bn::string<24> name;
        bn::ostringstream stream(name);
        const bool human = (_g.setup.human_mask >> side) & 1;
        const bool to_move = pos.side == side && ! _game.over();
        stream << side_name(side) << (human ? "" : " (AI)") << (to_move ? " ◆" : "");
        print(_panel_text, panel_x, y, name, to_move ? TextColor::GOLD : TextColor::WHITE);

        if(arcane)
        {
            _panel.gems(panel_tile_x + 1, gem_row, pos.mana[side], pos.rules.max_mana);
            bn::string<16> mana;
            bn::ostringstream mana_stream(mana);
            mana_stream << pos.mana[side] << "/" << int(pos.rules.max_mana);
            print(_panel_text, panel_x + 56, gem_row * 8, mana, TextColor::CYAN);
            print(_panel_text, panel_x, gem_row * 8 + 10, pos.escapes[side] ? "Escape ready" : "Escape used",
                  pos.escapes[side] ? TextColor::GRAY : TextColor::RED);
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
        }
    };

    side_block(top_side, 8, 2);
    side_block(bottom_side, 120, 16);

    // Middle block: the selected / hovered piece and its ability, or the AI's thinking state.
    if(_state == State::AI)
    {
        constexpr const char* dots[] = { "Thinking", "Thinking.", "Thinking..", "Thinking..." };
        print(_panel_text, panel_x, 60, dots[(_think_phase >> 3) & 3], TextColor::CYAN);
        print(_panel_text, panel_x, 74, difficulty_name(_g.setup.difficulty), TextColor::GRAY);
        return;
    }

    const int focus = _selected != NO_SQUARE ? _selected : _view.cursor();
    const Piece piece = pos.board[focus];

    if(piece == EMPTY)
    {
        char square[3];
        square_name(focus, square);
        print(_panel_text, panel_x, 60, square, TextColor::GRAY);
        return;
    }

    print(_panel_text, panel_x, 58, piece_title(type_of(piece)), TextColor::WHITE);

    if(! arcane)
    {
        return;
    }

    const Ability ability = ability_for(type_of(piece));
    const bool own = color_of(piece) == pos.side;
    const bool affordable = pos.mana[color_of(piece)] >= pos.rules.cost[ability];
    bool usable = own && affordable && (pos.rules.ability_mask[pos.side] & (1 << ability));

    if(ability == ABILITY_ROYAL_ESCAPE)
    {
        usable = usable && pos.escapes[pos.side] && pos.in_check();
    }

    bn::sprite_ptr icon = bn::sprite_items::ability_icons.create_sprite(panel_x + 8 - 120, 80 + 8 - 80,
                                                                         int(ability) + (usable ? 0 : 6));
    icon.set_bg_priority(0);
    _icons.push_back(bn::move(icon));
    print(_panel_text, panel_x + 20, 76, ability_name(ability), usable ? TextColor::CYAN : TextColor::GRAY);

    bn::string<20> cost;
    bn::ostringstream stream(cost);
    stream << "Cost " << int(pos.rules.cost[ability]) << " ◆";
    print(_panel_text, panel_x + 20, 86, cost, affordable ? TextColor::WHITE : TextColor::RED);

    if(_state == State::MAGIC || _state == State::COMMAND)
    {
        print(_panel_text, panel_x, 98, "Choose a target", TextColor::GOLD);
    }
    else if(own && ! affordable)
    {
        bn::string<20> need;
        bn::ostringstream need_stream(need);
        need_stream << "Need " << int(pos.rules.cost[ability]) << " mana";
        print(_panel_text, panel_x, 98, need, TextColor::RED);
    }
    else if(own && usable && _state == State::SELECTED)
    {
        print(_panel_text, panel_x, 98, "Ⓛ to cast", TextColor::GOLD);
    }
}

void MatchScene::_set_status()
{
    _status_text.clear();

    if(_message_frames > 0)
    {
        print(_status_text, 4, 0, _message, TextColor::RED);
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
    stream << "Move " << pos.fullmove << "  " << side_name(pos.side) << " to play";
    print(_status_text, 4, 0, text, TextColor::WHITE);

    if(pos.in_check())
    {
        print(_status_text, 140, 0, "CHECK!", TextColor::RED, Align::RIGHT);
    }
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
    _message = text;
    _message_frames = 90;
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
}

}
