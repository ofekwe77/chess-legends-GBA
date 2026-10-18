"""Chessboard graphics: square / frame tiles (one tile set, recolored by eight theme palettes), the marker overlay
tile set and the animated board cursor.

Tile sets are emitted as Butano regular_bg_tiles items; the game builds the board map in RAM. The tile order is
fixed and mirrored by constants in include/cl_board_tiles.h (generated).
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gfx'))

from canvas import Canvas  # noqa: E402
from imageio import write_bmp, write_json, write_png_rgb  # noqa: E402
import palettes as P  # noqa: E402

# Board palette roles (see palettes.board_theme).
F_INK, F0, F1, F2, F_HI = 1, 2, 3, 4, 5
L0, L1, L2 = 6, 7, 8
D0, D1, D2 = 9, 10, 11
SEL, LAST, CHECK, COORD = 12, 13, 14, 15

# ------------------------------------------------------------------------------------------ square art


def marble_square(base0, base1, base2, variant):
    """16x16 square: carved bevel and deliberate marble veins (two hand-placed vein layouts)."""
    c = Canvas(16, 16, fill=base1)
    # soft clouding: a few lighter blotches (fixed positions per variant)
    blotches = [((4, 4), (11, 9), (6, 12)), ((10, 3), (4, 9), (12, 13))][variant]
    for bx, by in blotches:
        for y in range(-2, 3):
            for x in range(-2, 3):
                if x * x + y * y <= 4 and (x + y + bx + by) % 3 != 0:
                    c.set(bx + x, by + y, base2)
    # veins: one soft, broken vein per square (dashed so the texture stays subtle at 1x)
    veins = [
        [(1, 5), (4, 6), (7, 9), (10, 10), (14, 13)],
        [(2, 13), (5, 11), (8, 9), (10, 6), (13, 4)],
    ][variant]
    vein = Canvas(16, 16)
    for i in range(len(veins) - 1):
        vein.line(veins[i][0], veins[i][1], veins[i + 1][0], veins[i + 1][1], 1)
    for y in range(16):
        for x in range(16):
            if vein.get(x, y) and (x * 7 + y * 3) % 5 != 0:
                c.set(x, y, base0)
    # carved bevel: light top / left, dark bottom / right
    for i in range(16):
        c.set(i, 0, base2)
        c.set(0, i, base2)
        c.set(i, 15, base0)
        c.set(15, i, base0)
    c.set(0, 15, base1)
    c.set(15, 0, base1)
    return c


def highlighted(square, color, width, glow_color=None):
    c = square.copy()
    for y in range(16):
        for x in range(16):
            edge = min(x, y, 15 - x, 15 - y)
            if edge < width:
                c.set(x, y, color)
            elif glow_color is not None and edge < width + 2 and (x + y) % 2 == 0:
                c.set(x, y, glow_color)
    # corner jewels
    for x, y in ((1, 1), (14, 1), (1, 14), (14, 14)):
        c.set(x, y, F_HI)
    return c


def check_square(square):
    """Red glow from the edges inwards (dithered), for the king in check."""
    c = square.copy()
    for y in range(16):
        for x in range(16):
            edge = min(x, y, 15 - x, 15 - y)
            if edge == 0 or edge == 1:
                c.set(x, y, CHECK)
            elif edge == 2 and (x + y) % 2 == 0:
                c.set(x, y, CHECK)
            elif edge == 3 and (x + y) % 4 == 0:
                c.set(x, y, CHECK)
    return c


# ------------------------------------------------------------------------------------------ frame art

TINY_DIGITS = {
    '1': ['.#.', '##.', '.#.', '.#.', '###'], '2': ['##.', '..#', '.#.', '#..', '###'],
    '3': ['##.', '..#', '.#.', '..#', '##.'], '4': ['#.#', '#.#', '###', '..#', '..#'],
    '5': ['###', '#..', '##.', '..#', '##.'], '6': ['.#.', '#..', '##.', '#.#', '.#.'],
    '7': ['###', '..#', '.#.', '.#.', '.#.'], '8': ['.#.', '#.#', '.#.', '#.#', '.#.'],
    'a': ['...', '.##', '#.#', '#.#', '.##'], 'b': ['#..', '##.', '#.#', '#.#', '##.'],
    'c': ['...', '.##', '#..', '#..', '.##'], 'd': ['..#', '.##', '#.#', '#.#', '.##'],
    'e': ['...', '.#.', '###', '#..', '.##'], 'f': ['.##', '#..', '##.', '#..', '#..'],
    'g': ['...', '.##', '#.#', '.##', '##.'], 'h': ['#..', '##.', '#.#', '#.#', '#.#'],
}


def frame_tile(kind):
    """8x8 frame tiles. kind: 'h' horizontal edge (top), 'hb' bottom edge, 'v' left edge, 'vr' right edge,
    'corner' (top-left; flipped by the map for other corners)."""
    c = Canvas(8, 8, fill=F1)
    if kind in ('h', 'hb'):
        # metal band with a bevel and a central groove
        for x in range(8):
            c.set(x, 0, F_INK if kind == 'h' else F0)
            c.set(x, 1, F2)
            c.set(x, 2, F_HI if x % 4 == 1 else F2)
            c.set(x, 3, F1)
            c.set(x, 4, F0)
            c.set(x, 5, F1)
            c.set(x, 6, F0)
            c.set(x, 7, F_INK if kind == 'hb' else F0)
        # rivets every 8 pixels
        c.set(4, 4, F2)
        c.set(4, 5, F_HI)
    elif kind in ('v', 'vr'):
        for y in range(8):
            c.set(0, y, F_INK if kind == 'v' else F0)
            c.set(1, y, F2)
            c.set(2, y, F_HI if y % 4 == 1 else F2)
            c.set(3, y, F1)
            c.set(4, y, F0)
            c.set(5, y, F1)
            c.set(6, y, F0)
            c.set(7, y, F_INK if kind == 'vr' else F0)
        c.set(4, 4, F2)
        c.set(5, 4, F_HI)
    elif kind == 'corner':
        c.fill(F1)
        c.rect(0, 0, 8, 1, F_INK)
        c.rect(0, 0, 1, 8, F_INK)
        # jewel
        c.circle(4.5, 4.5, 3.0, F0)
        c.circle(4.5, 4.5, 2.2, SEL)
        c.set(3, 3, F_HI)
        c.set(4, 3, F_HI)
        c.set(6, 6, F0)
    return c


def label_tiles(base_kind, text):
    """Coordinate label drawn across two frame tiles so it is centred on its 16-pixel rank / file.
    Files: (left, right) tiles of the bottom frame. Ranks: (top, bottom) tiles of the left frame."""
    rows = TINY_DIGITS[text]
    if base_kind == 'hb':
        c = Canvas(16, 8)
        c.blit(frame_tile('hb'), 0, 0, transparent=-1)
        c.blit(frame_tile('hb'), 8, 0, transparent=-1)
        ox, oy = 6, 1
    else:
        c = Canvas(8, 16)
        c.blit(frame_tile('v'), 0, 0, transparent=-1)
        c.blit(frame_tile('v'), 0, 8, transparent=-1)
        ox, oy = 2, 5
    for y in range(5):
        for x in range(3):
            if rows[y][x] == '#':
                c.set(ox + x + 1, oy + y + 1, F_INK)
    for y in range(5):
        for x in range(3):
            if rows[y][x] == '#':
                c.set(ox + x, oy + y, COORD)
    if base_kind == 'hb':
        return c.crop(0, 0, 8, 8), c.crop(8, 0, 8, 8)
    return c.crop(0, 0, 8, 8), c.crop(0, 8, 8, 8)


# ------------------------------------------------------------------------------------------ tile set

class TileSet:
    def __init__(self):
        self.tiles = [Canvas(8, 8)]   # tile 0: transparent / empty
        self.names = {'empty': 0}

    def add(self, name, canvas8):
        self.names[name] = len(self.tiles)
        self.tiles.append(canvas8)
        return self.names[name]

    def add_square(self, name, canvas16):
        for i, (ox, oy) in enumerate(((0, 0), (8, 0), (0, 8), (8, 8))):
            self.add('%s.q%d' % (name, i), canvas16.crop(ox, oy, 8, 8))
        return self.names[name + '.q0']

    def sheet(self):
        out = Canvas(8, 8 * len(self.tiles))
        for i, t in enumerate(self.tiles):
            out.blit(t, 0, i * 8, transparent=-1)
        return out


def build_board_tiles():
    ts = TileSet()
    light = [marble_square(L0, L1, L2, v) for v in (0, 1)]
    dark = [marble_square(D0, D1, D2, v) for v in (0, 1)]
    ts.add_square('light_a', light[0])
    ts.add_square('light_b', light[1])
    ts.add_square('dark_a', dark[0])
    ts.add_square('dark_b', dark[1])
    ts.add_square('light_sel', highlighted(light[0], SEL, 2, L2))
    ts.add_square('dark_sel', highlighted(dark[0], SEL, 2, D2))
    ts.add_square('light_last', highlighted(light[0], LAST, 1))
    ts.add_square('dark_last', highlighted(dark[0], LAST, 1))
    ts.add_square('light_check', check_square(light[0]))
    ts.add_square('dark_check', check_square(dark[0]))
    ts.add('frame_top', frame_tile('h'))
    ts.add('frame_bottom', frame_tile('hb'))
    ts.add('frame_left', frame_tile('v'))
    ts.add('frame_right', frame_tile('vr'))
    ts.add('frame_corner', frame_tile('corner'))
    for f in 'abcdefgh':
        first, second = label_tiles('hb', f)
        ts.add('file_' + f, first)
        ts.add('file_' + f + '_2', second)
    for r in '12345678':
        first, second = label_tiles('v', r)
        ts.add('rank_' + r, first)
        ts.add('rank_' + r + '_2', second)
    return ts


# ------------------------------------------------------------------------------------------ markers
M_INK, M_DOT, M_DOT_HI = 1, 2, 3
M_CAP0, M_CAP1, M_CAP2 = 4, 5, 6
M_MAG0, M_MAG1, M_MAG2, M_MAG3 = 7, 8, 9, 10
M_GOLD0, M_GOLD1, M_GOLD2, M_ESC, M_WHITE = 11, 12, 13, 14, 15


def marker_move():
    c = Canvas(16, 16)
    c.circle(8, 8, 3.2, M_INK)
    c.circle(8, 8, 2.4, M_DOT)
    c.set(7, 6, M_DOT_HI)
    c.set(6, 7, M_DOT_HI)
    return c


def marker_capture():
    c = Canvas(16, 16)
    for sx, sy in ((0, 0), (1, 0), (0, 1), (1, 1)):
        x0 = 0 if sx == 0 else 15
        y0 = 0 if sy == 0 else 15
        dx = 1 if sx == 0 else -1
        dy = 1 if sy == 0 else -1
        for i in range(5):
            c.set(x0 + dx * i, y0, M_CAP1)
            c.set(x0, y0 + dy * i, M_CAP1)
            c.set(x0 + dx * i, y0 + dy, M_CAP0 if i else M_CAP1)
            c.set(x0 + dx, y0 + dy * i, M_CAP0 if i else M_CAP1)
        c.set(x0, y0, M_CAP2)
    return c


def marker_magic():
    """Rune ring for magical destinations."""
    c = Canvas(16, 16)
    c.circle(8, 8, 6.0, M_MAG0, filled=False)
    c.circle(8, 8, 5.0, M_MAG1, filled=False)
    for x, y in ((8, 2), (8, 13), (2, 8), (13, 8)):
        c.set(x, y, M_MAG3)
        c.set(x - 1 if x != 8 else x, y - 1 if y != 8 else y, M_MAG2)
    c.circle(8, 8, 1.6, M_MAG2)
    c.set(8, 8, M_MAG3)
    return c


def marker_special():
    """Gold four-point star (Royal Escape landing squares, Castle Swap partners)."""
    c = Canvas(16, 16)
    pts = [(8, 1), (10, 6), (15, 8), (10, 10), (8, 15), (6, 10), (1, 8), (6, 6)]
    c.polygon([(x + 0.0, y + 0.0) for x, y in pts], M_GOLD1)
    c.outline(M_INK)
    c.set(8, 5, M_GOLD2)
    c.set(7, 7, M_GOLD2)
    c.set(8, 8, M_WHITE)
    return c


def marker_target():
    """Violet frame: the ally that Royal Command will move / the piece selected for a spell."""
    c = Canvas(16, 16)
    for i in range(16):
        for w in (0, 1):
            if (i + w) % 3 != 2:
                c.set(i, w, M_MAG1)
                c.set(i, 15 - w, M_MAG1)
                c.set(w, i, M_MAG1)
                c.set(15 - w, i, M_MAG1)
    for x, y in ((0, 0), (15, 0), (0, 15), (15, 15)):
        c.set(x, y, M_MAG3)
    return c


def marker_hint():
    """Small cyan dot: preview of where a magical action could go (inspection mode)."""
    c = Canvas(16, 16)
    c.circle(8, 8, 2.2, M_INK)
    c.circle(8, 8, 1.5, M_MAG2)
    return c


def build_marker_tiles():
    ts = TileSet()
    ts.add_square('move', marker_move())
    ts.add_square('capture', marker_capture())
    ts.add_square('magic', marker_magic())
    ts.add_square('special', marker_special())
    ts.add_square('target', marker_target())
    ts.add_square('hint', marker_hint())
    return ts


# ------------------------------------------------------------------------------------------ cursor

def build_cursor():
    """32x32 cursor, 4 frames: gold brackets around a 16x16 square, breathing outwards."""
    frames = []
    for spread in (0, 1, 2, 1):
        c = Canvas(32, 32)
        lo = 7 - spread
        hi = 24 + spread
        length = 5
        for corner_x, corner_y, dx, dy in ((lo, lo, 1, 1), (hi, lo, -1, 1), (lo, hi, 1, -1), (hi, hi, -1, -1)):
            for i in range(length):
                c.set(corner_x + dx * i, corner_y, 8)
                c.set(corner_x, corner_y + dy * i, 8)
                c.set(corner_x + dx * i, corner_y + dy, 7)
                c.set(corner_x + dx, corner_y + dy * i, 7)
            c.set(corner_x, corner_y, 9)
        c.outline(1)
        frames.append(c)
    sheet = Canvas(32, 32 * len(frames))
    for i, f in enumerate(frames):
        sheet.blit(f, 0, i * 32)
    return sheet


# ------------------------------------------------------------------------------------------ output

def write_tiles_header(path, board, markers):
    lines = ['// Generated by assets_source/gen_board.py - do not edit.', '#ifndef CL_BOARD_TILES_H',
             '#define CL_BOARD_TILES_H', '', 'namespace cl::board_tiles', '{', '']
    for name, index in board.names.items():
        if name.endswith(('.q1', '.q2', '.q3')):
            continue
        lines.append('    constexpr int %s = %d;' % (name.replace('.q0', ''), index))
    lines += ['}', '', 'namespace cl::marker_tiles', '{', '']
    for name, index in markers.names.items():
        if name.endswith(('.q1', '.q2', '.q3')):
            continue
        lines.append('    constexpr int %s = %d;' % (name.replace('.q0', ''), index))
    lines += ['}', '', 'namespace cl::board_themes', '{', '']
    for i, name in enumerate(P.BOARD_THEME_ORDER):
        lines.append('    constexpr int %s = %d;' % (name, i))
    lines.append('    constexpr int count = %d;' % len(P.BOARD_THEME_ORDER))
    lines += ['}', '', '#endif', '']
    with open(path, 'w', newline='\n') as f:
        f.write('\n'.join(lines))


def generate(root):
    graphics = os.path.join(root, 'graphics')
    include = os.path.join(root, 'include')
    board = build_board_tiles()
    markers = build_marker_tiles()

    write_bmp(os.path.join(graphics, 'board_tiles.bmp'), board.sheet(), P.BOARD_THEMES['royal'], bpp=4)
    write_json(os.path.join(graphics, 'board_tiles.json'), {'type': 'regular_bg_tiles', 'bpp_mode': 'bpp_4'})
    swatch = Canvas(8, 8)
    for i in range(16):
        swatch.set(i % 8, i // 8, i)
    for name in P.BOARD_THEME_ORDER:
        write_bmp(os.path.join(graphics, 'board_palette_%s.bmp' % name), swatch, P.BOARD_THEMES[name], bpp=4)
        write_json(os.path.join(graphics, 'board_palette_%s.json' % name),
                   {'type': 'bg_palette', 'bpp_mode': 'bpp_4'})

    write_bmp(os.path.join(graphics, 'marker_tiles.bmp'), markers.sheet(), P.MARKERS, bpp=4)
    write_json(os.path.join(graphics, 'marker_tiles.json'), {'type': 'regular_bg_tiles', 'bpp_mode': 'bpp_4'})
    write_bmp(os.path.join(graphics, 'marker_palette.bmp'), swatch, P.MARKERS, bpp=4)
    write_json(os.path.join(graphics, 'marker_palette.json'), {'type': 'bg_palette', 'bpp_mode': 'bpp_4'})

    cursor = build_cursor()
    write_bmp(os.path.join(graphics, 'board_cursor.bmp'), cursor, P.UI, bpp=4)
    write_json(os.path.join(graphics, 'board_cursor.json'), {'type': 'sprite', 'height': 32})

    write_tiles_header(os.path.join(include, 'cl_board_tiles.h'), board, markers)
    preview_all(root, board, markers)


def preview_all(root, board, markers):
    """Composite: framed board with highlights and markers for every theme (2x)."""
    import gen_pieces
    pieces = gen_pieces.build_sheet()
    themes = P.BOARD_THEME_ORDER
    board_px = 8 * 16 + 16
    cols = 4
    width = cols * board_px
    height = ((len(themes) + cols - 1) // cols) * board_px
    rows = [[(0, 0, 0)] * width for _ in range(height)]
    layout = {
        (4, 3): 'sel', (4, 4): 'last', (4, 5): 'last', (3, 6): 'check',
    }
    marker_squares = {(5, 5): 'move', (6, 5): 'move', (3, 4): 'capture', (5, 3): 'magic', (6, 2): 'special',
                      (2, 2): 'target'}
    setup = 'rnbqkbnr/pppp1ppp/8/4p3/4P3/5N2/PPPP1PPP/RNBQKB1R'
    placement = {}
    r = 7
    for rank in setup.split('/'):
        f = 0
        for ch in rank:
            if ch.isdigit():
                f += int(ch)
            else:
                placement[(f, r)] = ch
                f += 1
        r -= 1
    order = 'pnbrqk'
    for t, theme in enumerate(themes):
        pal = P.BOARD_THEMES[theme]
        ox = (t % cols) * board_px
        oy = (t // cols) * board_px
        canvas = Canvas(board_px, board_px)
        # frame
        for i in range(board_px // 8):
            canvas.blit(board.tiles[board.names['frame_top']], i * 8, 0, transparent=-1)
            canvas.blit(board.tiles[board.names['frame_bottom']], i * 8, board_px - 8, transparent=-1)
            canvas.blit(board.tiles[board.names['frame_left']], 0, i * 8, transparent=-1)
            canvas.blit(board.tiles[board.names['frame_right']], board_px - 8, i * 8, transparent=-1)
        for f in range(8):
            name = 'file_' + 'abcdefgh'[f]
            canvas.blit(board.tiles[board.names[name]], 8 + f * 16, board_px - 8, transparent=-1)
            canvas.blit(board.tiles[board.names[name + '_2']], 16 + f * 16, board_px - 8, transparent=-1)
        for rr in range(8):
            name = 'rank_' + str(rr + 1)
            canvas.blit(board.tiles[board.names[name]], 0, 8 + (7 - rr) * 16, transparent=-1)
            canvas.blit(board.tiles[board.names[name + '_2']], 0, 16 + (7 - rr) * 16, transparent=-1)
        corner = board.tiles[board.names['frame_corner']]
        canvas.blit(corner, 0, 0, transparent=-1)
        canvas.blit(corner, board_px - 8, 0, transparent=-1, flip_h=True)
        canvas.blit(corner, 0, board_px - 8, transparent=-1, flip_v=True)
        canvas.blit(corner, board_px - 8, board_px - 8, transparent=-1, flip_h=True, flip_v=True)
        for rr in range(8):
            for f in range(8):
                light = (f + rr) % 2 == 1
                kind = layout.get((f, rr))
                if kind:
                    name = ('light_' if light else 'dark_') + kind
                else:
                    name = ('light_' if light else 'dark_') + ('a' if (f * 3 + rr) % 2 == 0 else 'b')
                base = board.names[name + '.q0']
                for i, (sx, sy) in enumerate(((0, 0), (8, 0), (0, 8), (8, 8))):
                    canvas.blit(board.tiles[base + i], 8 + f * 16 + sx, 8 + (7 - rr) * 16 + sy, transparent=-1)
        rgb = [[pal[canvas.get(x, y)] for x in range(board_px)] for y in range(board_px)]
        # markers then pieces
        for (f, rr), kind in marker_squares.items():
            base = markers.names[kind + '.q0']
            for i, (sx, sy) in enumerate(((0, 0), (8, 0), (0, 8), (8, 8))):
                tile = markers.tiles[base + i]
                for y in range(8):
                    for x in range(8):
                        idx = tile.get(x, y)
                        if idx:
                            rgb[8 + (7 - rr) * 16 + sy + y][8 + f * 16 + sx + x] = P.MARKERS[idx]
        for (f, rr), ch in placement.items():
            army = P.ARMY_WHITE if ch.isupper() else P.ARMY_BLACK
            kind = order.index(ch.lower())
            for y in range(16):
                for x in range(16):
                    idx = pieces.get(x, kind * 16 + y)
                    if idx:
                        rgb[8 + (7 - rr) * 16 + y][8 + f * 16 + x] = army[idx]
        for y in range(board_px):
            for x in range(board_px):
                rows[oy + y][ox + x] = rgb[y][x]
    write_png_rgb(os.path.join(root, 'assets_source', 'previews', 'boards.png'), width, height, rows, scale=2)


if __name__ == '__main__':
    sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
    generate(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    print('board generated')
