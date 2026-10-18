"""Full-screen backdrops of CHESS LEGENDS: ARCANE WARS (240x160 visible, stored as 256x256 regular BGs).

One backdrop per kingdom (spec section 90: forest, volcano, ice palace, shadow realm, storm citadel, royal palace),
the world map and the title sky. Everything is built from deterministic shapes: dithered gradients (4x4 Bayer),
sine-sum skylines, hand-placed architecture and hashed (not random) star and particle fields.
"""
import math
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gfx'))

from canvas import Canvas  # noqa: E402
from imageio import write_bmp, write_json, write_png  # noqa: E402

W, H = 240, 160
BAYER = ((0, 8, 2, 10), (12, 4, 14, 6), (3, 11, 1, 9), (15, 7, 13, 5))


def hashed(x, y, seed=0):
    """Deterministic integer hash -> 0..1."""
    n = (x * 374761393 + y * 668265263 + seed * 2147483647) & 0xFFFFFFFF
    n = (n ^ (n >> 13)) * 1274126177 & 0xFFFFFFFF
    return ((n ^ (n >> 16)) & 0xFFFF) / 65535.0


def gradient(c, y0, y1, colors, x0=0, x1=W):
    """Vertical dithered gradient through the palette indices in `colors` (top to bottom)."""
    steps = len(colors) - 1
    for y in range(y0, y1):
        t = (y - y0) / max(1, (y1 - y0 - 1)) * steps
        base = min(int(t), steps - 1) if steps else 0
        frac = t - base
        for x in range(x0, x1):
            threshold = (BAYER[y & 3][x & 3] + 0.5) / 16
            c.set(x, y, colors[base + 1] if steps and frac > threshold else colors[base])


def skyline(c, base_y, amplitude, waves, color, top_limit=0, seed=0.0):
    """Fills below a sine-sum horizon line."""
    for x in range(W):
        h = 0.0
        for i, (freq, amp) in enumerate(waves):
            h += math.sin(x * freq + seed + i * 1.7) * amp
        y_top = int(base_y - amplitude - h)
        for y in range(max(top_limit, y_top), H):
            c.set(x, y, color)


def stars(c, region, colors, density=0.012, seed=1):
    x0, y0, x1, y1 = region
    for y in range(y0, y1):
        for x in range(x0, x1):
            v = hashed(x, y, seed)
            if v < density:
                c.set(x, y, colors[int(v / density * len(colors)) % len(colors)])


def pine(c, x, base, height, dark, light):
    for i in range(height):
        half = (i * 5) // (height) + 1 + (i % 4 == 3)
        y = base - height + i
        for dx in range(-half, half + 1):
            c.set(x + dx, y, light if dx < 0 and i % 4 != 0 else dark)
    c.vline(x, base, base + 3, dark)


def finalize(c):
    big = Canvas(256, 256)
    big.blit(c, 0, 0, transparent=-1)
    # fill the unused area with the bottom-left color (never visible)
    edge = c.get(0, H - 1)
    for y in range(256):
        for x in range(256):
            if x >= W or y >= H:
                big.set(x, y, edge)
    return big


# ----------------------------------------------------------------------------------------------- kingdoms

def verdant():
    pal = [(10, 22, 20), (16, 40, 34), (24, 62, 48), (34, 88, 60), (50, 118, 74), (80, 150, 92), (120, 182, 120),
           (170, 214, 160), (214, 236, 196), (64, 104, 120), (96, 148, 160), (150, 196, 196), (210, 236, 228),
           (236, 220, 140), (120, 92, 60), (70, 52, 40)]
    c = Canvas(W, H)
    gradient(c, 0, 92, [12, 11, 10, 9])
    # sun glow
    for y in range(20, 60):
        for x in range(150, 220):
            d = math.hypot(x - 186, y - 40)
            if d < 18 + (BAYER[y & 3][x & 3] / 8):
                c.set(x, y, 13 if d < 9 else 12)
    # distant temple on a hill
    skyline(c, 86, 6, ((0.03, 5), (0.07, 2)), 9, seed=0.5)
    c.rect(70, 52, 30, 30, 9)
    for i in range(4):
        c.rect(72 + i * 7, 50, 4, 32, 10)
    c.polygon([(66, 52), (85, 38), (104, 52)], 9)
    c.rect(82, 30, 6, 10, 9)
    # forest layers
    skyline(c, 104, 8, ((0.05, 4), (0.11, 3), (0.23, 2)), 3, seed=1.1)
    for x in range(-4, W + 8, 13):
        pine(c, x + int(hashed(x, 1) * 6), 110 + int(hashed(x, 2) * 6), 26 + int(hashed(x, 3) * 14), 2, 3)
    skyline(c, 132, 6, ((0.06, 4), (0.17, 2)), 1, seed=2.3)
    for x in range(-6, W + 8, 19):
        pine(c, x + int(hashed(x, 4) * 8), 150, 40 + int(hashed(x, 5) * 16), 1, 2)
    # mossy ground and fireflies
    gradient(c, 146, H, [2, 1, 0])
    for i in range(26):
        x = int(hashed(i, 7) * W)
        y = 70 + int(hashed(i, 8) * 80)
        c.set(x, y, 13)
        if hashed(i, 9) > 0.5:
            c.set(x + 1, y, 8)
    return c, pal


