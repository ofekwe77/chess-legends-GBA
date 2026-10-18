// CHESS LEGENDS: ARCANE WARS - chessboard presentation (squares, highlights, markers, piece sprites, cursor).
#ifndef CL_BOARD_VIEW_H
#define CL_BOARD_VIEW_H

#include "bn_fixed_point.h"
#include "bn_optional.h"
#include "bn_sprite_animate_actions.h"
#include "bn_sprite_palette_item.h"
#include "bn_sprite_palette_ptr.h"
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

// Sprite palette of an army: the light side, or the dark side (0 obsidian, 1-6 kingdom factions).
[[nodiscard]] const bn::sprite_palette_item& army_palette(Color color, int faction);

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

    // File and rank labels on the frame (accessibility option).
    void set_coordinates_visible(bool visible);

    // Hides / shows the whole board (squares, markers, pieces, cursor) - e.g. during a battle scene.
    void set_visible(bool visible);

    // Dark-side army palette: 0 = obsidian default, 1-6 = kingdom factions.
    void set_dark_faction(int faction);

    // Brings the piece sprites in line with the position: a square whose piece did not change keeps its sprite
    // (put back at rest: position, army palette, visible, opaque), a changed square gets a new sprite or loses its
    // own. A normal move touches two or three sprites instead of all of them.
    void rebuild_pieces(const Position& pos);

    // Releases every piece sprite (a battle scene needs the sprites; rebuild_pieces brings them back).
    void clear_pieces();

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
    bn::optional<bn::sprite_ptr> _pieces[64];     // by sq64
    Piece _shown[64];                             // piece drawn by each sprite
    bn::optional<bn::sprite_palette_ptr> _army_palettes[2];   // current army palettes (light, dark faction)
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
    bool _coordinates = true;
    bool _cursor_wanted = true;
    bool _squares_dirty = true;
    int16_t _drawn_base[64];        // square tile last drawn on each square (-1: redraw)
    int16_t _drawn_marker[64];      // marker tile last drawn on each square

    void _invalidate_squares();
    void _draw_frame();
    void _draw_squares();
    void _display_cell(int sq, int& column, int& row) const;
};

}

#endif
