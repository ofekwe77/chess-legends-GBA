"""Character portraits of CHESS LEGENDS: ARCANE WARS (dialogue, gallery, boss introductions).

Every portrait is a 64x64 bust facing slightly right, drawn in layers from a character description: attire,
neck, a shaded head (light from the upper left), hair (back and front layers), expression-dependent eyes, brows
and mouth, facial hair, headwear and accessories, then an outline. Three expressions per character:
neutral, intense (determined / angry / smug) and defeated (humbled / surprised).

Each character has its own 16-color palette with fixed index roles, so one sprite item per character holds
its three expressions as frames.
"""
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gfx'))

from canvas import Canvas  # noqa: E402
from imageio import write_bmp, write_json, write_png_rgb  # noqa: E402
import palettes as P  # noqa: E402

# Palette roles
T, OUT, SK0, SK1, SK2, HR0, HR1, HR2, CL0, CL1, CL2, MT0, MT1, EYE, LIGHT, ACC = range(16)

NEUTRAL, INTENSE, DEFEATED = range(3)

SKIN = {
    'pale': ((196, 150, 140), (236, 200, 182), (252, 228, 212)),
    'light': ((192, 134, 104), (230, 176, 140), (248, 208, 176)),
    'tan': ((160, 104, 70), (204, 144, 100), (232, 184, 136)),
    'brown': ((112, 70, 46), (156, 102, 66), (196, 140, 96)),
    'dark': ((70, 42, 32), (108, 68, 48), (148, 100, 72)),
    'frost': ((150, 160, 192), (204, 214, 236), (236, 242, 252)),
}


def palette(skin, hair, cloth, metal, eye, accent, outline=(20, 14, 30), light=(250, 248, 240)):
    pal = [P.TRANSPARENT, outline] + list(SKIN[skin]) + list(hair) + list(cloth) + list(metal) + [eye, light, accent]
    assert len(pal) == 16, len(pal)
    return pal