def inferno():
    pal = [(14, 6, 8), (32, 10, 12), (60, 16, 16), (98, 24, 18), (146, 34, 18), (196, 62, 22), (236, 110, 30),
           (252, 170, 60), (255, 230, 140), (30, 26, 32), (48, 42, 52), (72, 64, 78), (110, 98, 112),
           (20, 16, 22), (160, 60, 40), (255, 250, 220)]
    c = Canvas(W, H)
    gradient(c, 0, 100, [1, 2, 3, 4, 5])
    # volcano with lava glow
    c.polygon([(20, 112), (96, 30), (128, 30), (210, 112)], 13)
    c.polygon([(96, 30), (112, 22), (128, 30)], 6)
    for y in range(24, 60):
        for dx in range(-3, 4):
            if hashed(dx, y, 3) > 0.35:
                c.set(112 + dx + int(math.sin(y * 0.4) * 2), y, 7 if abs(dx) < 2 else 6)
    # smoke plume
    for i in range(40):
        y = 22 - i // 2
        for dx in range(-4 - i // 6, 5 + i // 6):
            if BAYER[y & 3][(112 + dx) & 3] < 6 and y >= 0:
                c.set(112 + dx + i // 5, y, 10)
    # iron fortress walls
    for x in range(0, W, 1):
        top = 96 + (4 if (x // 10) % 2 else 0)
        for y in range(top, 134):
            c.set(x, y, 9 if x % 20 < 17 else 10)
    for tx in (30, 110, 190):
        c.rect(tx - 9, 70, 18, 64, 11)
        c.rect(tx - 11, 66, 22, 6, 12)
        for i in range(0, 22, 6):
            c.rect(tx - 11 + i, 62, 3, 4, 12)
        c.rect(tx - 2, 86, 4, 8, 6)
    # lava river
    gradient(c, 134, H, [5, 6, 7, 6, 4])
    for i in range(50):
        x = int(hashed(i, 11) * W)
        y = 136 + int(hashed(i, 12) * 22)
        c.set(x, y, 8)
    # embers
    for i in range(40):
        c.set(int(hashed(i, 13) * W), int(hashed(i, 14) * 96), 7 if i % 3 else 8)
    return c, pal


def frozen():
    pal = [(6, 10, 30), (12, 20, 52), (20, 34, 80), (32, 54, 110), (52, 82, 146), (84, 124, 186), (130, 172, 222),
           (190, 220, 246), (240, 250, 255), (40, 120, 120), (70, 190, 160), (140, 240, 200), (110, 140, 190),
           (170, 196, 230), (220, 234, 250), (255, 255, 255)]
    c = Canvas(W, H)
    gradient(c, 0, 110, [0, 1, 2, 3, 4])
    stars(c, (0, 0, W, 80), [7, 8, 15], 0.01, seed=3)
    # aurora ribbons
    for band, (col, off) in enumerate(((9, 0.0), (10, 1.3), (11, 2.1))):
        for x in range(W):
            y = int(26 + band * 5 + math.sin(x * 0.045 + off) * 8 + math.sin(x * 0.13 + off) * 3)
            for t in range(6 - band * 2):
                if BAYER[(y + t) & 3][x & 3] < 10 - band * 3:
                    c.set(x, y + t, col)
    # crystal palace
    skyline(c, 122, 4, ((0.04, 3), (0.09, 2)), 12, seed=0.2)
    for (x, w, h) in ((60, 10, 50), (84, 14, 76), (108, 18, 92), (134, 14, 70), (158, 10, 48), (176, 8, 36),
                      (40, 8, 34)):
        c.polygon([(x - w // 2, 122), (x - w // 2, 122 - h), (x, 122 - h - w), (x + w // 2, 122 - h),
                   (x + w // 2, 122)], 13)
        c.vline(x - w // 2 + 1, 122 - h, 121, 14)
        c.polygon([(x, 122 - h - w), (x + w // 2, 122 - h), (x + w // 2, 122)], 12)
        for wy in range(122 - h + 8, 118, 10):
            c.rect(x - 1, wy, 3, 4, 7)
    # snow field
    gradient(c, 122, H, [13, 14, 8, 15])
    for i in range(60):
        c.set(int(hashed(i, 21) * W), int(hashed(i, 22) * 120), 15 if i % 2 else 7)
    return c, pal


def umbral():
    pal = [(8, 4, 16), (18, 10, 32), (30, 18, 52), (46, 28, 76), (66, 42, 104), (92, 62, 138), (130, 96, 180),
           (180, 150, 226), (234, 220, 255), (40, 24, 20), (70, 44, 32), (104, 70, 46), (150, 104, 60),
           (250, 210, 120), (255, 244, 200), (120, 220, 240)]
    c = Canvas(W, H)
    gradient(c, 0, H, [3, 2, 1, 0])
    # receding bookshelves (one-point perspective)
    vx, vy = 120, 70
    for side in (-1, 1):
        for i in range(6):
            x_near = vx + side * (120 - i * 16)
            x_far = vx + side * (104 - i * 16)
            top_near, top_far = 6 + i * 6, 14 + i * 6
            bot_near, bot_far = 160 - i * 8, 150 - i * 8
            c.polygon([(x_near, top_near), (x_far, top_far), (x_far, bot_far), (x_near, bot_near)], 9 if i % 2 else 10)
            # shelves and books
            for s in range(6):
                t = (s + 1) / 7
                y_n = int(top_near + (bot_near - top_near) * t)
                y_f = int(top_far + (bot_far - top_far) * t)
                c.line(x_near, y_n, x_far, y_f, 11)
                for b in range(0, abs(x_far - x_near), 3):
                    bx = x_near + side * (-b) if side > 0 else x_near + b
                    by = int(y_n + (y_f - y_n) * (b / max(1, abs(x_far - x_near))))
                    color = (4, 5, 6, 12, 11)[(b + s + i) % 5]
                    c.vline(bx, by - 5, by - 1, color)
    # central doorway of light
    gradient(c, 40, 120, [6, 7, 8], 104, 136)
    c.polygon([(100, 120), (104, 40), (120, 30), (136, 40), (140, 120)], 4)
    gradient(c, 42, 120, [5, 6, 7], 106, 134)
    # floating candles and runes
    for i in range(14):
        x = int(20 + hashed(i, 31) * 200)
        y = int(16 + hashed(i, 32) * 70)
        c.rect(x, y, 2, 5, 14)
        c.set(x, y - 1, 13)
        c.set(x + 1, y - 2, 13)
    for i in range(30):
        c.set(int(hashed(i, 33) * W), int(hashed(i, 34) * H), 15)
    return c, pal


def storm():
    pal = [(10, 12, 20), (20, 24, 38), (32, 38, 58), (46, 54, 80), (64, 74, 104), (90, 102, 134), (124, 136, 166),
           (170, 180, 206), (230, 236, 250), (56, 48, 44), (84, 74, 66), (120, 108, 96), (160, 148, 132),
           (250, 240, 140), (255, 255, 230), (110, 180, 230)]
    c = Canvas(W, H)
    gradient(c, 0, H, [5, 4, 3, 2, 1])
    # cloud banks
    for layer, (y, col) in enumerate(((30, 6), (54, 5), (96, 3), (130, 2))):
        for x in range(W):
            top = int(y + math.sin(x * 0.05 + layer) * 5 + math.sin(x * 0.17 + layer * 2) * 3)
            for t in range(10):
                if BAYER[(top + t) & 3][x & 3] < 16 - t:
                    c.set(x, top + t, col)
    # floating islands
    for (x, y, w) in ((40, 104, 40), (186, 92, 34), (120, 132, 60)):
        c.ellipse(x, y, w // 2, 5, 11)
        c.polygon([(x - w // 2, y), (x, y + w // 2), (x + w // 2, y)], 9)
        c.hline(x - w // 2 + 2, x + w // 2 - 2, y - 4, 12)
    # citadel tower on the central island
    c.rect(110, 46, 20, 84, 10)
    c.rect(106, 40, 28, 8, 11)
    c.polygon([(104, 40), (120, 14), (136, 40)], 10)
    for wy in range(54, 124, 12):
        c.rect(118, wy, 4, 6, 13)
    # lightning bolt
    bolt = [(176, 0), (168, 22), (178, 24), (160, 52), (172, 54), (150, 86)]
    c.polyline(bolt, 14)
    c.polyline([(x + 1, y) for x, y in bolt], 13)
    # rain
    for i in range(120):
        x = int(hashed(i, 41) * W)
        y = int(hashed(i, 42) * H)
        c.line(x, y, x - 2, y + 5, 7)
    return c, pal


def nexus():
    pal = [(14, 8, 24), (34, 20, 54), (64, 36, 92), (104, 60, 130), (150, 92, 160), (212, 140, 150),
           (246, 196, 140), (255, 236, 180), (255, 252, 230), (40, 34, 46), (70, 62, 76), (110, 100, 114),
           (160, 150, 160), (230, 180, 60), (252, 224, 110), (180, 120, 255)]
    c = Canvas(W, H)
    # split sky: dusk violet on the left, golden dawn on the right
    for y in range(110):
        for x in range(W):
            t = x / W + (y / 110) * 0.15
            threshold = (BAYER[y & 3][x & 3] + 0.5) / 16
            level = t * 7
            base = min(6, int(level))
            c.set(x, y, base + (1 if level - base > threshold else 0))
    stars(c, (0, 0, 90, 60), [7, 8, 15], 0.012, seed=5)
    # floating crown fragments around a central light
    for y in range(20, 70):
        for x in range(90, 150):
            d = math.hypot(x - 120, y - 44)
            if d < 14 and BAYER[y & 3][x & 3] < 16 - d:
                c.set(x, y, 8)
    for i, (dx, dy) in enumerate(((-30, -8), (-18, -22), (0, -28), (18, -22), (30, -8), (0, 14))):
        x, y = 120 + dx, 44 + dy
        c.polygon([(x - 4, y + 3), (x - 2, y - 4), (x, y + 1), (x + 2, y - 4), (x + 4, y + 3)], 13)
        c.set(x, y, 14)
    # ruined colonnade
    for i, x in enumerate(range(10, W, 28)):
        height = 70 - (i % 3) * 12 - (12 if i in (3, 5) else 0)
        top = 132 - height
        c.rect(x, top, 12, height, 11)
        c.rect(x + 8, top, 4, height, 10)
        c.rect(x - 2, top - 4, 16, 4, 12)
        if i % 3 == 1:
            c.polygon([(x, top), (x + 6, top - 6), (x + 12, top)], 9)
    gradient(c, 132, H, [10, 9, 1])
    c.hline(0, W, 132, 12)
    return c, pal


def world_map():
    pal = [(54, 40, 30), (92, 66, 44), (140, 104, 66), (184, 146, 96), (214, 182, 128), (234, 210, 160),
           (246, 232, 196), (60, 110, 64), (180, 60, 40), (90, 150, 200), (110, 80, 150), (120, 130, 150),
           (220, 180, 70), (40, 30, 24), (100, 160, 110), (250, 246, 230)]
    c = Canvas(W, H)
    gradient(c, 0, H, [5, 4, 5, 4])
    # parchment fibres
    for i in range(400):
        x = int(hashed(i, 51) * W)
        y = int(hashed(i, 52) * H)
        c.set(x, y, 3 if i % 3 else 6)
    # sea along the edges
    for x in range(W):
        coast = int(12 + math.sin(x * 0.07) * 4 + math.sin(x * 0.19) * 2)
        for y in range(0, coast):
            c.set(x, y, 9 if BAYER[y & 3][x & 3] < 10 else 4)
        south = int(H - 10 - math.sin(x * 0.05 + 1) * 4)
        for y in range(south, H):
            c.set(x, y, 9 if BAYER[y & 3][x & 3] < 10 else 4)
    # kingdom regions (above the information panel, which covers y >= 112). Marker positions in the ROM:
    # src/cl_scene_story.cpp marker_x / marker_y = these centres, 10 px higher.
    for i in range(18):
        pine(c, 22 + (i % 6) * 7, 80 + (i // 6) * 7, 6, 7, 14)          # VERDANT around (40, 86)
    c.polygon([(82, 62), (98, 38), (114, 62)], 1)                       # INFERNO around (98, 52)
    c.polygon([(95, 42), (98, 38), (101, 42)], 8)
    for (x, h) in ((154, 18), (162, 26), (170, 16)):                    # FROZEN around (162, 52)
        c.polygon([(x - 4, 62), (x, 62 - h), (x + 4, 62)], 9)
    c.ellipse(80, 102, 14, 7, 10)                                       # UMBRAL around (80, 100)
    c.rect(76, 95, 8, 7, 13)
    for (x, y) in ((198, 86), (208, 80), (214, 90)):                    # STORM around (206, 86)
        c.ellipse(x, y, 7, 3, 11)
    c.polygon([(202, 82), (206, 72), (210, 82)], 11)
    c.polygon([(124, 86), (128, 78), (132, 82), (136, 76), (140, 82), (144, 78), (148, 86)], 12)   # NEXUS (136, 82)
    # dotted roads between the kingdoms
    for (x0, y0, x1, y1) in ((48, 84, 90, 60), (106, 56, 154, 56), (162, 62, 140, 80), (132, 86, 88, 100),
                             (92, 100, 200, 90), (200, 88, 146, 84)):
        steps = int(math.hypot(x1 - x0, y1 - y0) // 4)
        for s in range(steps):
            t = s / max(1, steps)
            if s % 2 == 0:
                c.set(int(x0 + (x1 - x0) * t), int(y0 + (y1 - y0) * t), 0)
    # compass rose
    c.circle(214, 32, 9, 2, filled=False)
    c.polygon([(214, 20), (217, 32), (214, 44), (211, 32)], 13)
    c.polygon([(202, 32), (214, 29), (226, 32), (214, 35)], 2)
    c.set(214, 32, 12)
    return c, pal


def title_sky():
    pal = [(4, 4, 14), (10, 10, 28), (18, 16, 46), (30, 24, 70), (46, 34, 96), (70, 48, 126), (104, 70, 160),
           (150, 110, 200), (220, 200, 255), (40, 30, 30), (90, 70, 40), (170, 130, 60), (240, 200, 90),
           (255, 244, 190), (80, 200, 240), (255, 255, 255)]
    c = Canvas(W, H)
    gradient(c, 0, 96, [0, 1, 2, 3, 4, 5])
    stars(c, (0, 0, W, 90), [7, 8, 15, 14], 0.016, seed=9)
    # glowing crown silhouette high in the sky
    for y in range(8, 40):
        for x in range(96, 145):
            d = math.hypot((x - 120) * 0.8, y - 24)
            if d < 22 and BAYER[y & 3][x & 3] < 18 - d:
                c.set(x, y, 6)
    c.polygon([(104, 32), (106, 18), (112, 26), (120, 12), (128, 26), (134, 18), (136, 32)], 12)
    c.hline(104, 136, 32, 11)
    c.hline(104, 136, 33, 11)
    for x in (110, 120, 130):
        c.set(x, 30, 14)
    # chessboard floor in perspective
    horizon = 96
    for y in range(horizon, H):
        depth = (y - horizon + 6)
        for x in range(W):
            u = (x - 120) * 16 / depth
            v = 900 / depth
            light = (int(math.floor(u / 6)) + int(math.floor(v / 3))) % 2
            fade = y < horizon + 10
            c.set(x, y, (4 if fade else 7) if light else (2 if fade else 3))
    c.hline(0, W, horizon, 12)
    return c, pal


BACKDROPS = (
    ('backdrop_verdant', verdant), ('backdrop_inferno', inferno), ('backdrop_frozen', frozen),
    ('backdrop_umbral', umbral), ('backdrop_storm', storm), ('backdrop_nexus', nexus),
    ('backdrop_map', world_map), ('backdrop_title', title_sky),
)


def generate(root):
    graphics = os.path.join(root, 'graphics')
    for name, builder in BACKDROPS:
        canvas, pal = builder()
        assert len(pal) == 16, name
        write_bmp(os.path.join(graphics, name + '.bmp'), finalize(canvas), pal, bpp=4)
        write_json(os.path.join(graphics, name + '.json'), {'type': 'regular_bg', 'bpp_mode': 'bpp_4'})
        write_png(os.path.join(root, 'assets_source', 'previews', name + '.png'), canvas, pal, scale=2,
                  transparent_color=None)


if __name__ == '__main__':
    generate(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    print('backdrops generated')
