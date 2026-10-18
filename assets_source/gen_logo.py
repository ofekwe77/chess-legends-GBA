"""Title logo of CHESS LEGENDS: ARCANE WARS: "CHESS" over "LEGENDS" in the title font drawn at double size, with a
gold gradient, a dark outline, a drop shadow and a few glints. Output: a screen-aligned regular BG (transparent
around the logo) shown above the title backdrop; the crown sprite and the "ARCANE WARS" subtitle complete it.
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gfx'))

from canvas import Canvas  # noqa: E402
from imageio import write_bmp, write_json, write_png  # noqa: E402
import palettes as P  # noqa: E402
from gen_font_big import BIG  # noqa: E402

W, H = 240, 160
SCALE = 2
LOGO_PAL = [P.TRANSPARENT,
            (34, 16, 8),                                           # 1 outline
            (14, 10, 34),                                          # 2 shadow
            (110, 60, 18), (160, 100, 28), (206, 150, 44),          # 3-5 gold, dark to mid
            (240, 196, 80), (255, 230, 140), (255, 248, 214),       # 6-8 gold, light to highlight
            (255, 255, 255)] + [(0, 0, 0)] * 6                     # 9 glint
GRADIENT = [8, 7, 7, 6, 6, 5, 5, 4, 4, 3, 3]                       # by glyph row (11 rows), top to bottom


def word_width(word):
    return sum(len(BIG[ch][0]) * SCALE for ch in word) + (len(word) - 1) * SCALE * 2


def draw_word(canvas, word, center_x, top):
    x = center_x - word_width(word) // 2
    for ch in word:
        rows = BIG[ch]
        for gy, row in enumerate(rows):
            color = GRADIENT[min(gy, len(GRADIENT) - 1)]
            for gx, cell in enumerate(row):
                if cell == '#':
                    canvas.rect(x + gx * SCALE, top + gy * SCALE, SCALE, SCALE, color)
        x += len(rows[0]) * SCALE + SCALE * 2


def logo():
    letters = Canvas(W, H)
    draw_word(letters, 'CHESS', W // 2, 38)
    draw_word(letters, 'LEGENDS', W // 2, 66)

    # Bevel: edges facing up catch the light, edges facing down fall into shade.
    beveled = letters.copy()
    for y in range(H):
        for x in range(W):
            color = letters.get(x, y)
            if not color:
                continue
            if not letters.get(x, y - 1):
                beveled.set(x, y, min(color + 2, 8))
            elif not letters.get(x, y + 1):
                beveled.set(x, y, max(color - 1, 3))
    letters = beveled

    # Glints on a few top edges.
    for y in (38, 66):
        for x in range(1, W):
            if letters.get(x, y) and not letters.get(x - 1, y) and (x * 7) % 5 == 0:
                letters.set(x, y, 9)

    # Diamonds flanking the first word.
    half = word_width('CHESS') // 2
    for cx in (W // 2 - half - 14, W // 2 + half + 13):
        for dy in range(-5, 6):
            span = 5 - abs(dy)
            for dx in range(-span, span + 1):
                letters.set(cx + dx, 49 + dy, 7 if dx + dy < 0 else 5)
        letters.set(cx - 1, 46, 9)

    c = Canvas(W, H)
    # Drop shadow (2 px down-right), then the letters with a dark outline and a second, outer one.
    for y in range(H):
        for x in range(W):
            if letters.get(x, y):
                c.set(x + 2, y + 2, 2)
    outlined = letters.copy()
    outlined.outline(1, diagonal=True)
    outlined.outline(2, diagonal=False)
    c.blit(outlined, 0, 0)
    return c


def generate(root):
    graphics = os.path.join(root, 'graphics')
    image = logo()
    big = Canvas(256, 256)
    big.blit(image, 0, 0)
    write_bmp(os.path.join(graphics, 'title_logo.bmp'), big, LOGO_PAL, bpp=4)
    write_json(os.path.join(graphics, 'title_logo.json'), {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
    preview = Canvas(W, H, fill=2)
    preview.blit(image, 0, 0)
    write_png(os.path.join(root, 'assets_source', 'previews', 'title_logo.png'), preview, LOGO_PAL, scale=3,
              transparent_color=None)


if __name__ == '__main__':
    generate(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    print('title logo generated')
