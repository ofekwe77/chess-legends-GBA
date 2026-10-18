// CHESS LEGENDS: ARCANE WARS - dynamic tile layers (map cells kept in RAM, aligned with the screen).
#ifndef CL_LAYERS_H
#define CL_LAYERS_H

#include "bn_optional.h"
#include "bn_regular_bg_item.h"
#include "bn_regular_bg_map_cell.h"
#include "bn_regular_bg_map_item.h"
#include "bn_regular_bg_ptr.h"

namespace cl::gba
{

// A 32x32-cell regular BG whose map lives in RAM. Cell (0, 0) is the top-left 8x8 tile of the screen.
class TileLayer
{
public:
    static constexpr int columns = 32;
    static constexpr int rows = 32;

    TileLayer(const bn::regular_bg_tiles_item& tiles, const bn::bg_palette_item& palette, int bg_priority,
              int z_order = 0);

    void clear(int tile = 0);
    void set(int x, int y, int tile, bool flip_h = false, bool flip_v = false, int palette_id = 0);

    // Fast 2x2 block (no flip, palette 0): tiles first, first + 1 (top row), first + 2, first + 3; first = 0
    // clears the block. (x, y) must leave room for the block.
    void set_quad(int x, int y, int first);
    [[nodiscard]] int tile_at(int x, int y) const;
    void commit();

    [[nodiscard]] bn::regular_bg_ptr& bg()
    {
        return *_bg;
    }

    void set_palette(const bn::bg_palette_item& palette);

private:
    alignas(int) bn::regular_bg_map_cell _cells[columns * rows];
    bn::regular_bg_map_item _map_item;
    bn::optional<bn::regular_bg_ptr> _bg;
    bool _dirty = false;
};

// Interface panels drawn with the panel tile set (gold-trimmed windows, inset boxes, dividers, mana gems).
class PanelLayer : public TileLayer
{
public:
    explicit PanelLayer(int bg_priority = 1, int z_order = 0);

    void window(int x, int y, int width, int height);
    void inset(int x, int y, int width, int height);
    void divider(int x, int y, int width);
    void fill(int x, int y, int width, int height, int tile);

    // Mana gems: `full` filled gems, `preview` of them shown as being spent, up to `capacity` sockets.
    void gems(int x, int y, int full, int capacity, int preview = 0);
};

}

#endif
