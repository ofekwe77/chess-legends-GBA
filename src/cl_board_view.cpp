// CHESS LEGENDS: ARCANE WARS - chessboard presentation.
#include "cl_board_view.h"

#include "cl_scene.h"

#include "bn_bg_palette_item.h"
#include "bn_sprite_palette_item.h"

#include "cl_board_tiles.h"

#include "bn_bg_palette_items_board_palette_contrast.h"
#include "bn_bg_palette_items_board_palette_frost.h"
#include "bn_bg_palette_items_board_palette_inferno.h"
#include "bn_bg_palette_items_board_palette_nexus.h"
#include "bn_bg_palette_items_board_palette_royal.h"
#include "bn_bg_palette_items_board_palette_storm.h"
#include "bn_bg_palette_items_board_palette_umbral.h"
#include "bn_bg_palette_items_board_palette_verdant.h"
#include "bn_bg_palette_items_marker_palette.h"
#include "bn_regular_bg_tiles_items_board_tiles.h"
#include "bn_regular_bg_tiles_items_marker_tiles.h"
#include "bn_sprite_items_board_cursor.h"
#include "bn_sprite_items_pieces.h"
#include "bn_sprite_palette_items_army_black.h"
#include "bn_sprite_palette_items_army_frost.h"
#include "bn_sprite_palette_items_army_inferno.h"
#include "bn_sprite_palette_items_army_nexus.h"
#include "bn_sprite_palette_items_army_storm.h"
#include "bn_sprite_palette_items_army_umbral.h"
#include "bn_sprite_palette_items_army_verdant.h"
#include "bn_sprite_palette_items_army_white.h"

namespace cl::gba
{

namespace
{
    const bn::bg_palette_item& theme_palette(int theme)
    {
        switch(theme)
        {
        case board_themes::verdant: return bn::bg_palette_items::board_palette_verdant;
        case board_themes::inferno: return bn::bg_palette_items::board_palette_inferno;
        case board_themes::frost: return bn::bg_palette_items::board_palette_frost;
        case board_themes::umbral: return bn::bg_palette_items::board_palette_umbral;
        case board_themes::storm: return bn::bg_palette_items::board_palette_storm;
        case board_themes::nexus: return bn::bg_palette_items::board_palette_nexus;
        case board_themes::contrast: return bn::bg_palette_items::board_palette_contrast;
        default: return bn::bg_palette_items::board_palette_royal;
        }
    }

    const bn::sprite_palette_item& dark_palette(int faction)
    {
        switch(faction)
        {
        case 1: return bn::sprite_palette_items::army_verdant;
        case 2: return bn::sprite_palette_items::army_inferno;
        case 3: return bn::sprite_palette_items::army_frost;
        case 4: return bn::sprite_palette_items::army_umbral;
        case 5: return bn::sprite_palette_items::army_storm;
        case 6: return bn::sprite_palette_items::army_nexus;
        default: return bn::sprite_palette_items::army_black;
        }
    }

