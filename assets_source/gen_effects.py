"""Effect sprites of CHESS LEGENDS: ARCANE WARS: particles (cinematics, spells) and the world-map marker.

Particles are 8x8 frames sharing one 16-color palette; the map marker is a 16x16 banner with three states.
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gfx'))

from canvas import Canvas  # noqa: E402
from imageio import write_bmp, write_json, write_png  # noqa: E402
import palettes as P  # noqa: E402

# Shared particle palette
PARTICLE_PAL = [P.TRANSPARENT,
                (40, 90, 40), (90, 170, 70), (170, 230, 120),       # 1-3 leaf greens
                (160, 40, 20), (240, 120, 30), (255, 220, 120),      # 4-6 ember
                (150, 190, 240), (255, 255, 255),                    # 7-8 frost / white
                (120, 60, 200), (200, 150, 255),                     # 9-10 arcane violet
                (90, 150, 220),                                      # 11 rain blue
                (200, 150, 40), (255, 230, 110),                     # 12-13 gold
                (30, 20, 50), (110, 230, 250)]                       # 14 ink, 15 cyan

FRAMES = ['leaf_a', 'leaf_b', 'ember_a', 'ember_b', 'snow_a', 'snow_b', 'rune_a', 'rune_b', 'rain', 'spark_a',
          'spark_b', 'fragment', 'star', 'orb', 'smoke', 'shard']

TEMPLATES = {
    'leaf_a': ['........', '.....22.', '...2233.', '..22332.', '.22322..', '.2322...', '12......', '1.......'],
    'leaf_b': ['........', '..33....', '.23322..', '.223322.', '..22322.', '...2221.', '......1.', '........'],
    'ember_a': ['........', '...6....', '..656...', '..454...', '...4....', '........', '........', '........'],
    'ember_b': ['........', '........', '...6....', '..66....', '..54....', '...4....', '........', '........'],
    'snow_a': ['...8....', '.8.7.8..', '..777...', '8777778.', '..777...', '.8.7.8..', '...8....', '........'],
    'snow_b': ['........', '...7....', '..787...', '.78887..', '..787...', '...7....', '........', '........'],
    'rune_a': ['.99999..', '.9...9..', '.9.a.9..', '.9aaa9..', '.9.a.9..', '.9...9..', '.99999..', '........'],
    'rune_b': ['..aaa...', '.a...a..', 'a..9..a.', 'a.999.a.', 'a..9..a.', '.a...a..', '..aaa...', '........'],
    'rain': ['......b.', '.....b..', '.....b..', '....b...', '....b...', '...b....', '........', '........'],
    'spark_a': ['...d....', '...d....', '.ddddd..', '...d....', '...d....', '........', '........', '........'],
    'spark_b': ['........', '..d.d...', '...d....', '..d.d...', '........', '........', '........', '........'],
    'fragment': ['........', '..c.c...', '.cdcdc..', '.ccccc..', '.ccccc..', '........', '........', '........'],
    'star': ['...8....', '...8....', '.88888..', '...8....', '...8....', '........', '........', '........'],
    'orb': ['..ff....', '.f88f...', '.f88f...', '..ff....', '........', '........', '........', '........'],
    'smoke': ['..ee....', '.eeee...', 'eeeeee..', '.eeee...', '........', '........', '........', '........'],
    'shard': ['...8....', '..787...', '..777...', '.77777..', '..777...', '...7....', '........', '........'],
}

DIGITS = {'.': 0, '1': 1, '2': 2, '3': 3, '4': 4, '5': 5, '6': 6, '7': 7, '8': 8, '9': 9, 'a': 10, 'b': 11,
          'c': 12, 'd': 13, 'e': 14, 'f': 15}


def particles():
    sheet = Canvas(8, 8 * len(FRAMES))
    for i, name in enumerate(FRAMES):
        rows = TEMPLATES[name]
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                if ch != '.':
                    sheet.set(x, i * 8 + y, DIGITS[ch])
    return sheet


def map_marker():
    """16x16 banner on a pole: frame 0 open kingdom, 1 restored (gold), 2 locked (grey)."""
    sheet = Canvas(16, 48)
    colors = ((9, 10), (12, 13), (14, 7))
    for f, (dark, light) in enumerate(colors):
        top = f * 16
        sheet.vline(4, top + 1, top + 15, 14)
        sheet.vline(5, top + 1, top + 15, 7 if f != 2 else 14)
        sheet.polygon([(6, top + 2), (14, top + 4), (6, top + 9)], light)
        sheet.polygon([(6, top + 6), (14, top + 4), (6, top + 9)], dark)
        sheet.set(4, top, 13 if f == 1 else 8)
        sheet.set(5, top, 13 if f == 1 else 8)
        sheet.hline(2, 7, top + 15, 14)
        if f == 1:
            sheet.set(9, top + 5, 8)
    return sheet


SIGILS = {
    # 16x16 kingdom sigils (cinematics): leaf, flame, snowflake, rune, lightning, crown
    'leaf': ['................', '...........333..', '.........33322..', '.......3332222..', '......332222222.',
             '.....3322222222.', '....32222222221.', '...322222222211.', '...32222222211..', '..322222222211..',
             '..32222222111...', '..3222221111....', '..2111111.......', '.11.............', '1...............',
             '................'],
    'flame': ['.......6........', '......66........', '......656.......', '.....6556.......', '.....65556..6...',
              '....655556.66...', '....6555556556..', '...65554555556..', '...65544455556..', '..655444445556..',
              '..654444444556..', '..654444444456..', '..554444444455..', '...5544444455...', '....55444455....',
              '......5555......'],
    'snow': ['.......8........', '....8..8..8.....', '.....8.8.8......', '..8...777...8...', '...8..787..8....',
             '....8.777.8.....', '.888877777888888', '.....77877......', '....8.777.8.....', '...8..787..8....',
             '..8...777...8...', '.....8.8.8......', '....8..8..8.....', '.......8........', '................',
             '................'],
    'rune': ['....99999999....', '...9aaaaaaaa9...', '..9a........a9..', '.9a....aa....a9.', '.9a...a99a...a9.',
             '.9a..a9..9a..a9.', '.9a..a9..9a..a9.', '.9a...a99a...a9.', '.9a....aa....a9.', '.9a....aa....a9.',
             '.9a...aaaa...a9.', '.9a..........a9.', '..9a........a9..', '...9aaaaaaaa9...', '....99999999....',
             '................'],
    'bolt': ['.........dd.....', '........ddb.....', '.......ddb......', '......ddb.......', '.....ddb........',
             '....ddddddddd...', '...ddddddddb....', '.......ddb......', '......ddb.......', '.....ddb........',
             '....ddb.........', '...ddb..........', '..ddb...........', '..db............', '.db.............',
             '................'],
    'crown': ['................', '................', '.d.....d.....d..', '.dd...ddd...dd..', '.ddd..ddd..ddd..',
              '.dddddddddddddd.', '.dc4dcdfdcd9cdd.', '.dc4dcdfdcd9cdd.', '.dddddddddddddd.', '.cccccccccccccc.',
              '..cccccccccccc..', '................', '................', '................', '................',
              '................'],
}
SIGIL_ORDER = ['leaf', 'flame', 'snow', 'rune', 'bolt', 'crown']


def sigils():
    sheet = Canvas(16, 16 * len(SIGIL_ORDER))
    for i, name in enumerate(SIGIL_ORDER):
        for y, row in enumerate(SIGILS[name]):
            for x, ch in enumerate(row):
                if ch != '.':
                    sheet.set(x, i * 16 + y, DIGITS[ch])
        frame = sheet.crop(0, i * 16, 16, 16)
        frame.outline(14)
        sheet.blit(frame, 0, i * 16, transparent=-1)
    return sheet


def crown():
    """64x32 golden crown with three gems (title, cinematics)."""
    c = Canvas(64, 32)
    gold_dark, gold, white = 12, 13, 8
    # band
    c.rect(6, 20, 52, 8, gold_dark)
    c.hline(6, 57, 20, gold)
    c.hline(6, 57, 27, gold_dark)
    for x in range(8, 56, 6):
        c.set(x, 24, white)
    # five points
    for i, (x, h) in enumerate(((8, 10), (20, 15), (32, 19), (44, 15), (56, 10))):
        c.polygon([(x - 5, 20), (x, 20 - h), (x + 5, 20)], gold)
        c.circle(x, 19 - h, 2, gold)
        c.set(x, 19 - h, white)
        c.line(x - 1, 19 - h + 3, x - 3, 19, gold_dark)
    # gems on the band
    for x, color in ((16, 4), (32, 15), (48, 9)):
        c.ellipse(x, 24, 3, 2, color)
        c.set(x - 1, 23, white)
    c.outline(14)
    return c


def generate(root):
    graphics = os.path.join(root, 'graphics')
    write_bmp(os.path.join(graphics, 'sigils.bmp'), sigils(), PARTICLE_PAL, bpp=4)
    write_json(os.path.join(graphics, 'sigils.json'), {'type': 'sprite', 'height': 16})
    write_bmp(os.path.join(graphics, 'crown.bmp'), crown(), PARTICLE_PAL, bpp=4)
    write_json(os.path.join(graphics, 'crown.json'), {'type': 'sprite', 'height': 32})
    sheet = particles()
    write_bmp(os.path.join(graphics, 'particles.bmp'), sheet, PARTICLE_PAL, bpp=4)
    write_json(os.path.join(graphics, 'particles.json'), {'type': 'sprite', 'height': 8})
    marker = map_marker()
    write_bmp(os.path.join(graphics, 'map_marker.bmp'), marker, PARTICLE_PAL, bpp=4)
    write_json(os.path.join(graphics, 'map_marker.json'), {'type': 'sprite', 'height': 16})
    preview = Canvas(8 * len(FRAMES) + 20 + 16 * 6 + 70, 36, fill=14)
    for i in range(len(FRAMES)):
        preview.blit(sheet.crop(0, i * 8, 8, 8), i * 8, 4)
    sig = sigils()
    for i in range(6):
        preview.blit(sig.crop(0, i * 16, 16, 16), 8 * len(FRAMES) + 4 + i * 16, 2)
    preview.blit(crown(), 8 * len(FRAMES) + 4 + 16 * 6 + 2, 2)
    write_png(os.path.join(root, 'assets_source', 'previews', 'particles.png'), preview, PARTICLE_PAL, scale=4,
              transparent_color=None)


if __name__ == '__main__':
    generate(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    print('effects generated')
