// CHESS LEGENDS: ARCANE WARS - chessboard presentation.
#include "cl_board_view.h"

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
    for(int8_t& index : _piece_index)
    {
        index = -1;
    }

    for(Marker& m : _markers)
    {
        m = Marker::NONE;
    }

    _board.clear(0);
    _marker_layer.clear(0);
    _draw_frame();
    _draw_squares();
    _board.commit();
    _marker_layer.commit();

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
        _squares_dirty = true;
        set_cursor(_cursor_sq);
    }
}

void BoardView::set_dark_faction(int faction)
{
    _faction = faction;
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

void BoardView::_draw_squares()
{
    using namespace board_tiles;

    for(int index = 0; index < 64; ++index)
    {
        const int sq = sq88(index);
        const int f = file_of(sq);
        const int r = rank_of(sq);
        const bool light = ((f + r) & 1) != 0;
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
            bool variant = ((f * 3 + r * 5) % 7) < 3;
            base = light ? (variant ? light_b : light_a) : (variant ? dark_b : dark_a);
        }

        int column;
        int row;
        _display_cell(sq, column, row);
        const int mx = 1 + column * 2;
        const int my = 2 + row * 2;
        _board.set(mx, my, base);
        _board.set(mx + 1, my, base + 1);
        _board.set(mx, my + 1, base + 2);
        _board.set(mx + 1, my + 1, base + 3);

        const int marker = marker_tile(_markers[index]);

        if(marker)
        {
            _marker_layer.set(mx, my, marker);
            _marker_layer.set(mx + 1, my, marker + 1);
            _marker_layer.set(mx, my + 1, marker + 2);
            _marker_layer.set(mx + 1, my + 1, marker + 3);
        }
        else
        {
            _marker_layer.set(mx, my, 0);
            _marker_layer.set(mx + 1, my, 0);
            _marker_layer.set(mx, my + 1, 0);
            _marker_layer.set(mx + 1, my + 1, 0);
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
    if(_squares_dirty)
    {
        _draw_squares();
    }

    _board.commit();
    _marker_layer.commit();
}

void BoardView::clear_pieces()
{
    _pieces.clear();

    for(int8_t& index : _piece_index)
    {
        index = -1;
    }
}

void BoardView::rebuild_pieces(const Position& pos)
{
    clear_pieces();

    for(int index = 0; index < 64; ++index)
    {
        const int sq = sq88(index);
        const Piece piece = pos.board[sq];

        if(piece == EMPTY)
        {
            continue;
        }

        bn::sprite_ptr sprite = bn::sprite_items::pieces.create_sprite(square_center(sq), type_of(piece) - 1);
        apply_army_palette(sprite, color_of(piece));
        sprite.set_bg_priority(2);
        sprite.set_z_order(0);
        _piece_index[sq] = int8_t(_pieces.size());
        _pieces.push_back(bn::move(sprite));
    }
}

bn::sprite_ptr* BoardView::sprite_at(int sq)
{
    if(! on_board(sq) || _piece_index[sq] < 0)
    {
        return nullptr;
    }

    return &_pieces[_piece_index[sq]];
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

    for(bn::sprite_ptr& sprite : _pieces)
    {
        sprite.set_visible(visible);
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
