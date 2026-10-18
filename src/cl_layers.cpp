// CHESS LEGENDS: ARCANE WARS - dynamic tile layers.
#include "cl_layers.h"

#include "bn_assert.h"
#include "bn_bg_tiles.h"
#include "bn_memory.h"
#include "bn_regular_bg_map_cell_info.h"
#include "bn_regular_bg_map_ptr.h"

#include "cl_panel_tiles.h"

#include "bn_bg_palette_items_ui_palette.h"
#include "bn_regular_bg_tiles_items_panel_tiles.h"

namespace cl::gba
{

TileLayer::TileLayer(const bn::regular_bg_tiles_item& tiles, const bn::bg_palette_item& palette, int bg_priority,
                     int z_order) :
    _map_item(_cells[0], bn::size(columns, rows))
{
    bn::memory::clear(_cells);

    // Map cells index the tile set directly, so the tiles must not be placed at an offset.
    bn::bg_tiles::set_allow_offset(false);
    bn::regular_bg_item item(tiles, palette, _map_item);
    _bg = item.create_bg(8, 48);   // map cell (0, 0) at the top-left corner of the screen
    bn::bg_tiles::set_allow_offset(true);

    _bg->set_priority(bg_priority);
    _bg->set_z_order(z_order);
}

void TileLayer::clear(int tile)
{
    bn::regular_bg_map_cell_info info;
    info.set_tile_index(tile);
    bn::regular_bg_map_cell cell = info.cell();

    for(bn::regular_bg_map_cell& c : _cells)
    {
        c = cell;
    }

    _dirty = true;
}

void TileLayer::set(int x, int y, int tile, bool flip_h, bool flip_v, int palette_id)
{
    if(x < 0 || y < 0 || x >= columns || y >= rows)
    {
        return;
    }

    bn::regular_bg_map_cell_info info;
    info.set_tile_index(tile);
    info.set_horizontal_flip(flip_h);
    info.set_vertical_flip(flip_v);
    info.set_palette_id(palette_id);
    _cells[_map_item.cell_index(x, y)] = info.cell();
    _dirty = true;
}

void TileLayer::set_quad(int x, int y, int first)
{
    BN_ASSERT(x >= 0 && y >= 0 && x + 1 < columns && y + 1 < rows, "Quad outside the layer");

    // A map cell with no flip and palette 0 is just the tile index.
    const int index = _map_item.cell_index(x, y);
    const bn::regular_bg_map_cell step = first ? 1 : 0;
    _cells[index] = bn::regular_bg_map_cell(first);
    _cells[index + 1] = bn::regular_bg_map_cell(first + step);
    _cells[index + columns] = bn::regular_bg_map_cell(first + 2 * step);
    _cells[index + columns + 1] = bn::regular_bg_map_cell(first + 3 * step);
    _dirty = true;
}

int TileLayer::tile_at(int x, int y) const
{
    bn::regular_bg_map_cell_info info(_cells[_map_item.cell_index(x, y)]);
    return info.tile_index();
}

void TileLayer::commit()
{
    if(_dirty)
    {
        bn::regular_bg_map_ptr map = _bg->map();
        map.reload_cells_ref();
        _dirty = false;
    }
}

void TileLayer::set_palette(const bn::bg_palette_item& palette)
{
    _bg->set_palette(palette);
}

PanelLayer::PanelLayer(int bg_priority, int z_order) :
    TileLayer(bn::regular_bg_tiles_items::panel_tiles, bn::bg_palette_items::ui_palette, bg_priority, z_order)
{
}

void PanelLayer::window(int x, int y, int width, int height)
{
    using namespace panel_tiles;

    for(int ty = 0; ty < height; ++ty)
    {
        for(int tx = 0; tx < width; ++tx)
        {
            bool left = tx == 0;
            bool right = tx == width - 1;
            bool top = ty == 0;
            bool bottom = ty == height - 1;

            if((left || right) && (top || bottom))
            {
                set(x + tx, y + ty, win_corner, right, bottom);
            }
            else if(top || bottom)
            {
                set(x + tx, y + ty, win_top, false, bottom);
            }
            else if(left || right)
            {
                set(x + tx, y + ty, win_left, right, false);
            }
            else
            {
                set(x + tx, y + ty, win_fill);
            }
        }
    }
}

void PanelLayer::inset(int x, int y, int width, int height)
{
    using namespace panel_tiles;

    for(int ty = 0; ty < height; ++ty)
    {
        for(int tx = 0; tx < width; ++tx)
        {
            int tile = inset_fill;

            if(tx == 0 && ty == 0)
            {
                tile = inset_corner;
            }
            else if(ty == 0)
            {
                tile = inset_top;
            }
            else if(tx == 0)
            {
                tile = inset_left;
            }
            else if(ty == height - 1)
            {
                tile = inset_bottom;
            }
            else if(tx == width - 1)
            {
                tile = inset_right;
            }

            set(x + tx, y + ty, tile);
        }
    }
}

void PanelLayer::divider(int x, int y, int width)
{
    for(int tx = 0; tx < width; ++tx)
    {
        set(x + tx, y, tx == width / 2 ? panel_tiles::divider_center : panel_tiles::divider);
    }
}

void PanelLayer::fill(int x, int y, int width, int height, int tile)
{
    for(int ty = 0; ty < height; ++ty)
    {
        for(int tx = 0; tx < width; ++tx)
        {
            set(x + tx, y + ty, tile);
        }
    }
}

void PanelLayer::gems(int x, int y, int full, int capacity, int preview)
{
    for(int i = 0; i < capacity; ++i)
    {
        int tile = panel_tiles::gem_empty;

        if(i < full)
        {
            tile = i >= full - preview ? panel_tiles::gem_spend : panel_tiles::gem_full;
        }

        set(x + i, y, tile);
    }
}

}
