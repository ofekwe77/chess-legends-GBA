// CHESS LEGENDS: ARCANE WARS - the match scene (every mode: quick matches, story, challenges, gauntlet).
//
// The game state is changed in exactly one place (_execute -> Game::apply). Objectives, statistics and achievements
// observe the returned ActionInfo; animations only present a state that is already final.
#ifndef CL_SCENE_MATCH_H
#define CL_SCENE_MATCH_H

#include "bn_fixed_point.h"
#include "bn_sprite_ptr.h"
#include "bn_string.h"
#include "bn_vector.h"

#include "cl_board_view.h"
#include "cl_campaign.h"
#include "cl_progress.h"
#include "cl_scene.h"

namespace cl::gba
{

class MatchScene final : public Scene
{
public:
    MatchScene();
    ~MatchScene() override;

    [[nodiscard]] bn::optional<SceneId> update() override;

    // Called from inside the AI search about once per frame.
    bool think_frame();

private:
    enum class State : uint8_t
    {
        HUMAN,          // choosing a piece
        SELECTED,       // a piece is selected, normal destinations shown
        MAGIC,          // magical destinations shown
        COMMAND,        // Royal Command: ally chosen, destinations shown
        PROMOTION,      // promotion piece menu
        ANIMATING,
        AI,
        HANDOFF,        // hot-seat: pass the console
        PAUSE,
        SETTINGS,       // pause > settings
        HELP,           // help pages or a rule card
        CONFIRM,        // yes / no question
        GAME_OVER
    };

    enum class Confirm : uint8_t
    {
        RESIGN,
        QUIT,
        OFFER_DRAW
    };

    enum class InfoView : uint8_t
    {
        PIECE,          // hovered / selected piece and its ability
        THREATS,        // who attacks the hovered square
        GAME            // material, captures, last move, objective
    };

    enum class EndOption : uint8_t
    {
        REMATCH,
        MAIN_MENU,
        CONTINUE,           // story: on to the victory / defeat dialogue
        RETRY,
        NEXT_CHALLENGE,
        CHALLENGE_LIST,
        GAUNTLET_NEXT,      // gauntlet: next stage / run summary
    };

    struct Animation
    {
        Move move;
        ActionInfo info;
        int frame = 0;
        int length = 0;
        bn::fixed_point mover_from;
        bn::fixed_point mover_to;
        bn::fixed_point partner_from;
        bn::fixed_point partner_to;
        int mover_sq = NO_SQUARE;
        int partner_sq = NO_SQUARE;
        int victim_sq = NO_SQUARE;
    };

    Globals& _g;
    Game& _game;
    BoardView _view;
    PanelLayer _panel;
    bn::vector<bn::sprite_ptr, 40> _panel_text;
    bn::vector<bn::sprite_ptr, 12> _status_text;
    bn::vector<bn::sprite_ptr, 14> _hint_text;
    bn::vector<bn::sprite_ptr, 64> _overlay_text;
    bn::vector<bn::sprite_ptr, 8> _icons;
    MoveList _legal;
    Animation _anim;
    ObjectiveTracker _tracker;
    MatchRecorder _recorder;
    ToastBanner _toast;
    State _state = State::HUMAN;
    State _state_before_pause = State::HUMAN;
    Confirm _confirm = Confirm::RESIGN;
    InfoView _info = InfoView::PIECE;
    RepeatInput _input;
    int _selected = NO_SQUARE;
    int _command_queen = NO_SQUARE;
    int _command_ally = NO_SQUARE;
    Move _promotions[4];
    int _promotion_count = 0;
    int _menu_index = 0;
    int _menu_count = 0;
    int _message_frames = 0;
    int _think_phase = 0;
    int _ai_last_score = 0;
    int _help_page = 0;
    int _help_ability = -1;             // rule card shown by SELECT (-1: the help pages)
    int _medal = 0;                     // challenge medal earned by this attempt
    int _end_count = 0;
    EndOption _end_options[3];
    uint32_t _match_seed = 1;
    Color _player = WHITE;              // the person's side in single-player modes
    bool _panel_dirty = true;
    bool _overlay_open = false;
    bool _pause_requested = false;
    bool _standard_start = true;
    bool _scenario = false;             // story / challenge / gauntlet objective in force
    bool _result_recorded = false;
    bool _objective_success = false;
    bool _threat_markers = false;
    bool _help_from_pause = false;
    bool _settings_changed = false;
    bn::optional<SceneId> _next_scene;
    bn::string<48> _message;
    bn::string<16> _opponent_label;     // short opponent name in scenarios
    int _captured[2][7] = {};
    Move _last_move;

    // flow
    void _start_match();
    void _setup_mode();
    [[nodiscard]] MatchContext _context() const;
    void _begin_turn();
    [[nodiscard]] bool _human_to_move() const;
    void _after_action(const ActionInfo& info);
    void _record_result();
    void _suspend_and_quit();
    [[nodiscard]] SceneId _exit_scene() const;

    // human input
    void _update_human();
    void _move_cursor();
    void _select(int sq);
    void _deselect();
    void _enter_magic();
    void _try_destination(int sq);
    void _show_normal_targets();
    void _show_magic_targets();
    void _show_command_destinations();
    void _show_threat_markers();
    [[nodiscard]] Ability _ability_of(int sq) const;
    [[nodiscard]] const char* _idle_hint() const;

    // actions
    void _execute(Move move);
    void _update_animation();
    void _finish_animation();
    [[nodiscard]] int _animation_length(const Move& move) const;

    // AI
    void _update_ai();
    [[nodiscard]] AiSettings _ai_settings() const;

    // menus and overlays
    void _open_pause();
    void _update_pause();
    void _open_settings();
    void _update_settings();
    void _open_help(int ability);
    void _update_help();
    void _open_confirm(Confirm confirm);
    void _update_confirm();
    void _open_promotion(int to);
    void _update_promotion();
    void _open_handoff();
    void _update_handoff();
    void _open_game_over();
    void _update_game_over();
    void _draw_game_over();
    void _close_overlay();
    void _resume_from_pause();
    void _draw_menu(const char* title, const char* const* items, int count, int selected, int y = 40);

    // panel / status
    void _draw_panel_frame();
    void _refresh_panel();
    void _panel_piece(const Position& pos);
    void _panel_threats(const Position& pos);
    void _panel_game(const Position& pos);
    void _set_hint(const char* text);
    void _set_status();
    void _flash(const char* text);
    void _publish_state();
    [[nodiscard]] bn::string<20> _side_label(Color side) const;
};

}

#endif
