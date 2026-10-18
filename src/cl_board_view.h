// CHESS LEGENDS: ARCANE WARS - chessboard presentation (squares, highlights, markers, piece sprites, cursor).
#ifndef CL_BOARD_VIEW_H
#define CL_BOARD_VIEW_H

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_ptr.h"
#include "bn_vector.h"

#include "cl_layers.h"
#include "cl_position.h"

namespace cl::gba
{

enum class Marker : uint8_t
{
    NONE,
    MOVE,       // normal destination
    CAPTURE,    // normal capture
    MAGIC,      // magical destination
    SPECIAL,    // Royal Escape landing / Castle Swap partner
    TARGET,     // Royal Command ally / spell source
    HINT        // inspection preview
};

class BoardView
{
public:
    // Board placement on screen (pixels): squares start at (left, top), 16 pixels each.
    static constexpr int left = 8;
    static constexpr int top = 16;

    BoardView(int theme, bool flipped);

    void set_theme(int theme);

    [[nodiscard]] bool flipped() const
    {
        return _flipped;
    }

    void set_flipped(bool flipped);

    // Dark-side army palette: 0 = obsidian default, 1-6 = kingdom factions.
    void set_dark_faction(int faction);

    // Recreates every piece sprite from the position.
    void rebuild_pieces(const Position& pos);

    void set_selected(int sq);
    void set_last_move(int from, int to);
    void set_check(int sq);
    void clear_markers();
    void set_marker(int sq, Marker marker);
    [[nodiscard]] Marker marker(int sq) const
    {
        return _markers[sq64(sq)];
    }

    // Applies pending square / marker changes to the BG maps.
    void commit();

    void set_cursor(int sq);
    void set_cursor_visible(bool visible);

    [[nodiscard]] int cursor() const
    {
        return _cursor_sq;
    }

    // Screen pixel coordinates of a square's top-left corner, and Butano coordinates of its centre.
    void square_screen(int sq, int& x, int& y) const;
    [[nodiscard]] bn::fixed_point square_center(int sq) const;

    // Piece sprite currently drawn on a square (nullptr if none).
    [[nodiscard]] bn::sprite_ptr* sprite_at(int sq);

    // Moves the sprite bookkeeping of a square to another square (after an animation, before a rebuild).
    void hide_piece(int sq);

    void update();

    [[nodiscard]] TileLayer& board_layer()
    {
        return _board;
    }

    [[nodiscard]] TileLayer& marker_layer()
    {
        return _marker_layer;
    }

    // Applies the dark-side palette (default or faction) to a sprite.
    void apply_army_palette(bn::sprite_ptr& sprite, Color color) const;

private:
    TileLayer _board;
    TileLayer _marker_layer;
    bn::vector<bn::sprite_ptr, 32> _pieces;
    int8_t _piece_index[128];
    Marker _markers[64];
    bn::optional<bn::sprite_ptr> _cursor;
    bn::optional<bn::sprite_animate_action<4>> _cursor_action;
    int _theme = 0;
    int _faction = 0;
    int _selected = NO_SQUARE;
    int _last_from = NO_SQUARE;
    int _last_to = NO_SQUARE;
    int _check = NO_SQUARE;
    int _cursor_sq = 0x14;
    bool _flipped = false;
    bool _squares_dirty = true;

    void _draw_frame();
    void _draw_squares();
    void _display_cell(int sq, int& column, int& row) const;
};

}

#endif