    int marker_tile(Marker marker)
    {
        switch(marker)
        {
        case Marker::MOVE: return marker_tiles::move;
        case Marker::CAPTURE: return marker_tiles::capture;
        case Marker::MAGIC: return marker_tiles::magic;
        case Marker::SPECIAL: return marker_tiles::special;
        case Marker::TARGET: return marker_tiles::target;
        case Marker::HINT: return marker_tiles::hint;
        default: return marker_tiles::empty;
        }
    }
}

BoardView::BoardView(int theme, bool flipped) :
    _board(bn::regular_bg_tiles_items::board_tiles, theme_palette(theme), 3, 0),
    _marker_layer(bn::regular_bg_tiles_items::marker_tiles, bn::bg_palette_items::marker_palette, 2, 0),
    _theme(theme),
    _flipped(flipped)
{
    for(Piece& piece : _shown)
    {
        piece = EMPTY;
    }

    for(Marker& m : _markers)
    {
        m = Marker::NONE;
    }

    _invalidate_squares();

    _board.clear(0);
    _marker_layer.clear(0);
    _draw_frame();
    _draw_squares();
    _board.commit();
    _marker_layer.commit();

    set_dark_faction(0);
    _cursor = bn::sprite_items::board_cursor.create_sprite(square_center(_cursor_sq));
    _cursor->set_bg_priority(1);
    _cursor->set_z_order(-10);
    _cursor_action = bn::create_sprite_animate_action_forever(
                *_cursor, 6, bn::sprite_items::board_cursor.tiles_item(), 0, 1, 2, 3);
}

void BoardView::set_theme(int theme)
{
    _theme = theme;
    _board.set_palette(theme_palette(theme));
}

void BoardView::set_flipped(bool flipped)
{
    if(flipped != _flipped)
    {
        _flipped = flipped;
        _draw_frame();
        _invalidate_squares();     // every square moves on screen
        set_cursor(_cursor_sq);
    }
}

void BoardView::_invalidate_squares()
{
    for(int index = 0; index < 64; ++index)
    {
        _drawn_base[index] = -1;
        _drawn_marker[index] = -1;
    }

    _squares_dirty = true;
}

void BoardView::set_dark_faction(int faction)
{
    _faction = faction;
    _army_palettes[WHITE] = bn::sprite_palette_items::army_white.create_palette();
    _army_palettes[BLACK] = dark_palette(faction).create_palette();
}

void BoardView::apply_army_palette(bn::sprite_ptr& sprite, Color color) const
{
    if(color == WHITE)
    {
        sprite.set_palette(bn::sprite_palette_items::army_white);
    }
    else
    {
        sprite.set_palette(dark_palette(_faction));
    }
}

void BoardView::_display_cell(int sq, int& column, int& row) const
{
    int f = file_of(sq);
    int r = rank_of(sq);
    column = _flipped ? 7 - f : f;
    row = _flipped ? r : 7 - r;
}

void BoardView::square_screen(int sq, int& x, int& y) const
{
    int column;
    int row;
    _display_cell(sq, column, row);
    x = left + column * 16;
    y = top + row * 16;
}

bn::fixed_point BoardView::square_center(int sq) const
{
    int x;
    int y;
    square_screen(sq, x, y);
    return bn::fixed_point(x + 8 - 120, y + 8 - 80);
}

void BoardView::_draw_frame()
{
    using namespace board_tiles;

    // Frame occupies map columns 0 and 17, rows 1 and 18.
    for(int x = 1; x <= 16; ++x)
    {
        _board.set(x, 1, frame_top);
        _board.set(x, 18, frame_bottom);
    }

    for(int y = 2; y <= 17; ++y)
    {
        _board.set(0, y, frame_left);
        _board.set(17, y, frame_right);
    }

    _board.set(0, 1, frame_corner);
    _board.set(17, 1, frame_corner, true, false);
    _board.set(0, 18, frame_corner, false, true);
    _board.set(17, 18, frame_corner, true, true);

    if(! _coordinates)
    {
        return;
    }

    for(int column = 0; column < 8; ++column)
    {
        int f = _flipped ? 7 - column : column;
        _board.set(1 + column * 2, 18, file_a + f * 2);
        _board.set(2 + column * 2, 18, file_a + f * 2 + 1);
    }

    for(int row = 0; row < 8; ++row)
    {
        int r = _flipped ? row : 7 - row;
        _board.set(0, 2 + row * 2, rank_1 + r * 2);
        _board.set(0, 3 + row * 2, rank_1 + r * 2 + 1);
    }
}

void BoardView::set_coordinates_visible(bool visible)
{
    if(visible != _coordinates)
    {
        _coordinates = visible;
        _draw_frame();
        _board.commit();
    }
}

namespace
{
    // Texture variant of each square (a fixed pattern: square_variant[sq64]).
    constexpr bool variant_of(int index)
    {
        return (((index & 7) * 3 + (index >> 3) * 5) % 7) < 3;
    }