CHARACTERS = [
    dict(key='arden', skin='light', face='young',
         pal=palette('light', hair=((58, 34, 22), (96, 60, 34), (140, 94, 54)),
                     cloth=((28, 48, 112), (52, 86, 168), (96, 140, 220)), metal=((176, 124, 40), (244, 206, 104)),
                     eye=(60, 120, 80), accent=(200, 50, 60)),
         hair='tousled', attire='tunic', head=None, extras=[]),
    dict(key='orrin', skin='pale', face='old',
         pal=palette('pale', hair=((150, 150, 164), (204, 204, 214), (244, 244, 250)),
                     cloth=((46, 54, 86), (74, 88, 132), (116, 132, 178)), metal=((150, 110, 44), (228, 196, 110)),
                     eye=(70, 110, 160), accent=(240, 214, 112)),
         hair='bald_long', attire='robe', head=None, extras=['beard_long', 'wrinkles']),
    dict(key='rowan', skin='tan', face='woman',
         pal=palette('tan', hair=((92, 40, 22), (146, 70, 34), (196, 112, 58)),
                     cloth=((30, 72, 40), (52, 118, 62), (98, 168, 92)), metal=((150, 120, 40), (226, 204, 110)),
                     eye=(54, 130, 70), accent=(120, 200, 90)),
         hair='long', attire='cloak', head='leaf_circlet', extras=[]),
    dict(key='veyra', skin='pale', face='woman',
         pal=palette('pale', hair=((150, 30, 20), (222, 72, 30), (252, 162, 60)),
                     cloth=((34, 26, 30), (64, 50, 54), (104, 84, 84)), metal=((150, 40, 30), (240, 110, 50)),
                     eye=(230, 140, 40), accent=(252, 200, 80)),
         hair='flame', attire='armor', head='iron_crown', extras=[]),
    dict(key='ysra', skin='frost', face='woman',
         pal=palette('frost', hair=((150, 166, 196), (210, 222, 240), (250, 252, 255)),
                     cloth=((46, 86, 140), (84, 140, 200), (150, 200, 240)), metal=((90, 170, 220), (210, 244, 255)),
                     eye=(70, 170, 230), accent=(160, 230, 255)),
         hair='long_straight', attire='gown', head='ice_crown', extras=[]),
    dict(key='noctis', skin='dark', face='man',
         pal=palette('dark', hair=((26, 18, 40), (44, 30, 66), (70, 50, 100)),
                     cloth=((40, 20, 60), (70, 38, 104), (110, 70, 150)), metal=((150, 150, 170), (224, 224, 236)),
                     eye=(186, 110, 255), accent=(220, 170, 255)),
         hair='short', attire='robe', head='hood', extras=['monocle']),
    dict(key='rael', skin='brown', face='man',
         pal=palette('brown', hair=((168, 132, 40), (226, 196, 80), (252, 240, 150)),
                     cloth=((22, 34, 76), (40, 64, 130), (76, 108, 186)), metal=((170, 140, 30), (252, 230, 90)),
                     eye=(252, 230, 90), accent=(140, 220, 255)),
         hair='spiky', attire='armor', head='storm_crown', extras=['beard_short']),
    dict(key='aurelia', skin='light', face='woman',
         pal=palette('light', hair=((14, 10, 20), (34, 26, 44), (66, 54, 84)),
                     cloth=((150, 140, 150), (230, 226, 230), (252, 252, 252)), metal=((176, 124, 30), (252, 216, 96)),
                     eye=(150, 90, 210), accent=(90, 40, 130)),
         hair='long', attire='royal', head='grand_crown', extras=[]),
    dict(key='brin', skin='tan', face='stout',
         pal=palette('tan', hair=((70, 44, 24), (110, 70, 40), (150, 104, 62)),
                     cloth=((54, 82, 34), (86, 120, 50), (128, 160, 72)), metal=((100, 70, 40), (150, 110, 70)),
                     eye=(90, 70, 40), accent=(160, 200, 90)),
         hair='short', attire='leather', head='leaf_hood', extras=['beard_full']),
    dict(key='harrik', skin='brown', face='man',
         pal=palette('brown', hair=((40, 28, 22), (64, 44, 32), (92, 66, 48)),
                     cloth=((40, 36, 44), (74, 68, 80), (116, 110, 124)), metal=((110, 104, 120), (196, 190, 206)),
                     eye=(120, 70, 40), accent=(214, 50, 40)),
         hair='none', attire='armor', head='helmet', extras=['scar']),
    dict(key='elsk', skin='pale', face='man',
         pal=palette('pale', hair=((150, 170, 196), (196, 214, 236), (238, 246, 255)),
                     cloth=((30, 40, 64), (52, 68, 104), (92, 112, 156)), metal=((110, 190, 220), (220, 248, 255)),
                     eye=(150, 160, 180), accent=(130, 230, 250)),
         hair='slick', attire='high_collar', head=None, extras=['pendant']),
    dict(key='quill', skin='light', face='young',
         pal=palette('light', hair=((30, 22, 30), (54, 40, 54), (86, 66, 86)),
                     cloth=((60, 30, 70), (98, 54, 112), (144, 96, 160)), metal=((130, 100, 50), (210, 180, 110)),
                     eye=(90, 60, 40), accent=(240, 240, 230)),
         hair='messy', attire='vest', head=None, extras=['glasses', 'quill']),
    dict(key='tamsin', skin='light', face='woman',
         pal=palette('light', hair=((150, 50, 20), (210, 90, 40), (246, 140, 70)),
                     cloth=((150, 74, 20), (210, 120, 40), (246, 170, 80)), metal=((120, 90, 40), (210, 170, 90)),
                     eye=(60, 130, 170), accent=(120, 210, 240)),
         hair='bob', attire='jumpsuit', head='goggles', extras=['freckles']),
    dict(key='seraph', skin='brown', face='woman',
         pal=palette('brown', hair=((190, 150, 60), (236, 200, 100), (252, 236, 170)),
                     cloth=((180, 170, 160), (236, 232, 222), (255, 255, 250)), metal=((190, 140, 40), (252, 220, 110)),
                     eye=(200, 150, 60), accent=(255, 240, 180)),
         hair='long', attire='wings', head='halo', extras=[]),
]

HEAD_CX = 31          # head centre (pixels)
HEAD_TOP = 13
HEAD_W = {'young': 11, 'woman': 10, 'man': 11, 'old': 11, 'stout': 12}
HEAD_H = {'young': 13, 'woman': 13, 'man': 14, 'old': 14, 'stout': 13}


# ----------------------------------------------------------------------------------------------- body

