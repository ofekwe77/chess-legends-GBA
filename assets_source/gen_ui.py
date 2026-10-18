"""Interface art: panel frame tiles, mana gems, ability / difficulty icons and mini pieces.

Panel tiles form a 9-slice set (corners and edges are mirrored by the map), in two styles: 'window' (gold trim)
and 'inset' (dark recessed box). All use the UI palette (palettes.UI).
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gfx'))

from canvas import Canvas  # noqa: E402
from imageio import write_bmp, write_json, write_png  # noqa: E402
import palettes as P  # noqa: E402

INK, PD0, PD1, PM, PL = 1, 2, 3, 4, 5
G0, G1, G2, G3 = 6, 7, 8, 9
WHITE, MUTED, RED, CYAN, VIOLET, GREEN = 10, 11, 12, 13, 14, 15


# ------------------------------------------------------------------------------------------ panel tiles

def window_fill():
    c = Canvas(8, 8, fill=PD1)
    # faint woven texture
    for y in range(8):
        for x in range(8):
            if (x + 2 * y) % 8 == 0:
                c.set(x, y, PD0)
    return c


def window_top():
    c = window_fill()
    for x in range(8):
        c.set(x, 0, INK)
        c.set(x, 1, G1)
        c.set(x, 2, G2 if x % 4 else G3)
        c.set(x, 3, G0)
        c.set(x, 4, INK)
        c.set(x, 5, PD0)
    return c


def window_left():
    c = window_fill()
    for y in range(8):
        c.set(0, y, INK)
        c.set(1, y, G1)
        c.set(2, y, G2 if y % 4 else G3)
        c.set(3, y, G0)
        c.set(4, y, INK)
        c.set(5, y, PD0)
    return c


def window_corner():
    c = Canvas(8, 8)
    c.fill(PD1)
    for i in range(8):
        c.set(i, 0, INK)
        c.set(0, i, INK)
    # gold rosette in the corner
    c.rect(1, 1, 5, 5, G1)
    c.frame(1, 1, 5, 5, G0)
    c.set(2, 2, G3)
    c.set(3, 2, G2)
    c.set(2, 3, G2)
    c.set(3, 3, G2)
    c.set(4, 4, G0)
    for x in range(6, 8):
        c.set(x, 1, G1)
        c.set(x, 2, G2)
        c.set(x, 3, G0)
        c.set(x, 4, INK)
        c.set(x, 5, PD0)
    for y in range(6, 8):
        c.set(1, y, G1)
        c.set(2, y, G2)
        c.set(3, y, G0)
        c.set(4, y, INK)
        c.set(5, y, PD0)
    return c


def inset_fill():
    return Canvas(8, 8, fill=PD0)


def inset_top():
    c = inset_fill()
    for x in range(8):
        c.set(x, 0, INK)
        c.set(x, 1, PD0)
    return c


def inset_left():
    c = inset_fill()
    for y in range(8):
        c.set(0, y, INK)
    return c


def inset_bottom():
    c = inset_fill()
    for x in range(8):
        c.set(x, 7, PM)
    return c


def inset_right():
    c = inset_fill()
    for y in range(8):
        c.set(7, y, PM)
    return c


def inset_corner_tl():
    c = inset_fill()
    for i in range(8):
        c.set(i, 0, INK)
        c.set(0, i, INK)
    return c


def divider():
    c = window_fill()
    for x in range(8):
        c.set(x, 3, G0)
        c.set(x, 4, G1 if x % 2 else G2)
    return c


def divider_center():
    c = divider()
    c.rect(2, 1, 4, 6, G1)
    c.set(3, 0, G2)
    c.set(4, 0, G2)
    c.set(3, 7, G0)
    c.set(4, 7, G0)
    c.set(3, 3, G3)
    c.set(4, 4, G0)
    return c


def gem_full(color_dark, color_light):
    c = Canvas(8, 8)
    c.polygon([(3.5, 0.0), (7.0, 3.5), (3.5, 7.0), (0.0, 3.5)], color_dark)
    c.polygon([(3.5, 1.0), (5.5, 3.5), (3.5, 5.0), (1.5, 3.5)], color_light)
    c.set(3, 2, WHITE)
    c.set(2, 3, WHITE)
    c.outline(INK)
    return c


def gem_empty():
    c = Canvas(8, 8)
    c.polygon([(3.5, 0.0), (7.0, 3.5), (3.5, 7.0), (0.0, 3.5)], PD0)
    c.polygon([(3.5, 1.5), (5.0, 3.5), (3.5, 5.0), (2.0, 3.5)], PD1)
    c.outline(MUTED)
    return c


class TileSet:
    def __init__(self):
        self.tiles = [Canvas(8, 8)]
        self.names = {'empty': 0}

    def add(self, name, canvas):
        self.names[name] = len(self.tiles)
        self.tiles.append(canvas)

    def sheet(self):
        out = Canvas(8, 8 * len(self.tiles))
        for i, t in enumerate(self.tiles):
            out.blit(t, 0, i * 8, transparent=-1)
        return out


def build_panel_tiles():
    ts = TileSet()
    ts.add('win_fill', window_fill())
    ts.add('win_top', window_top())
    ts.add('win_left', window_left())
    ts.add('win_corner', window_corner())
    ts.add('inset_fill', inset_fill())
    ts.add('inset_top', inset_top())
    ts.add('inset_left', inset_left())
    ts.add('inset_bottom', inset_bottom())
    ts.add('inset_right', inset_right())
    ts.add('inset_corner', inset_corner_tl())
    ts.add('divider', divider())
    ts.add('divider_center', divider_center())
    ts.add('gem_full', gem_full(VIOLET, CYAN))
    ts.add('gem_empty', gem_empty())
    ts.add('gem_spend', gem_full(G0, G2))
    ts.add('solid_ink', Canvas(8, 8, fill=INK))
    return ts


# ------------------------------------------------------------------------------------------ icons

ABILITY_COLORS = {
    'shadow_step': (VIOLET, PD0),
    'phase_shift': (CYAN, PM),
    'ghost_walk': (GREEN, PD1),
    'castle_swap': (G1, G0),
    'royal_command': (VIOLET, G0),
    'royal_escape': (G2, G0),
}

ABILITY_SYMBOLS = {
    # 10x10 white symbols centred on the badge
    'shadow_step': ['..........', '....##....', '...####...', '....##....', '...####...', '#.######.#',
                    '##.####.##', '#........#', '..........', '..........'],
    'phase_shift': ['....#.....', '....##....', '.#..###...', '..######..', '.########.', '..######..',
                    '...###..#.', '....##....', '.....#....', '..........'],
    'ghost_walk': ['...####...', '..######..', '.##.##.##.', '.##.##.##.', '.########.', '.########.',
                   '.########.', '.##.##.##.', '.#..#...#.', '..........'],
    'castle_swap': ['..........', '.##....##.', '.######...', '.##....#..', '.......##.', '.#.......#',
                    '..#....##.', '...######.', '.##....##.', '..........'],
    'royal_command': ['#..#..#..#', '##.##.##.#', '##########', '.########.', '..........', '...####...',
                      '..#....##.', '.......##.', '...######.', '..........'],
    'royal_escape': ['....##....', '...####...', '#..####..#', '##.####.##', '##########', '.########.',
                     '..######..', '..#.##.#..', '.#..##..#.', '..........'],
}


def ability_icon(name, enabled=True):
    c = Canvas(16, 16)
    main, dark = ABILITY_COLORS[name]
    if not enabled:
        main, dark = MUTED, PD0
    c.circle(8, 8, 7.4, dark)
    c.circle(8, 8, 6.3, main)
    c.circle(7, 7, 4.0, main)
    # glossy rim
    for x, y in ((4, 3), (5, 2), (3, 4), (6, 2)):
        c.set(x, y, WHITE if enabled else PL)
    symbol = ABILITY_SYMBOLS[name]
    for y, row in enumerate(symbol):
        for x, ch in enumerate(row):
            if ch == '#':
                c.set(3 + x, 3 + y, INK if not enabled else WHITE)
    # symbol shadow for depth
    shadow = Canvas(16, 16)
    for y in range(16):
        for x in range(16):
            if c.get(x, y) == (WHITE if enabled else INK) and (x, y) not in ((4, 3), (5, 2), (3, 4), (6, 2)):
                shadow.set(x + 1, y + 1, 1)
    for y in range(16):
        for x in range(16):
            if shadow.get(x, y) and c.get(x, y) in (main, dark):
                c.set(x, y, dark)
    c.outline(INK)
    return c


DIFFICULTY_SYMBOLS = {
    'apprentice': ['..######..', '.#......#.', '.#.####.#.', '.#......#.', '.#.####.#.', '.#......#.',
                   '.#.###..#.', '.#......#.', '..######..', '..........'],
    'knight': ['...####...', '..######..', '.##.##.##.', '.########.', '.##....##.', '.########.',
               '..######..', '..##..##..', '..........', '..........'],
    'sorcerer': ['.......##.', '......####', '.......##.', '......#...', '.....#....', '....#.....',
                 '...#......', '..#.......', '.#........', '..........'],
    'grandmaster': ['#..#..#..#', '##.##.##.#', '##########', '#.#.##.#.#', '##########', '.########.',
                    '..........', '..........', '..........', '..........'],
}

DIFFICULTY_COLORS = {'apprentice': (GREEN, PD1), 'knight': (CYAN, PM), 'sorcerer': (VIOLET, PD0),
                     'grandmaster': (G2, G0)}


def difficulty_icon(name):
    c = Canvas(16, 16)
    main, dark = DIFFICULTY_COLORS[name]
    c.rect(1, 1, 14, 14, dark)
    c.rect(2, 2, 12, 12, main)
    for y, row in enumerate(DIFFICULTY_SYMBOLS[name]):
        for x, ch in enumerate(row):
            if ch == '#':
                c.set(3 + x, 3 + y, WHITE)
    c.set(2, 2, WHITE)
    c.outline(INK)
    return c


MINI = {
    'pawn': ['..##..', '.####.', '..##..', '.####.', '######'],
    'knight': ['.###..', '##.##.', '.####.', '.####.', '######'],
    'bishop': ['..#...', '.###..', '.##.#.', '..##..', '######'],
    'rook': ['#.##.#', '######', '.####.', '.####.', '######'],
    'queen': ['#.#.#.', '######', '.####.', '..##..', '######'],
    'king': ['..#...', '.###..', '#####.', '.###..', '#####.'],
}


def mini_piece(name):
    """8x8 mini piece for the captured-material display (army palette roles: body 4, outline 1)."""
    c = Canvas(8, 8)
    for y, row in enumerate(MINI[name]):
        for x, ch in enumerate(row):
            if ch == '#':
                c.set(1 + x, 2 + y, 4 if y < 2 else 3)
    c.outline(1)
    return c


def generate(root):
    graphics = os.path.join(root, 'graphics')
    include = os.path.join(root, 'include')
    panel = build_panel_tiles()
    write_bmp(os.path.join(graphics, 'panel_tiles.bmp'), panel.sheet(), P.UI, bpp=4)
    write_json(os.path.join(graphics, 'panel_tiles.json'), {'type': 'regular_bg_tiles', 'bpp_mode': 'bpp_4'})
    swatch = Canvas(8, 8)
    for i in range(16):
        swatch.set(i % 8, i // 8, i)
    write_bmp(os.path.join(graphics, 'ui_palette.bmp'), swatch, P.UI, bpp=4)
    write_json(os.path.join(graphics, 'ui_palette.json'), {'type': 'bg_palette', 'bpp_mode': 'bpp_4'})
    write_bmp(os.path.join(graphics, 'ui_sprite_palette.bmp'), swatch, P.UI, bpp=4)
    write_json(os.path.join(graphics, 'ui_sprite_palette.json'), {'type': 'sprite_palette'})

    abilities = ['shadow_step', 'phase_shift', 'ghost_walk', 'castle_swap', 'royal_command', 'royal_escape']
    icons = Canvas(16, 16 * 12)
    for i, name in enumerate(abilities):
        icons.blit(ability_icon(name, True), 0, i * 16)
        icons.blit(ability_icon(name, False), 0, (6 + i) * 16)
    write_bmp(os.path.join(graphics, 'ability_icons.bmp'), icons, P.UI, bpp=4)
    write_json(os.path.join(graphics, 'ability_icons.json'), {'type': 'sprite', 'height': 16})

    difficulties = ['apprentice', 'knight', 'sorcerer', 'grandmaster']
    diff = Canvas(16, 16 * 4)
    for i, name in enumerate(difficulties):
        diff.blit(difficulty_icon(name), 0, i * 16)
    write_bmp(os.path.join(graphics, 'difficulty_icons.bmp'), diff, P.UI, bpp=4)
    write_json(os.path.join(graphics, 'difficulty_icons.json'), {'type': 'sprite', 'height': 16})

    minis = Canvas(8, 8 * 6)
    for i, name in enumerate(['pawn', 'knight', 'bishop', 'rook', 'queen', 'king']):
        minis.blit(mini_piece(name), 0, i * 8)
    write_bmp(os.path.join(graphics, 'mini_pieces.bmp'), minis, P.ARMY_WHITE, bpp=4)
    write_json(os.path.join(graphics, 'mini_pieces.json'), {'type': 'sprite', 'height': 8})

    # Announcement banner (achievements, notices): 32x16 frames - left cap, middle, right cap.
    banner = Canvas(32, 48)
    for frame in range(3):
        top = frame * 16
        for y in range(16):
            for x in range(32):
                inside = 2 <= y <= 13
                if not inside:
                    color = 1 if y in (0, 15) else 7
                else:
                    color = 3 if (x + y) % 2 else 2
                    if y == 2 or y == 13:
                        color = 8 if y == 2 else 6
                banner.set(x, top + y, color)
        if frame == 0:
            for y in range(16):
                for x in range(6):
                    banner.set(x, top + y, 0)
            for y in range(1, 15):
                banner.set(6, top + y, 1)
                banner.set(7, top + y, 7)
            for y in (0, 15):
                banner.set(6, top + y, 0)
            # gem on the left cap
            for (gx, gy, color) in ((11, 6, 13), (12, 6, 9), (10, 7, 14), (11, 7, 13), (12, 7, 13), (13, 7, 14),
                                    (10, 8, 14), (11, 8, 14), (12, 8, 13), (13, 8, 14), (11, 9, 14), (12, 9, 14)):
                banner.set(gx, top + gy, color)
        if frame == 2:
            for y in range(16):
                for x in range(26, 32):
                    banner.set(x, top + y, 0)
            for y in range(1, 15):
                banner.set(25, top + y, 1)
                banner.set(24, top + y, 7)
            for y in (0, 15):
                banner.set(25, top + y, 0)
    write_bmp(os.path.join(graphics, 'banner.bmp'), banner, P.UI, bpp=4)
    write_json(os.path.join(graphics, 'banner.json'), {'type': 'sprite', 'height': 16})

    lines = ['// Generated by assets_source/gen_ui.py - do not edit.', '#ifndef CL_PANEL_TILES_H',
             '#define CL_PANEL_TILES_H', '', 'namespace cl::panel_tiles', '{', '']
    for name, index in panel.names.items():
        lines.append('    constexpr int %s = %d;' % (name, index))
    lines += ['}', '', '#endif', '']
    with open(os.path.join(include, 'cl_panel_tiles.h'), 'w', newline='\n') as f:
        f.write('\n'.join(lines))

    # preview: a framed window with gems and icons
    prev = Canvas(160, 80, fill=PM)
    win_w, win_h = 18, 8
    t = panel.tiles
    for ty in range(win_h):
        for tx in range(win_w):
            name = 'win_fill'
            fh = fv = False
            if ty == 0 or ty == win_h - 1:
                name = 'win_top'
                fv = ty == win_h - 1
            if tx == 0 or tx == win_w - 1:
                name = 'win_left'
                fh = tx == win_w - 1
            if (tx in (0, win_w - 1)) and (ty in (0, win_h - 1)):
                name = 'win_corner'
                fh = tx == win_w - 1
                fv = ty == win_h - 1
            prev.blit(t[panel.names[name]], 4 + tx * 8, 4 + ty * 8, transparent=-1, flip_h=fh, flip_v=fv)
    for i in range(6):
        prev.blit(t[panel.names['gem_full' if i < 4 else 'gem_empty']], 14 + i * 9, 14)
    for i in range(6):
        prev.blit(icons.crop(0, i * 16, 16, 16), 14 + i * 18, 26)
        prev.blit(icons.crop(0, (6 + i) * 16, 16, 16), 14 + i * 18, 44)
    for i in range(4):
        prev.blit(diff.crop(0, i * 16, 16, 16), 124, 12 + i * 16)
    write_png(os.path.join(root, 'assets_source', 'previews', 'ui.png'), prev, P.UI, scale=3, transparent_color=None)


if __name__ == '__main__':
    generate(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    print('ui generated')