    constexpr bool square_variant[64] = {
        variant_of(0), variant_of(1), variant_of(2), variant_of(3), variant_of(4), variant_of(5), variant_of(6),
        variant_of(7), variant_of(8), variant_of(9), variant_of(10), variant_of(11), variant_of(12), variant_of(13),
        variant_of(14), variant_of(15), variant_of(16), variant_of(17), variant_of(18), variant_of(19),
        variant_of(20), variant_of(21), variant_of(22), variant_of(23), variant_of(24), variant_of(25),
        variant_of(26), variant_of(27), variant_of(28), variant_of(29), variant_of(30), variant_of(31),
        variant_of(32), variant_of(33), variant_of(34), variant_of(35), variant_of(36), variant_of(37),
        variant_of(38), variant_of(39), variant_of(40), variant_of(41), variant_of(42), variant_of(43),
        variant_of(44), variant_of(45), variant_of(46), variant_of(47), variant_of(48), variant_of(49),
        variant_of(50), variant_of(51), variant_of(52), variant_of(53), variant_of(54), variant_of(55),
        variant_of(56), variant_of(57), variant_of(58), variant_of(59), variant_of(60), variant_of(61),
        variant_of(62), variant_of(63),
    };
}

void BoardView::_draw_squares()
{
    using namespace board_tiles;

    // Only squares whose look changed are written (a full redraw cost most of a frame on the console).
    for(int index = 0; index < 64; ++index)
    {
        const int sq = sq88(index);
        const bool light = (((index & 7) + (index >> 3)) & 1) != 0;
        int base;

        if(sq == _check)
        {
            base = light ? light_check : dark_check;
        }
        else if(sq == _selected)
        {
            base = light ? light_sel : dark_sel;
        }
        else if(sq == _last_from || sq == _last_to)
        {
            base = light ? light_last : dark_last;
        }
        else
        {
            base = light ? (square_variant[index] ? light_b : light_a) : (square_variant[index] ? dark_b : dark_a);
        }

        const int marker = marker_tile(_markers[index]);

        if(base == _drawn_base[index] && marker == _drawn_marker[index])
        {
            continue;
        }

        int column;
        int row;
        _display_cell(sq, column, row);
        const int mx = 1 + column * 2;
        const int my = 2 + row * 2;

        if(base != _drawn_base[index])
        {
            _board.set_quad(mx, my, base);
            _drawn_base[index] = int16_t(base);
        }

        if(marker != _drawn_marker[index])
        {
            _marker_layer.set_quad(mx, my, marker);
            _drawn_marker[index] = int16_t(marker);
        }
    }

    _squares_dirty = false;
}

void BoardView::set_selected(int sq)
{
    if(sq != _selected)
    {
        _selected = sq;
        _squares_dirty = true;
    }
}

void BoardView::set_last_move(int from, int to)
{
    _last_from = from;
    _last_to = to;
    _squares_dirty = true;
}

void BoardView::set_check(int sq)
{
    if(sq != _check)
    {
        _check = sq;
        _squares_dirty = true;
    }
}

void BoardView::clear_markers()
{
    for(Marker& m : _markers)
    {
        if(m != Marker::NONE)
        {
            m = Marker::NONE;
            _squares_dirty = true;
        }
    }
}

void BoardView::set_marker(int sq, Marker marker)
{
    Marker& m = _markers[sq64(sq)];

    if(m != marker)
    {
        m = marker;
        _squares_dirty = true;
    }
}

void BoardView::commit()
{
    ProfileScope profile(ProfileSlot::BOARD_COMMIT);
    if(_squares_dirty)
    {
        _draw_squares();
    }

    _board.commit();
    _marker_layer.commit();
}

void BoardView::clear_pieces()
{
    for(int index = 0; index < 64; ++index)
    {
        _pieces[index].reset();
        _shown[index] = EMPTY;
    }
}

void BoardView::rebuild_pieces(const Position& pos)
{
    ProfileScope profile(ProfileSlot::PIECES);

    for(int index = 0; index < 64; ++index)
    {
        const int sq = sq88(index);
        const Piece piece = pos.board[sq];
        bn::optional<bn::sprite_ptr>& sprite = _pieces[index];

        if(piece != _shown[index])
        {
            sprite.reset();
            _shown[index] = piece;

            if(piece != EMPTY)
            {
                sprite = bn::sprite_items::pieces.create_sprite(square_center(sq), type_of(piece) - 1);
                sprite->set_bg_priority(2);
            }
        }

        if(sprite)
        {
            // Back at rest (an animation may have moved, tinted, hidden or faded it).
            sprite->set_position(square_center(sq));
            sprite->set_z_order(0);
            sprite->set_visible(true);
            sprite->set_blending_enabled(false);
            const bn::sprite_palette_ptr& army = _army_palettes[color_of(piece)].value();

            if(sprite->palette() != army)
            {
                sprite->set_palette(army);
            }
        }
    }
}

bn::sprite_ptr* BoardView::sprite_at(int sq)
{
    if(! on_board(sq))
    {
        return nullptr;
    }

    bn::optional<bn::sprite_ptr>& sprite = _pieces[sq64(sq)];
    return sprite ? &*sprite : nullptr;
}

void BoardView::hide_piece(int sq)
{
    if(bn::sprite_ptr* sprite = sprite_at(sq))
    {
        sprite->set_visible(false);
    }
}

void BoardView::set_cursor(int sq)
{
    _cursor_sq = sq;
    _cursor->set_position(square_center(sq));
}

void BoardView::set_cursor_visible(bool visible)
{
    _cursor_wanted = visible;
    _cursor->set_visible(visible);
}

const bn::sprite_palette_item& army_palette(Color color, int faction)
{
    return color == WHITE ? bn::sprite_palette_items::army_white : dark_palette(faction);
}

void BoardView::set_visible(bool visible)
{
    _board.bg().set_visible(visible);
    _marker_layer.bg().set_visible(visible);

    for(bn::optional<bn::sprite_ptr>& sprite : _pieces)
    {
        if(sprite)
        {
            sprite->set_visible(visible);
        }
    }

    if(_cursor)
    {
        _cursor->set_visible(visible && _cursor_wanted);
    }
}

void BoardView::update()
{
    if(_cursor_action && _cursor->visible())
    {
        _cursor_action->update();
    }
}

}
