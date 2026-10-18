"""Master color palettes of CHESS LEGENDS: ARCANE WARS.

Color direction: jewel tones over deep indigo, gold trim, readable light/dark contrast. Every palette is 16
entries (one GBA palette bank); entry 0 is transparent for sprites.
"""

TRANSPARENT = (255, 0, 255)

# ----------------------------------------------------------------------------------- piece palettes
# Index roles shared by every army palette (the piece sprites use these roles, so armies are palette swaps):
#  1 outline  2-5 body (dark -> light)  6-8 primary accent (dark -> light)  9-10 secondary accent
#  11 detail (eyes, engravings)  12 gem  13 rim highlight  14-15 spare
PIECE_ROLES = {
    'outline': 1, 'body0': 2, 'body1': 3, 'body2': 4, 'body3': 5,
    'acc0': 6, 'acc1': 7, 'acc2': 8, 'sec0': 9, 'sec1': 10, 'detail': 11, 'gem': 12, 'rim': 13,
}


def army(outline, body, accent, secondary, detail, gem, rim):
    pal = [TRANSPARENT, outline] + list(body) + list(accent) + list(secondary) + [detail, gem, rim]
    pal += [(0, 0, 0)] * (16 - len(pal))
    assert len(pal) == 16
    return pal


# Light side: ivory and silver with gold trim and sapphire accents.
ARMY_WHITE = army(
    outline=(28, 22, 46),
    body=((112, 108, 140), (168, 168, 192), (216, 216, 230), (250, 250, 252)),
    accent=((140, 88, 28), (216, 160, 56), (252, 228, 128)),
    secondary=((48, 92, 184), (120, 172, 244)),
    detail=(52, 44, 80),
    gem=(216, 48, 72),
    rim=(255, 255, 255),
)

# Dark side: obsidian with crimson trim and violet accents (light highlights keep it readable on dark squares).
ARMY_BLACK = army(
    outline=(10, 6, 18),
    body=((34, 26, 50), (58, 46, 82), (88, 72, 118), (136, 118, 166)),
    accent=((104, 18, 34), (184, 44, 60), (244, 104, 104)),
    secondary=((88, 48, 140), (168, 116, 228)),
    detail=(244, 214, 150),
    gem=(96, 236, 200),
    rim=(196, 180, 228),
)

# Kingdom factions (cosmetic only). Light factions keep light bodies, dark factions keep dark bodies, so the
# two sides of a match always stay distinguishable.
FACTIONS_DARK = {
    'verdant': army((8, 18, 10), ((22, 44, 26), (36, 70, 40), (56, 104, 58), (104, 152, 92)),
                    ((112, 82, 24), (200, 156, 52), (248, 220, 116)), ((24, 104, 72), (80, 196, 140)),
                    (232, 240, 160), (248, 196, 72), (176, 228, 150)),
    'inferno': army((16, 4, 4), ((40, 18, 18), (70, 30, 26), (108, 46, 34), (160, 82, 56)),
                    ((176, 72, 8), (240, 136, 24), (255, 216, 96)), ((120, 16, 16), (216, 56, 40)),
                    (255, 220, 120), (255, 160, 32), (248, 168, 120)),
    'frost': army((6, 10, 26), ((22, 34, 70), (36, 56, 106), (58, 88, 146), (110, 148, 200)),
                  ((96, 120, 168), (168, 196, 232), (236, 248, 255)), ((60, 140, 200), (140, 220, 248)),
                  (220, 248, 255), (140, 232, 255), (196, 228, 255)),
    'umbral': army((10, 4, 20), ((30, 16, 52), (52, 28, 86), (82, 48, 128), (130, 96, 180)),
                   ((90, 70, 140), (160, 140, 220), (224, 216, 255)), ((110, 40, 160), (196, 104, 248)),
                   (200, 255, 240), (120, 255, 220), (200, 168, 248)),
    'storm': army((6, 10, 18), ((26, 34, 50), (44, 56, 80), (70, 86, 118), (118, 136, 170)),
                  ((150, 110, 30), (230, 190, 60), (255, 240, 150)), ((40, 120, 220), (110, 200, 255)),
                  (255, 248, 180), (90, 230, 255), (176, 200, 232)),
    'nexus': army((14, 8, 24), ((44, 24, 60), (72, 40, 92), (108, 64, 128), (160, 112, 176)),
                  ((150, 100, 30), (236, 188, 72), (255, 244, 176)), ((140, 60, 200), (220, 150, 255)),
                  (255, 250, 220), (255, 220, 100), (236, 200, 255)),
}

# ----------------------------------------------------------------------------------- user interface
UI = [
    TRANSPARENT,
    (14, 10, 26),     # 1 ink / outline
    (26, 24, 50),     # 2 panel darkest
    (38, 36, 72),     # 3 panel dark
    (54, 52, 98),     # 4 panel mid
    (78, 74, 132),    # 5 panel light
    (112, 70, 22),    # 6 gold dark
    (196, 140, 46),   # 7 gold mid
    (246, 214, 112),  # 8 gold light
    (255, 248, 210),  # 9 gold highlight
    (240, 236, 226),  # 10 text white
    (120, 112, 150),  # 11 muted
    (220, 54, 62),    # 12 red
    (84, 214, 248),   # 13 arcane cyan
    (172, 100, 236),  # 14 arcane violet
    (100, 220, 128),  # 15 green
]

# Text palettes (sprite fonts): 1 glyph, 2 outline/shadow, 3 inner highlight.
def text_palette(glyph, shadow, highlight):
    return [TRANSPARENT, glyph, shadow, highlight] + [(0, 0, 0)] * 12


