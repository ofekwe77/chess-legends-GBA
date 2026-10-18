"""Board piece sprites (16x16, one sheet shared by every army through palette swaps).

Pawn, bishop, rook, queen and king are "lathe" pieces: a per-row half-width profile is filled, shaded from the
top-left light source, outlined, then decorated with hand-placed details. The knight is an asymmetric
hand-drawn template.
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gfx'))

from canvas import Canvas  # noqa: E402
from imageio import write_bmp, write_json, write_png  # noqa: E402
import palettes as P  # noqa: E402

R = P.PIECE_ROLES
O = R['outline']
B0, B1, B2, B3 = R['body0'], R['body1'], R['body2'], R['body3']
A0, A1, A2 = R['acc0'], R['acc1'], R['acc2']
S0, S1 = R['sec0'], R['sec1']
DET, GEM, RIM = R['detail'], R['gem'], R['rim']

SIZE = 16
ORDER = ['pawn', 'knight', 'bishop', 'rook', 'queen', 'king']


def shade(t, band):
    """Interior color by horizontal position t (0 = left edge, 1 = right edge)."""
    if band:
        return A2 if t < 0.22 else (A1 if t < 0.7 else A0)
    if t < 0.2:
        return B3
    if t < 0.5:
        return B2
    if t < 0.8:
        return B1
    return B0


def lathe(profile, bands=(), masks=None):
    """profile: {row: half_width}; bands: rows painted with the accent ramp; masks: {row: set(columns) to clear}."""
    c = Canvas(SIZE, SIZE)
    for row, half in profile.items():
        left = 8 - half
        right = 7 + half
        for x in range(left, right + 1):
            if masks and row in masks and x in masks[row]:
                continue
            t = (x - left) / max(1, right - left)
            c.set(x, row, shade(t, row in bands))
    # top highlight: first row of each vertical run gets a lighter tone on its left half
    for x in range(SIZE):
        for y in range(SIZE):
            if c.get(x, y) and not c.get(x, y - 1) and c.get(x, y) in (B1, B2):
                c.set(x, y, B3 if x < 8 else B2)
    c.outline(O)
    return c


def make_pawn():
    profile = {2: 1, 3: 2, 4: 2, 5: 2, 6: 1, 7: 2, 8: 1, 9: 1, 10: 2, 11: 3, 12: 4, 13: 4}
    c = lathe(profile, bands=(7, 12))
    # helmet visor slit and crest
    c.set(7, 4, DET)
    c.set(8, 4, DET)
    c.set(7, 1, A1)
    c.set(8, 1, A0)
    c.set(13, 13, O)
    return c


def make_bishop():
    profile = {2: 1, 3: 2, 4: 3, 5: 3, 6: 3, 7: 2, 8: 1, 9: 2, 10: 1, 11: 2, 12: 3, 13: 4, 14: 4}
    c = lathe(profile, bands=(9, 13))
    # orb on top of the mitre
    c.set(7, 0, O)
    c.set(8, 0, O)
    c.set(7, 1, A2)
    c.set(8, 1, A1)
    # mitre slit (diagonal) and gem
    for x, y in ((9, 3), (8, 4), (7, 5)):
        c.set(x, y, DET)
    c.set(7, 7, S1)
    c.set(8, 7, S0)
    return c


def make_rook():
    profile = {2: 5, 3: 5, 4: 4, 5: 3, 6: 3, 7: 3, 8: 3, 9: 3, 10: 3, 11: 4, 12: 5, 13: 5}
    masks = {2: {5, 6, 9, 10}}
    c = lathe(profile, bands=(4, 12), masks=masks)
    # arrow slit and blue banner
    c.set(7, 7, DET)
    c.set(7, 8, DET)
    c.set(8, 6, S1)
    c.set(8, 7, S0)
    c.set(8, 8, S0)
    return c


def make_queen():
    profile = {3: 4, 4: 3, 5: 2, 6: 2, 7: 2, 8: 1, 9: 3, 10: 2, 11: 3, 12: 4, 13: 5, 14: 5}
    c = lathe(profile, bands=(3, 9, 13))
    # five crown points with gems
    for x in (4, 6, 9, 11):
        c.set(x, 2, A1)
        c.set(x, 1, O)
    c.set(7, 1, GEM)
    c.set(8, 1, GEM)
    c.set(7, 0, O)
    c.set(8, 0, O)
    for x in (3, 5, 10, 12):
        c.set(x, 2, O)
    c.set(7, 2, A2)
    c.set(8, 2, A1)
    c.set(7, 6, GEM)
    return c


def make_king():
    profile = {4: 4, 5: 4, 6: 3, 7: 2, 8: 2, 9: 1, 10: 3, 11: 2, 12: 3, 13: 4, 14: 4}
    c = lathe(profile, bands=(4, 10, 14))
    # cross
    for y in (0, 1, 2, 3):
        c.set(7, y, A2 if y != 1 else A2)
        c.set(8, y, A1)
    c.set(6, 1, A2)
    c.set(9, 1, A0)
    c.outline(O)
    # crown arches
    c.set(5, 5, B3)
    c.set(10, 5, B1)
    c.set(7, 7, GEM)
    c.set(8, 7, GEM)
    return c


KNIGHT = [
    "................",
    "................",
    "......oo........",
    ".....o43ooo.....",
    "....o444333oo...",
    "...o44d4332bbo..",
    "..o444433322bo..",
    ".o4444433322b2o.",
    "o44oo4443322b2o.",
    "o32o.o44332b21o.",
    ".oo..o433221b1o.",
    "....o4433221b1o.",
    "...o44332221110.",
    "..oabbbbbbbbbaao",
    "..ooooooooooooo.",
    "................",
]

KNIGHT_MAP = {'o': O, '1': B0, '2': B1, '3': B2, '4': B3, 'a': A0, 'b': A1, 'c': A2, 'd': DET}


def make_knight():
    rows = [r.replace('0', 'o') for r in KNIGHT]
    return Canvas.from_ascii(rows, KNIGHT_MAP)


MAKERS = {
    'pawn': make_pawn, 'knight': make_knight, 'bishop': make_bishop,
    'rook': make_rook, 'queen': make_queen, 'king': make_king,
}


def build_sheet():
    sheet = Canvas(SIZE, SIZE * len(ORDER))
    for i, name in enumerate(ORDER):
        sheet.blit(MAKERS[name](), 0, i * SIZE)
    return sheet


def preview(sheet, path):
    """Both armies (and two factions) on light and dark squares of the default board theme, 4x."""
    board = P.BOARD_THEMES['royal']
    armies = [P.ARMY_WHITE, P.ARMY_BLACK, P.FACTIONS_DARK['inferno'], P.FACTIONS_DARK['frost']]
    cols = len(ORDER) * 2
    width = cols * SIZE
    height = len(armies) * SIZE
    rows = []
    for y in range(height):
        row = []
        army = armies[y // SIZE]
        for x in range(width):
            square_light = ((x // SIZE) + (y // SIZE)) % 2 == 0
            piece = ORDER[(x // SIZE) // 2]
            idx = sheet.get(x % SIZE, ORDER.index(piece) * SIZE + y % SIZE)
            if idx:
                row.append(army[idx])
            else:
                row.append(board[7] if square_light else board[10])
        rows.append(row)
    from imageio import write_png_rgb
    write_png_rgb(path, width, height, rows, scale=4)


def generate(root):
    sheet = build_sheet()
    graphics = os.path.join(root, 'graphics')
    write_bmp(os.path.join(graphics, 'pieces.bmp'), sheet, P.ARMY_WHITE, bpp=4)
    write_json(os.path.join(graphics, 'pieces.json'), {'type': 'sprite', 'height': SIZE})

    # Army palettes as sprite palette items.
    swatch = Canvas(8, 8)
    for i in range(16):
        swatch.set(i % 8, i // 8, i)
    palettes = {'army_white': P.ARMY_WHITE, 'army_black': P.ARMY_BLACK}
    for name, pal in P.FACTIONS_DARK.items():
        palettes['army_' + name] = pal
    for name, pal in palettes.items():
        write_bmp(os.path.join(graphics, name + '.bmp'), swatch, pal, bpp=4)
        write_json(os.path.join(graphics, name + '.json'), {'type': 'sprite_palette'})

    preview(sheet, os.path.join(root, 'assets_source', 'previews', 'pieces.png'))
    return sheet


if __name__ == '__main__':
    generate(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    print('pieces generated')
