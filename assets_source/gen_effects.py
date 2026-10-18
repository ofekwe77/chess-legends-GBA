"""Effect sprites of CHESS LEGENDS: ARCANE WARS: particles (cinematics, spells) and the world-map marker.

Particles are 8x8 frames sharing one 16-color palette; the map marker is a 16x16 banner with three states.
"""
import math
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


# 16x16 spell effects (board spells, battle impacts): frame sequences are animated by EffectLayer::fx.
FX_FRAMES = ['smoke_a', 'smoke_b', 'smoke_c', 'ring_s', 'ring_m', 'ring_l', 'flare_a', 'flare_b',
             'gold_ring_s', 'gold_ring_m', 'gold_ring_l', 'glow']


def _ring(c, top, radius, outer, inner, thin=False):
    for y in range(16):
        for x in range(16):
            d = math.hypot(x - 7.5, y - 7.5)
            if abs(d - radius) <= 0.95 and not (thin and (x + y) % 2):
                c.set(x, top + y, outer if d >= radius else inner)


def _smoke(c, top, stage):
    """A violet cloud of overlapping puffs that spreads and thins out (stage 0-2)."""
    blobs = [(6.0, 8.5, 4.6), (10.0, 7.5, 3.9), (8.0, 4.8, 3.6), (4.6, 5.2, 2.8), (11.2, 10.8, 2.7)]
    spread = 1 + stage * 0.45
    shrink = 1 - stage * 0.2
    for y in range(16):
        for x in range(16):
            for bx, by, r in blobs:
                cx = 7.5 + (bx - 7.5) * spread
                cy = 7.5 + (by - 7.5) * spread
                rr = r * shrink
                d = math.hypot(x - cx, y - cy)
                if d > rr:
                    continue
                if stage and (x * 3 + y * 5) % (5 - stage) == 0:
                    continue                                        # thinning holes
                if (x - cx) + (y - cy) < -rr * 0.6:
                    color = 10                                      # lit edge
                elif d > rr - 1.1 and (x - cx) + (y - cy) > 0:
                    color = 14                                      # shaded edge
                else:
                    color = 9
                if c.get(x, top + y) in (0, 14) or color == 10:
                    c.set(x, top + y, color)
                break


def _flare(c, top, long_ray, short_ray):
    for k in range(-long_ray, long_ray + 1):
        tip = abs(k) > long_ray - 2
        c.set(7 + k, top + 7, 15 if tip else 8)
        c.set(7, top + 7 + k, 15 if tip else 8)
    for k in range(-short_ray, short_ray + 1):
        c.set(7 + k, top + 7 + k, 15)
        c.set(7 + k, top + 7 - k, 15)
    c.rect(6, top + 6, 3, 3, 8)


def spell_fx():
    sheet = Canvas(16, 16 * len(FX_FRAMES))
    for i, name in enumerate(FX_FRAMES):
        top = i * 16
        if name.startswith('smoke'):
            _smoke(sheet, top, 'abc'.index(name[-1]))
        elif name.startswith('ring'):
            radius = {'s': 3.0, 'm': 5.2, 'l': 7.0}[name[-1]]
            _ring(sheet, top, radius, 8, 15, thin=name.endswith('l'))
        elif name.startswith('gold_ring'):
            radius = {'s': 3.0, 'm': 5.2, 'l': 7.0}[name[-1]]
            _ring(sheet, top, radius, 13, 12, thin=name.endswith('l'))
        elif name == 'flare_a':
            _flare(sheet, top, 7, 4)
        elif name == 'flare_b':
            _flare(sheet, top, 4, 2)
        elif name == 'glow':
            for y in range(16):
                for x in range(16):
                    d = math.hypot(x - 7.5, y - 7.5)
                    if 5.6 <= d <= 7.4 and (x + y) % 2 == 0:
                        sheet.set(x, top + y, 15 if d < 6.6 else 10)
    return sheet


def tint_palettes():
    """Whole-sprite tints for spell effects (a piece drawn with one of these instead of its army palette):
    a shadow silhouette (Shadow Step), a white flash (Phase Shift) and a golden glow (Castle Swap, Royal Escape).
    The silhouette keeps a glowing violet outline so it stays visible on dark squares."""
    def ramp(dark, light):
        colors = [P.TRANSPARENT]
        for i in range(15):
            t = i / 14
            colors.append(tuple(int(dark[k] + (light[k] - dark[k]) * t) for k in range(3)))
        return colors
    silhouette = [P.TRANSPARENT, (190, 140, 255)] + [(34 + 2 * i, 22 + i, 58 + 3 * i) for i in range(14)]
    return {
        'tint_shadow': silhouette,
        'tint_flash': [P.TRANSPARENT, (120, 140, 200)] + [(255, 255, 255)] * 14,
        'tint_gold': ramp((90, 50, 10), (255, 244, 170)),
    }


def generate(root):
    graphics = os.path.join(root, 'graphics')
    swatch = Canvas(8, 8)
    for i in range(16):
        swatch.set(i % 8, i // 8, i)
    for name, palette in tint_palettes().items():
        write_bmp(os.path.join(graphics, name + '.bmp'), swatch, palette, bpp=4)
        write_json(os.path.join(graphics, name + '.json'), {'type': 'sprite_palette'})
    write_bmp(os.path.join(graphics, 'spell_fx.bmp'), spell_fx(), PARTICLE_PAL, bpp=4)
    write_json(os.path.join(graphics, 'spell_fx.json'), {'type': 'sprite', 'height': 16})
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