TEXT_WHITE = text_palette((242, 238, 228), (20, 14, 34), (255, 255, 255))
TEXT_GOLD = text_palette((246, 204, 92), (40, 20, 8), (255, 244, 190))
TEXT_GRAY = text_palette((130, 124, 156), (20, 14, 34), (170, 164, 196))
TEXT_RED = text_palette((246, 92, 92), (36, 6, 10), (255, 190, 180))
TEXT_CYAN = text_palette((110, 222, 250), (8, 22, 44), (220, 250, 255))
TEXT_DARK = text_palette((40, 28, 60), (200, 186, 150), (90, 70, 110))   # ink on parchment

# ----------------------------------------------------------------------------------- board themes
# Board BG bank layout: 1 frame ink, 2-4 frame metal (dark -> light), 5 frame highlight,
# 6-8 light square (dark -> light), 9-11 dark square (dark -> light), 12 selection tint, 13 last-move tint,
# 14 check glow, 15 coordinate text.
def board_theme(frame, light, dark, select, last, check, coords):
    pal = [TRANSPARENT, frame[0], frame[1], frame[2], frame[3], frame[4]] + list(light) + list(dark)
    pal += [select, last, check, coords]
    assert len(pal) == 16, len(pal)
    return pal


BOARD_THEMES = {
    # The default: ivory and sea-green marble in a gold frame.
    'royal': board_theme(
        frame=((18, 12, 28), (96, 58, 20), (176, 124, 40), (236, 196, 96), (255, 244, 196)),
        light=((198, 190, 166), (220, 214, 192), (238, 234, 214)),
        dark=((70, 104, 108), (88, 126, 128), (106, 146, 146)),
        select=(252, 212, 92), last=(170, 196, 112), check=(236, 60, 60), coords=(250, 236, 190)),
    'verdant': board_theme(
        frame=((10, 16, 10), (70, 52, 22), (140, 108, 46), (206, 176, 96), (248, 236, 170)),
        light=((176, 182, 138), (198, 204, 160), (216, 222, 180)),
        dark=((58, 98, 58), (74, 120, 70), (92, 140, 84)),
        select=(250, 220, 100), last=(150, 210, 120), check=(236, 70, 60), coords=(236, 240, 190)),
    'inferno': board_theme(
        frame=((14, 6, 6), (90, 30, 14), (170, 72, 24), (240, 140, 50), (255, 222, 140)),
        light=((176, 150, 128), (198, 172, 148), (218, 194, 168)),
        dark=((92, 52, 46), (112, 64, 54), (132, 78, 62)),
        select=(255, 214, 80), last=(220, 150, 80), check=(255, 72, 40), coords=(255, 230, 190)),
    'frost': board_theme(
        frame=((10, 14, 30), (70, 90, 140), (140, 166, 210), (210, 228, 248), (255, 255, 255)),
        light=((200, 214, 230), (220, 232, 244), (238, 246, 252)),
        dark=((92, 120, 162), (110, 140, 182), (128, 160, 200)),
        select=(250, 230, 120), last=(150, 210, 230), check=(236, 70, 90), coords=(236, 246, 255)),
    'umbral': board_theme(
        frame=((12, 6, 22), (64, 36, 100), (120, 80, 170), (186, 150, 230), (238, 222, 255)),
        light=((176, 166, 196), (196, 186, 214), (214, 206, 230)),
        dark=((76, 62, 116), (92, 76, 136), (110, 92, 156)),
        select=(246, 214, 110), last=(150, 140, 220), check=(236, 70, 110), coords=(232, 220, 255)),
    'storm': board_theme(
        frame=((10, 12, 20), (60, 70, 96), (120, 136, 170), (196, 210, 236), (255, 248, 196)),
        light=((186, 194, 206), (206, 214, 224), (224, 230, 238)),
        dark=((72, 96, 128), (88, 114, 148), (106, 132, 168)),
        select=(255, 226, 90), last=(120, 200, 240), check=(240, 70, 70), coords=(230, 240, 255)),
    'nexus': board_theme(
        frame=((20, 10, 20), (120, 80, 24), (210, 160, 56), (250, 220, 120), (255, 252, 224)),
        light=((214, 204, 176), (232, 224, 198), (246, 240, 220)),
        dark=((110, 76, 128), (130, 92, 150), (150, 110, 170)),
        select=(255, 220, 110), last=(200, 170, 230), check=(240, 60, 70), coords=(255, 244, 210)),
    # Accessibility: maximum value contrast, strong frame, saturated highlights.
    'contrast': board_theme(
        frame=((0, 0, 0), (60, 60, 60), (140, 140, 140), (220, 220, 220), (255, 255, 255)),
        light=((214, 214, 214), (236, 236, 236), (255, 255, 255)),
        dark=((60, 60, 72), (78, 78, 90), (96, 96, 110)),
        select=(255, 220, 0), last=(0, 200, 120), check=(255, 0, 0), coords=(255, 255, 0)),
}

BOARD_THEME_ORDER = ['royal', 'verdant', 'inferno', 'frost', 'umbral', 'storm', 'nexus', 'contrast']

# Board marker overlay bank: 1 ink, 2 move dot, 3 move dot light, 4 capture red dark, 5 capture red,
# 6 capture light, 7 magic violet dark, 8 magic violet, 9 magic cyan, 10 magic light, 11 cursor gold dark,
# 12 cursor gold, 13 cursor light, 14 escape gold, 15 white.
MARKERS = [
    TRANSPARENT, (16, 12, 28), (40, 34, 64), (94, 88, 130), (120, 20, 30), (220, 56, 60), (255, 150, 140),
    (70, 30, 120), (160, 90, 240), (80, 210, 250), (220, 250, 255), (120, 72, 20), (230, 176, 56),
    (255, 236, 150), (255, 210, 80), (255, 255, 255),
]