def draw_bust(c, ch):
    attire = ch['attire']
    # shoulders: a trapezoid widening downwards
    top = 44
    for y in range(top, 64):
        t = (y - top) / 20
        half = int(15 + t * 16)
        for x in range(HEAD_CX - half, HEAD_CX + half + 1):
            c.set(x, y, CL1)
        # side shading (light from the upper left): darker on the right
        for x in range(HEAD_CX + half - 4, HEAD_CX + half + 1):
            c.set(x, y, CL0)
        if y < top + 3:
            for x in range(HEAD_CX - half + 2, HEAD_CX - half + 8):
                c.set(x, y, CL2)

    if attire in ('tunic', 'vest', 'jumpsuit', 'leather'):
        # V neckline showing skin, trimmed in metal
        for y in range(43, 54):
            w = max(0, 5 - (y - 43) // 2)
            for x in range(HEAD_CX - w, HEAD_CX + w + 1):
                c.set(x, y, SK1 if x < HEAD_CX + w - 1 else SK0)
            c.set(HEAD_CX - w - 1, y, MT1)
            c.set(HEAD_CX + w + 1, y, MT0)
        if attire == 'vest':
            for y in range(50, 64):
                c.set(HEAD_CX - 9, y, MT0)
                c.set(HEAD_CX + 9, y, MT0)
            for y in (54, 58, 62):
                c.set(HEAD_CX - 7, y, MT1)
        if attire == 'jumpsuit':
            # straps and a tool pocket
            for y in range(46, 64):
                c.set(HEAD_CX - 11, y, MT0)
                c.set(HEAD_CX + 11, y, MT0)
            c.rect(HEAD_CX + 3, 56, 6, 5, CL0)
            c.hline(HEAD_CX + 3, HEAD_CX + 8, 56, MT1)
        if attire == 'leather':
            for x in range(HEAD_CX - 14, HEAD_CX + 15, 4):
                c.set(x, 60, MT1)
            c.hline(HEAD_CX - 16, HEAD_CX + 16, 58, MT0)
        if attire == 'tunic':
            # cloak clasp
            c.circle(HEAD_CX - 11, 47, 2, MT1)
            c.set(HEAD_CX - 11, 47, ACC)
    elif attire in ('robe', 'high_collar', 'royal'):
        # high collar rising around the neck
        for y in range(39, 47):
            for x in range(HEAD_CX - 9, HEAD_CX - 4):
                c.set(x, y, CL2 if attire != 'high_collar' else CL1)
            for x in range(HEAD_CX + 5, HEAD_CX + 10):
                c.set(x, y, CL0)
        for y in range(47, 64):
            c.set(HEAD_CX, y, MT0 if attire == 'royal' else CL0)
        if attire == 'royal':
            for y in range(46, 64):
                for x in (HEAD_CX - 2, HEAD_CX + 2):
                    c.set(x, y, MT1)
            for y in range(48, 64):
                for x in range(HEAD_CX - 1, HEAD_CX + 2):
                    c.set(x, y, ACC)
            c.circle(HEAD_CX, 47, 2, MT1)
            c.set(HEAD_CX, 47, ACC)
        if attire == 'robe':
            c.circle(HEAD_CX, 48, 2, MT1)
            c.set(HEAD_CX, 48, ACC)
    elif attire == 'gown':
        # off-shoulder neckline with an ice-blue sash
        for y in range(44, 51):
            half = 12 - (y - 44)
            for x in range(HEAD_CX - half, HEAD_CX + half + 1):
                c.set(x, y, SK1 if x < HEAD_CX + half - 2 else SK0)
        c.hline(HEAD_CX - 14, HEAD_CX + 14, 51, MT1)
        c.hline(HEAD_CX - 15, HEAD_CX + 15, 52, MT0)
        c.circle(HEAD_CX, 54, 2, ACC)
    elif attire == 'armor':
        # pauldrons and a breastplate
        for (cx, shade) in ((HEAD_CX - 17, MT1), (HEAD_CX + 17, MT0)):
            c.ellipse(cx, 50, 9, 6, shade)
            c.ellipse(cx, 51, 8, 4, MT0 if shade == MT1 else CL0)
        for y in range(48, 64):
            half = 8 + (y - 48) // 3
            for x in range(HEAD_CX - half, HEAD_CX + half + 1):
                c.set(x, y, MT0 if x > HEAD_CX + 2 else MT1)
        c.vline(HEAD_CX, 49, 63, CL0)
        c.circle(HEAD_CX, 54, 2, ACC)
    elif attire == 'cloak':
        # hooded cloak folded over the shoulders, leaf clasp
        for y in range(42, 64):
            for x in range(HEAD_CX - 16, HEAD_CX - 10):
                c.set(x, y, CL2 if y < 50 else CL1)
            for x in range(HEAD_CX + 11, HEAD_CX + 17):
                c.set(x, y, CL0)
        c.circle(HEAD_CX, 47, 3, ACC)
        c.set(HEAD_CX, 46, MT1)
    elif attire == 'wings':
        # white robe with feathered wings rising behind the shoulders
        for side in (-1, 1):
            for i in range(5):
                x0 = HEAD_CX + side * (18 + i * 2)
                c.ellipse(x0, 40 + i * 3, 3, 7, CL2 if side < 0 else CL1)
        for y in range(46, 64):
            c.set(HEAD_CX, y, MT1)
        c.circle(HEAD_CX, 47, 2, MT1)


def draw_neck(c, ch):
    for y in range(34, 46):
        for x in range(HEAD_CX - 4, HEAD_CX + 4):
            c.set(x, y, SK1)
        c.set(HEAD_CX + 3, y, SK0)
        c.set(HEAD_CX + 2, y, SK0 if y < 38 else SK1)
    # jaw shadow on the neck
    c.hline(HEAD_CX - 4, HEAD_CX + 3, 34, SK0)
    c.hline(HEAD_CX - 3, HEAD_CX + 3, 35, SK0)


def draw_head(c, ch):
    face = ch['face']
    rx, ry = HEAD_W[face], HEAD_H[face]
    cy = HEAD_TOP + ry
    c.ellipse(HEAD_CX, cy, rx, ry, SK1)
    # jaw: narrower chin
    for y in range(cy + ry - 5, cy + ry + 1):
        inset = (y - (cy + ry - 5)) // 2 + (1 if face == 'woman' else 0)
        c.set(HEAD_CX - rx + inset - 1, y, 0)
        c.set(HEAD_CX + rx - inset + 1, y, 0)
    # shading: right side darker, upper-left highlight
    for y in range(HEAD_TOP, cy + ry + 1):
        row = [x for x in range(HEAD_CX - rx - 1, HEAD_CX + rx + 2) if c.get(x, y) == SK1]
        if not row:
            continue
        right = row[-1]
        for x in range(max(row[0], right - 3), right + 1):
            c.set(x, y, SK0)
    for (x, y) in ((HEAD_CX - 6, cy - 4), (HEAD_CX - 5, cy - 4), (HEAD_CX - 6, cy - 3), (HEAD_CX - 7, cy - 2),
                   (HEAD_CX - 7, cy + 1), (HEAD_CX - 6, cy + 2), (HEAD_CX - 2, cy - 8), (HEAD_CX - 1, cy - 8)):
        if c.get(x, y) == SK1:
            c.set(x, y, SK2)
    # ears
    c.ellipse(HEAD_CX - rx, cy + 1, 1, 3, SK1)
    c.ellipse(HEAD_CX + rx, cy + 1, 1, 3, SK0)
    if face == 'stout':
        # broad cheeks
        c.ellipse(HEAD_CX - 7, cy + 5, 3, 2, SK1)


def eyes_y(ch):
    return HEAD_TOP + HEAD_H[ch['face']] + 1


def draw_face(c, ch, expression):
    face = ch['face']
    ey = eyes_y(ch)
    lx, rx = HEAD_CX - 5, HEAD_CX + 4      # left / right eye: leftmost pixel x
    # eyes (3 px wide): white, iris, pupil
    for (x0, far) in ((lx, False), (rx, True)):
        if expression == DEFEATED:
            # eyes glancing down
            c.hline(x0, x0 + 2, ey, LIGHT)
            c.set(x0 + 1, ey + 1, EYE)
            c.set(x0 + (2 if not far else 1), ey + 1, OUT)
            c.hline(x0, x0 + 2, ey - 1, OUT)
        elif expression == INTENSE:
            # narrowed eyes
            c.hline(x0, x0 + 2, ey, LIGHT)
            c.set(x0 + 1, ey, EYE)
            c.set(x0 + 2, ey, OUT)
            c.hline(x0 - 1, x0 + 3, ey - 1, OUT)
        else:
            c.hline(x0, x0 + 2, ey, LIGHT)
            c.hline(x0, x0 + 2, ey + 1, LIGHT)
            c.set(x0 + 1, ey, EYE)
            c.set(x0 + 2, ey, OUT)
            c.set(x0 + 1, ey + 1, EYE)
            c.set(x0 + 2, ey + 1, EYE)
            c.hline(x0, x0 + 2, ey - 1, OUT)
    if face == 'woman':
        # lashes
        c.set(lx - 1, ey - 1, OUT)
        c.set(rx + 3, ey - 1, OUT)
    # brows
    by = ey - 3
    if expression == INTENSE:
        c.line(lx - 1, by, lx + 3, by + 1, HR0)
        c.line(rx - 1, by + 1, rx + 3, by, HR0)
    elif expression == DEFEATED:
        c.line(lx - 1, by + 1, lx + 3, by, HR0)
        c.line(rx - 1, by, rx + 3, by + 1, HR0)
    else:
        c.hline(lx - 1, lx + 2, by, HR0)
        c.hline(rx, rx + 3, by, HR0)
    # nose: shadow on the right side
    c.vline(HEAD_CX + 1, ey + 1, ey + 4, SK0)
    c.set(HEAD_CX, ey + 5, SK0)
    c.set(HEAD_CX + 1, ey + 5, SK0)
    # mouth
    my = ey + 8
    if expression == INTENSE:
        if ch['key'] in ('veyra', 'noctis', 'aurelia'):
            # smirk
            c.hline(HEAD_CX - 2, HEAD_CX + 2, my, OUT)
            c.set(HEAD_CX + 3, my - 1, OUT)
        else:
            # gritted teeth
            c.hline(HEAD_CX - 3, HEAD_CX + 2, my - 1, OUT)
            c.hline(HEAD_CX - 2, HEAD_CX + 1, my, LIGHT)
            c.hline(HEAD_CX - 3, HEAD_CX + 2, my + 1, OUT)
    elif expression == DEFEATED:
        # open, downturned
        c.set(HEAD_CX - 3, my + 1, OUT)
        c.hline(HEAD_CX - 2, HEAD_CX + 1, my, OUT)
        c.set(HEAD_CX + 2, my + 1, OUT)
        c.set(HEAD_CX - 1, my + 1, SK0)
    else:
        c.hline(HEAD_CX - 2, HEAD_CX + 1, my, OUT)
        c.set(HEAD_CX + 2, my - 1, OUT if face != 'old' else SK0)
        if face == 'woman':
            c.hline(HEAD_CX - 1, HEAD_CX, my + 1, ACC if ch['key'] != 'ysra' else SK0)


# ----------------------------------------------------------------------------------------------- hair

def hair_back(c, ch):
    style = ch['hair']
    if style in ('long', 'long_straight'):
        for y in range(HEAD_TOP + 4, 58 if style == 'long' else 62):
            spread = 13 + (y - HEAD_TOP) // 6
            for x in range(HEAD_CX - spread, HEAD_CX - spread + 6):
                c.set(x, y, HR1 if x < HEAD_CX - spread + 4 else HR0)
            for x in range(HEAD_CX + spread - 5, HEAD_CX + spread + 1):
                c.set(x, y, HR0)
    elif style == 'bald_long':
        for y in range(HEAD_TOP + 10, 46):
            for x in range(HEAD_CX - 14, HEAD_CX - 9):
                c.set(x, y, HR1)
            for x in range(HEAD_CX + 10, HEAD_CX + 15):
                c.set(x, y, HR0)
    elif style == 'flame':
        # tongues of fire rising from the head and falling to the shoulders, dark at the root, bright at the tips
        tongues = ((-13, 12, -2), (-9, 17, -1), (-4, 20, 0), (2, 19, 1), (7, 16, 2), (12, 11, 3))
        for dx, h, lean in tongues:
            base = HEAD_TOP + 8
            for t in range(h):
                y = base - t
                w = max(0, 3 - (t * 3) // h)
                x0 = HEAD_CX + dx + (lean * t) // h
                for x in range(x0 - w, x0 + w + 1):
                    c.set(x, y, HR2 if t > h * 2 // 3 else (HR1 if t > h // 4 else HR0))
        for y in range(HEAD_TOP + 8, 52):
            for x in range(HEAD_CX - 15, HEAD_CX - 10):
                c.set(x, y, HR1 if (y // 3) % 2 else HR0)
            for x in range(HEAD_CX + 11, HEAD_CX + 16):
                c.set(x, y, HR0)
    elif style == 'bob':
        c.ellipse(HEAD_CX, HEAD_TOP + 12, 14, 14, HR0)


def hair_front(c, ch):
    style = ch['hair']
    top = HEAD_TOP
    face = ch['face']
    rx = HEAD_W[face]
    if style == 'none':
        return
    if style in ('tousled', 'messy', 'short', 'spiky', 'slick', 'long', 'long_straight', 'bob', 'flame'):
        # cap of hair over the top of the head
        c.ellipse(HEAD_CX, top + 7, rx + 2, 8, HR1)
        for y in range(top - 1, top + 15):
            for x in range(HEAD_CX - rx - 2, HEAD_CX + rx + 3):
                if c.get(x, y) == HR1 and x > HEAD_CX + 4:
                    c.set(x, y, HR0)
        # clear the face area below the hairline
        hairline = top + (9 if style in ('slick', 'spiky') else 10)
        for y in range(hairline, top + 16):
            for x in range(HEAD_CX - rx + 2, HEAD_CX + rx - 1):
                if c.get(x, y) in (HR0, HR1):
                    c.set(x, y, SK1 if x < HEAD_CX + rx - 4 else SK0)
        # highlight streak
        for i in range(5):
            c.set(HEAD_CX - 5 + i, top + 2 + (i // 2), HR2)
    if style == 'tousled':
        for (x, y) in ((HEAD_CX - 7, top + 10), (HEAD_CX - 5, top + 11), (HEAD_CX - 2, top + 11), (HEAD_CX + 1, top + 10),
                       (HEAD_CX + 3, top + 11), (HEAD_CX - 4, top + 12), (HEAD_CX + 4, top + 12)):
            c.set(x, y, HR1)
        for (x, y) in ((HEAD_CX - 9, top), (HEAD_CX - 3, top - 2), (HEAD_CX + 3, top - 1), (HEAD_CX + 8, top + 1)):
            c.set(x, y, HR1)
            c.set(x + 1, y + 1, HR1)
    elif style == 'messy':
        for i in range(-10, 11, 3):
            c.line(HEAD_CX + i, top + 1, HEAD_CX + i + 2, top + 11, HR1 if i < 4 else HR0)
        for (x, y) in ((HEAD_CX - 11, top + 2), (HEAD_CX - 6, top - 2), (HEAD_CX + 1, top - 2), (HEAD_CX + 9, top)):
            c.set(x, y, HR1)
    elif style == 'spiky':
        for i, dx in enumerate((-10, -6, -2, 2, 6, 10)):
            peak = top - 6 + (abs(dx) // 3)
            c.polygon([(HEAD_CX + dx - 3, top + 4), (HEAD_CX + dx, peak), (HEAD_CX + dx + 3, top + 4)],
                      HR2 if dx < 0 else HR1)
    elif style == 'slick':
        for y in range(top + 1, top + 9):
            c.set(HEAD_CX - 2 + (y - top) // 3, y, HR2)
    elif style in ('long', 'long_straight'):
        # side locks framing the face
        for y in range(top + 6, top + 30):
            c.set(HEAD_CX - rx - 1, y, HR1)
            c.set(HEAD_CX - rx, y, HR1)
            c.set(HEAD_CX + rx, y, HR0)
            c.set(HEAD_CX + rx + 1, y, HR0)
        # parted bangs
        c.line(HEAD_CX - 1, top + 2, HEAD_CX - 8, top + 12, HR1)
        c.line(HEAD_CX, top + 2, HEAD_CX + 7, top + 11, HR0)
    elif style == 'bob':
        for y in range(top + 8, top + 22):
            c.set(HEAD_CX - rx - 1, y, HR1)
            c.set(HEAD_CX - rx - 2, y, HR1)
            c.set(HEAD_CX + rx + 1, y, HR0)
            c.set(HEAD_CX + rx + 2, y, HR0)
        c.hline(HEAD_CX - 8, HEAD_CX + 6, top + 9, HR1)
    elif style == 'bald_long':
        # bald crown with a soft shine
        c.set(HEAD_CX - 4, top + 3, SK2)
        c.set(HEAD_CX - 3, top + 3, SK2)
        c.set(HEAD_CX - 4, top + 4, SK2)


# ----------------------------------------------------------------------------------------------- extras

def draw_extras(c, ch, expression):
    ey = eyes_y(ch)
    extras = ch['extras']
    if 'beard_long' in extras:
        for y in range(ey + 6, 60):
            half = min(10, 4 + (y - ey - 6) // 2) - max(0, (y - 52))
            for x in range(HEAD_CX - half, HEAD_CX + half + 1):
                c.set(x, y, HR1 if x < HEAD_CX + half - 2 else HR0)
        # mustache and the mouth inside the beard
        c.hline(HEAD_CX - 5, HEAD_CX + 4, ey + 7, HR2)
        c.hline(HEAD_CX - 2, HEAD_CX + 1, ey + 8, OUT if expression != DEFEATED else SK0)
        for i in range(4):
            c.set(HEAD_CX - 4 + i * 3, ey + 14 + i * 2, HR2)
    if 'beard_full' in extras:
        for y in range(ey + 5, ey + 14):
            half = 9 - max(0, (y - ey - 9))
            for x in range(HEAD_CX - half, HEAD_CX + half + 1):
                if c.get(x, y) in (SK0, SK1, SK2) or y > ey + 10:
                    c.set(x, y, HR1 if x < HEAD_CX + half - 3 else HR0)
        c.hline(HEAD_CX - 2, HEAD_CX + 1, ey + 8, OUT)
    if 'beard_short' in extras:
        for y in range(ey + 9, ey + 13):
            for x in range(HEAD_CX - 6 + (y - ey - 9), HEAD_CX + 6 - (y - ey - 9)):
                c.set(x, y, HR0 if (x + y) % 2 else HR1)
    if 'wrinkles' in extras:
        c.set(HEAD_CX - 7, ey + 2, SK0)
        c.set(HEAD_CX + 7, ey + 2, SK0)
        c.hline(HEAD_CX - 3, HEAD_CX + 2, ey - 6, SK0)
    if 'scar' in extras:
        c.line(HEAD_CX + 3, ey - 3, HEAD_CX + 6, ey + 4, ACC)
    if 'monocle' in extras:
        c.circle(HEAD_CX + 5, ey, 3, MT1, filled=False)
        c.line(HEAD_CX + 8, ey + 1, HEAD_CX + 10, ey + 10, MT0)
    if 'glasses' in extras:
        c.frame(HEAD_CX - 7, ey - 2, 6, 5, MT0)
        c.frame(HEAD_CX + 2, ey - 2, 6, 5, MT0)
        c.hline(HEAD_CX - 1, HEAD_CX + 1, ey - 1, MT0)
    if 'freckles' in extras:
        for (x, y) in ((HEAD_CX - 6, ey + 3), (HEAD_CX - 4, ey + 4), (HEAD_CX + 3, ey + 4), (HEAD_CX + 5, ey + 3)):
            c.set(x, y, SK0)
    if 'pendant' in extras:
        c.line(HEAD_CX - 4, 44, HEAD_CX, 50, MT0)
        c.line(HEAD_CX + 4, 44, HEAD_CX, 50, MT0)
        c.ellipse(HEAD_CX, 52, 2, 3, ACC)
        c.set(HEAD_CX - 1, 51, LIGHT)
    if 'quill' in extras:
        c.line(HEAD_CX + 12, 4, HEAD_CX + 9, 22, LIGHT)
        c.line(HEAD_CX + 13, 5, HEAD_CX + 10, 20, ACC)
        c.set(HEAD_CX + 9, 23, OUT)


def draw_headwear(c, ch):
    head = ch['head']
    top = HEAD_TOP
    rx = HEAD_W[ch['face']]
    if head == 'leaf_circlet':
        c.hline(HEAD_CX - rx - 1, HEAD_CX + rx + 1, top + 7, MT0)
        for i, dx in enumerate((-9, -5, 0, 5, 9)):
            c.ellipse(HEAD_CX + dx, top + 5, 2, 3, ACC if i % 2 == 0 else CL2)
        c.set(HEAD_CX, top + 4, MT1)
    elif head == 'iron_crown':
        c.rect(HEAD_CX - 9, top + 1, 19, 4, CL0)
        for dx in (-8, -3, 2, 7):
            c.polygon([(HEAD_CX + dx, top + 1), (HEAD_CX + dx + 1, top - 5), (HEAD_CX + dx + 2, top + 1)], CL1)
        c.circle(HEAD_CX, top + 2, 1, MT1)
        c.hline(HEAD_CX - 9, HEAD_CX + 9, top + 5, MT0)
    elif head == 'ice_crown':
        for i, (dx, h) in enumerate(((-8, 5), (-4, 8), (0, 11), (4, 8), (8, 5))):
            c.polygon([(HEAD_CX + dx - 2, top + 3), (HEAD_CX + dx, top + 3 - h), (HEAD_CX + dx + 2, top + 3)],
                      MT1 if dx <= 0 else MT0)
            c.set(HEAD_CX + dx, top + 2 - h // 2, LIGHT)
        c.hline(HEAD_CX - 10, HEAD_CX + 10, top + 3, MT0)
    elif head == 'storm_crown':
        c.hline(HEAD_CX - 10, HEAD_CX + 10, top + 4, MT0)
        c.hline(HEAD_CX - 10, HEAD_CX + 10, top + 5, MT1)
        for dx in (-7, 0, 7):
            c.polyline([(HEAD_CX + dx, top + 4), (HEAD_CX + dx + 2, top - 1), (HEAD_CX + dx, top - 1),
                        (HEAD_CX + dx + 2, top - 6)], MT1)
        c.circle(HEAD_CX, top + 4, 1, ACC)
    elif head == 'grand_crown':
        c.rect(HEAD_CX - 10, top - 1, 21, 5, MT0)
        c.hline(HEAD_CX - 10, HEAD_CX + 10, top - 1, MT1)
        for dx in (-9, -4, 1, 6):
            c.polygon([(HEAD_CX + dx, top - 1), (HEAD_CX + dx + 2, top - 8), (HEAD_CX + dx + 4, top - 1)], MT1)
            c.set(HEAD_CX + dx + 2, top - 7, LIGHT)
        for dx in (-6, 0, 6):
            c.circle(HEAD_CX + dx, top + 1, 1, ACC)
    elif head == 'hood':
        # hood rim over the forehead and along the face (the hood itself is drawn behind the head)
        for x in range(HEAD_CX - rx - 2, HEAD_CX + rx + 3):
            for y in range(top - 3, top + 4):
                if c.get(x, y) != T or y >= top:
                    c.set(x, y, CL1 if x < HEAD_CX + 3 else CL0)
            if c.get(x, top + 4) in (SK0, SK1, SK2, HR0, HR1, HR2):
                c.set(x, top + 4, SK0)
        for y in range(top + 4, top + 26):
            for x in (HEAD_CX - rx - 2, HEAD_CX - rx - 1):
                c.set(x, y, CL1)
            for x in (HEAD_CX + rx + 1, HEAD_CX + rx + 2):
                c.set(x, y, CL0)
        c.hline(HEAD_CX - rx + 1, HEAD_CX + rx - 1, top - 2, CL2)
    elif head == 'leaf_hood':
        for x in range(HEAD_CX - rx - 1, HEAD_CX + rx + 2):
            for y in range(top + 1, top + 5):
                c.set(x, y, CL1 if x < HEAD_CX + 3 else CL0)
        for i in range(-rx, rx + 1, 4):
            c.ellipse(HEAD_CX + i, top + 4, 2, 2, CL2 if i < 0 else CL1)
            c.set(HEAD_CX + i, top + 4, ACC)
    elif head == 'helmet':
        # brow band with rivets, cheek guards and a plume (the dome is drawn behind the head)
        c.rect(HEAD_CX - rx - 1, top + 5, 2 * rx + 3, 4, MT0)
        c.hline(HEAD_CX - rx - 1, HEAD_CX + rx + 1, top + 5, MT1)
        for dx in (-8, -3, 2, 7):
            c.set(HEAD_CX + dx, top + 7, LIGHT)
        for y in range(top + 9, top + 23):
            for x in (HEAD_CX - rx - 2, HEAD_CX - rx - 1):
                c.set(x, y, MT1)
            for x in (HEAD_CX + rx + 1, HEAD_CX + rx + 2):
                c.set(x, y, MT0)
        c.vline(HEAD_CX, top + 9, top + 13, MT0)
        for t in range(10):
            c.ellipse(HEAD_CX + 2 + t, top - 6 - (t // 2), 2, 2, ACC)
    elif head == 'goggles':
        c.hline(HEAD_CX - rx - 1, HEAD_CX + rx + 1, top + 5, MT0)
        c.circle(HEAD_CX - 5, top + 5, 3, MT1)
        c.circle(HEAD_CX + 4, top + 5, 3, MT0)
        c.circle(HEAD_CX - 5, top + 5, 1, ACC)
        c.circle(HEAD_CX + 4, top + 5, 1, ACC)
    elif head == 'halo':
        c.ellipse(HEAD_CX, top - 7, 10, 2, MT1, filled=False)
        c.ellipse(HEAD_CX, top - 7, 9, 1, ACC, filled=False)


def headwear_back(c, ch):
    """Parts of head coverings that sit behind the head (hood cloth, helmet dome)."""
    head = ch['head']
    top = HEAD_TOP
    rx = HEAD_W[ch['face']]
    if head == 'hood':
        c.ellipse(HEAD_CX, top + 12, rx + 7, 18, CL1)
        for y in range(top - 8, top + 32):
            for x in range(HEAD_CX + 2, HEAD_CX + rx + 9):
                if c.get(x, y) == CL1:
                    c.set(x, y, CL0)
        # hood point
        c.polygon([(HEAD_CX - 4, top - 4), (HEAD_CX + 8, top - 9), (HEAD_CX + 6, top)], CL1)
    elif head == 'leaf_hood':
        c.ellipse(HEAD_CX, top + 8, rx + 5, 13, CL1)
        for i in range(-rx - 4, rx + 5, 4):
            c.ellipse(HEAD_CX + i, top - 3 + abs(i) // 3, 2, 3, CL2 if i < 0 else CL0)
    elif head == 'helmet':
        c.ellipse(HEAD_CX, top + 9, rx + 3, 13, MT0)
        c.ellipse(HEAD_CX - 3, top + 4, rx - 3, 7, MT1)


def portrait(ch, expression):
    c = Canvas(64, 64)
    hair_back(c, ch)
    headwear_back(c, ch)
    draw_bust(c, ch)
    draw_neck(c, ch)
    draw_head(c, ch)
    hair_front(c, ch)
    draw_face(c, ch, expression)
    draw_extras(c, ch, expression)
    draw_headwear(c, ch)
    c.outline(OUT)
    return c


def generate(root):
    graphics = os.path.join(root, 'graphics')
    previews = []
    for ch in CHARACTERS:
        sheet = Canvas(64, 64 * 3)
        for e in range(3):
            sheet.blit(portrait(ch, e), 0, e * 64)
        name = 'portrait_' + ch['key']
        write_bmp(os.path.join(graphics, name + '.bmp'), sheet, ch['pal'], bpp=4)
        write_json(os.path.join(graphics, name + '.json'), {'type': 'sprite', 'height': 64})
        previews.append((sheet, ch['pal']))

    # Preview: every character in every expression on a dialogue-box colour.
    cols = len(previews)
    width, height = cols * 66, 3 * 66
    background = (38, 36, 72)
    rows = [[background] * width for _ in range(height)]
    for i, (sheet, pal) in enumerate(previews):
        for e in range(3):
            for y in range(64):
                for x in range(64):
                    index = sheet.get(x, e * 64 + y)
                    if index:
                        rows[e * 66 + 1 + y][i * 66 + 1 + x] = pal[index]
    write_png_rgb(os.path.join(root, 'assets_source', 'previews', 'portraits.png'), width, height, rows, scale=2)


if __name__ == '__main__':
    generate(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    print('portraits generated')
