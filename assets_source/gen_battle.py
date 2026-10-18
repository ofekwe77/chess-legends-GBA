"""Side-view battle characters of CHESS LEGENDS: ARCANE WARS (capture scenes).

Six classes - Soldier (pawn), Cavalry Champion (knight), Arcane Mystic (bishop), Fortress Guardian (rook), Royal
Sorceress (queen), Sovereign (king) - each a 64x64 sprite with six frames: idle, idle (breath), wind-up, strike,
hurt, defeat. Figures face right; the defender is the horizontally flipped sprite. They use the army palette index
roles of the board pieces (gfx/palettes.py PIECE_ROLES), so every army and faction is a palette swap.
"""
import math
import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gfx'))

from canvas import Canvas  # noqa: E402
from imageio import write_bmp, write_json, write_png_rgb  # noqa: E402
import palettes as P  # noqa: E402

R = P.PIECE_ROLES
OUT, B0, B1, B2, B3 = R['outline'], R['body0'], R['body1'], R['body2'], R['body3']
A0, A1, A2 = R['acc0'], R['acc1'], R['acc2']
S0, S1 = R['sec0'], R['sec1']
DET, GEM, RIM = R['detail'], R['gem'], R['rim']

IDLE, BREATH, WINDUP, STRIKE, HURT, DEFEAT = range(6)
GROUND = 60


def line(c, x0, y0, x1, y1, color, width=1):
    if width <= 1:
        c.line(int(x0), int(y0), int(x1), int(y1), color)
    else:
        c.thick_line(int(x0), int(y0), int(x1), int(y1), color, width)


def arm(c, shoulder, angle, length, color, width=4):
    """Draws an arm from the shoulder at `angle` degrees (0 = forward / right, 90 = up); returns the hand."""
    rad = math.radians(angle)
    hand = (shoulder[0] + math.cos(rad) * length, shoulder[1] - math.sin(rad) * length)
    line(c, shoulder[0], shoulder[1], hand[0], hand[1], color, width)
    return hand


def weapon(c, hand, angle, length, color, tip=None, width=2):
    rad = math.radians(angle)
    end = (hand[0] + math.cos(rad) * length, hand[1] - math.sin(rad) * length)
    line(c, hand[0], hand[1], end[0], end[1], color, width)
    if tip:
        c.circle(int(end[0]), int(end[1]), tip[1], tip[0])
    return end


def legs(c, hip_x, hip_y, stance, color, shade):
    """stance: spread of the feet; the back leg is shaded."""
    line(c, hip_x - 2, hip_y, hip_x - 4 - stance, GROUND - 1, shade, 4)
    line(c, hip_x + 2, hip_y, hip_x + 4 + stance, GROUND - 1, color, 4)
    c.hline(int(hip_x - 6 - stance), int(hip_x - 2 - stance), GROUND - 1, OUT)
    c.hline(int(hip_x + 3 + stance), int(hip_x + 7 + stance), GROUND - 1, OUT)


def head(c, x, y, r=6, helmet=None):
    c.circle(int(x), int(y), r, B3)
    c.circle(int(x) + 2, int(y) + 1, r - 2, B2)       # shading away from the light
    c.set(int(x) + 3, int(y) - 1, DET)                 # eye (facing right)
    if helmet == 'soldier':
        c.ellipse(int(x), int(y) - 2, r + 1, r - 1, B1)
        c.hline(int(x) - r, int(x) + r + 1, int(y) - 1, A1)
        c.set(int(x) + 3, int(y) + 1, DET)
    elif helmet == 'knight':
        c.ellipse(int(x), int(y) - 1, r + 1, r, B1)
        c.hline(int(x), int(x) + r + 1, int(y), DET)
        for i in range(5):
            c.set(int(x) - 2 - i, int(y) - r - 1 + i // 2, A2)
    elif helmet == 'mitre':
        c.polygon([(int(x) - r, int(y) - 2), (int(x), int(y) - r - 9), (int(x) + r, int(y) - 2)], B3)
        c.vline(int(x), int(y) - r - 7, int(y) - 3, A1)
        c.hline(int(x) - r + 1, int(x) + r - 1, int(y) - 3, A1)
    elif helmet == 'great':
        c.rect(int(x) - r, int(y) - r - 1, 2 * r + 1, 2 * r + 2, B1)
        c.hline(int(x), int(x) + r, int(y), DET)
        c.hline(int(x) - r, int(x) + r, int(y) - r - 1, A1)
    elif helmet in ('crown', 'tiara'):
        top = int(y) - r - 1
        c.hline(int(x) - r + 1, int(x) + r - 1, top, A1)
        spikes = (-4, 0, 4) if helmet == 'crown' else (-3, 0, 3)
        for dx in spikes:
            c.vline(int(x) + dx, top - (3 if helmet == 'crown' else 2), top, A2)
        c.set(int(x), top - (4 if helmet == 'crown' else 3), GEM)


def torso(c, x, y, w, h, color, shade, trim=None):
    c.rect(int(x - w // 2), int(y), w, h, color)
    c.rect(int(x + w // 2 - 2), int(y), 2, h, shade)
    if trim:
        c.hline(int(x - w // 2), int(x + w // 2 - 1), int(y + h - 2), trim)


def soldier(c, frame):
    lean = {IDLE: 0, BREATH: 0, WINDUP: -2, STRIKE: 5, HURT: -4, DEFEAT: -6}[frame]
    bob = 1 if frame == BREATH else 0
    x = 26 + lean
    hip = 45 + bob
    legs(c, 26 + lean // 2, hip, 3 if frame == STRIKE else 1, B1, B0)
    torso(c, x, 32 + bob, 14, 14, B2, B0, A1)
    # shield on the front arm
    c.ellipse(x + 7, 39 + bob, 4, 7, S1)
    c.ellipse(x + 8, 39 + bob, 2, 5, S0)
    c.set(x + 8, 39 + bob, A2)
    angle = {IDLE: 60, BREATH: 58, WINDUP: 135, STRIKE: -10, HURT: 150, DEFEAT: 200}[frame]
    hand = arm(c, (x - 3, 34 + bob), angle, 8, B2)
    blade_angle = angle + (30 if frame != STRIKE else 0)
    weapon(c, hand, blade_angle, 10, B3, width=2)
    c.set(int(hand[0]), int(hand[1]), A1)
    head(c, x + 1, 25 + bob, 6, 'soldier')


def cavalry(c, frame):
    if frame == DEFEAT:
        c = c.shifted(2, 5)         # the champion slumps: the whole figure drops
    rear = frame == WINDUP
    gallop = frame == STRIKE
    tilt = -3 if frame == HURT else 0
    hx, hy = 30 + (4 if gallop else 0), 44
    # horse body and legs
    c.ellipse(hx, hy, 15, 7, B1)
    c.ellipse(hx + 2, hy + 2, 12, 4, B0)
    leg_front = 6 if gallop else (2 if not rear else -2)
    for dx, spread, shade in ((-10, -3, B0), (-6, 2, B1), (8, leg_front, B0), (12, leg_front + 3, B1)):
        top_y = hy + 4 - (6 if rear and dx > 0 else 0)
        line(c, hx + dx, top_y, hx + dx + spread, GROUND - 1 - (8 if rear and dx > 0 else 0), shade, 2)
    # neck and head (raised when rearing)
    neck_top = (hx + 17, hy - 14 - (4 if rear else 0) + tilt)
    line(c, hx + 11, hy - 3, neck_top[0], neck_top[1], B1, 5)
    c.ellipse(neck_top[0] + 3, neck_top[1] + 1, 5, 3, B2)
    c.set(neck_top[0] + 4, neck_top[1], DET)
    for i in range(4):
        c.set(neck_top[0] - 3 - i, neck_top[1] + 2 + i * 2, A1)           # mane
    line(c, hx - 15, hy - 2, hx - 21, hy + 6, A0, 2)                     # tail
    # caparison trim
    c.hline(hx - 12, hx + 10, hy + 3, A1)
    # rider
    rx, ry = hx - 2 + (3 if gallop else 0), hy - 16
    torso(c, rx, ry, 12, 10, B2, B0)
    head(c, rx + 1, ry - 6, 5, 'knight')
    angle = {IDLE: 70, BREATH: 70, WINDUP: 110, STRIKE: 5, HURT: 120, DEFEAT: 160}[frame]
    hand = arm(c, (rx, ry + 3), angle - 40, 6, B2, 2)
    weapon(c, hand, angle - 60 if frame == STRIKE else angle, 22, B3, (A2, 1), 2)


def mystic(c, frame):
    lean = {IDLE: 0, BREATH: 0, WINDUP: -2, STRIKE: 3, HURT: -4, DEFEAT: -3}[frame]
    bob = 1 if frame == BREATH else 0
    x = 28 + lean
    # long robe (no visible legs)
    c.polygon([(x - 10, GROUND - 1), (x - 6, 30 + bob), (x + 6, 30 + bob), (x + 10, GROUND - 1)], B2)
    c.polygon([(x + 3, 30 + bob), (x + 6, 30 + bob), (x + 10, GROUND - 1), (x + 5, GROUND - 1)], B0)
    c.vline(x, 32 + bob, GROUND - 2, A1)
    c.hline(x - 7, x + 8, GROUND - 2, A1)
    head(c, x + 1, 24 + bob, 6, 'mitre')
    # staff: raised for the wind-up, pointed for the strike
    angle = {IDLE: 80, BREATH: 80, WINDUP: 100, STRIKE: 20, HURT: 120, DEFEAT: 150}[frame]
    hand = arm(c, (x + 2, 33 + bob), 30 if frame != STRIKE else 10, 6, B2)
    top = weapon(c, (hand[0], hand[1] + 10), angle, 28 if frame != STRIKE else 20, A0, None, 2)
    c.circle(int(top[0]), int(top[1]), 3, S1 if frame != WINDUP else GEM)
    c.set(int(top[0]), int(top[1]), RIM)


def guardian(c, frame):
    lean = {IDLE: 0, BREATH: 0, WINDUP: -3, STRIKE: 4, HURT: -3, DEFEAT: -2}[frame]
    bob = 1 if frame == BREATH else 0
    drop = 6 if frame == DEFEAT else 0
    x = 30 + lean
    legs(c, x, 46 + bob + drop, 3, B1, B0)
    torso(c, x, 28 + bob + drop, 20, 19, B1, B0, A0)
    c.hline(x - 9, x + 8, 30 + bob + drop, B3)                          # pauldron rims
    head(c, x + 1, 21 + bob + drop, 6, 'great')
    # tower shield in front
    c.rect(x + 7, 27 + bob + drop, 8, 24, S0)
    c.rect(x + 8, 28 + bob + drop, 6, 22, S1)
    c.vline(x + 11, 29 + bob + drop, 48 + bob + drop, A1)
    # great hammer
    angle = {IDLE: 100, BREATH: 98, WINDUP: 125, STRIKE: 10, HURT: 130, DEFEAT: 235}[frame]
    hand = arm(c, (x - 5, 32 + bob + drop), angle, 9, B1, 4)
    end = weapon(c, hand, angle, 14, A0, None, 2)
    rad = math.radians(angle + 90)
    hx, hy = end
    c.thick_line(int(hx - math.cos(rad) * 5), int(hy + math.sin(rad) * 5), int(hx + math.cos(rad) * 5),
                 int(hy - math.sin(rad) * 5), B2, 5)


def sorceress(c, frame):
    lean = {IDLE: 0, BREATH: 0, WINDUP: -1, STRIKE: 2, HURT: -4, DEFEAT: -2}[frame]
    bob = 1 if frame == BREATH else 0
    x = 28 + lean
    # flowing gown
    c.polygon([(x - 11, GROUND - 1), (x - 4, 32 + bob), (x + 4, 32 + bob), (x + 12, GROUND - 1)], S1)
    c.polygon([(x + 2, 32 + bob), (x + 4, 32 + bob), (x + 12, GROUND - 1), (x + 6, GROUND - 1)], S0)
    for gx in range(x - 10, x + 12, 4):
        c.set(gx, GROUND - 2, A2)
    torso(c, x, 28 + bob, 10, 7, B2, B0, A1)
    head(c, x + 1, 22 + bob, 6, 'tiara')
    for i in range(6):
        c.set(x - 5, 22 + bob + i, A0)                                   # hair falling down the back
    # casting hand: forward and open for the strike
    angle = {IDLE: 20, BREATH: 25, WINDUP: 120, STRIKE: 0, HURT: 160, DEFEAT: 200}[frame]
    hand = arm(c, (x + 2, 30 + bob), angle, 9, B3, 2)
    wand_end = weapon(c, hand, angle + 10, 7, A1, None, 1)
    c.circle(int(wand_end[0]), int(wand_end[1]), 2 if frame in (WINDUP, STRIKE) else 1, GEM)


def sovereign(c, frame):
    lean = {IDLE: 0, BREATH: 0, WINDUP: -2, STRIKE: 4, HURT: -3, DEFEAT: -2}[frame]
    bob = 1 if frame == BREATH else 0
    x = 27 + lean
    # royal cape behind
    c.polygon([(x - 6, 29 + bob), (x - 13, GROUND - 1), (x + 1, GROUND - 1), (x + 2, 29 + bob)], A0)
    c.polygon([(x - 6, 29 + bob), (x - 11, GROUND - 3), (x - 8, GROUND - 3)], A1)
    legs(c, x, 46 + bob, 2, B1, B0)
    torso(c, x, 29 + bob, 15, 17, B2, B0, A1)
    c.vline(x, 30 + bob, 44 + bob, A2)
    head(c, x + 1, 22 + bob, 6, 'crown')
    angle = {IDLE: 70, BREATH: 68, WINDUP: 130, STRIKE: 0, HURT: 150, DEFEAT: 200}[frame]
    hand = arm(c, (x + 3, 32 + bob), angle - 20, 8, B2)
    weapon(c, hand, angle, 16, B3, None, 2)
    c.hline(int(hand[0]) - 2, int(hand[0]) + 2, int(hand[1]), A1)       # cross-guard


CLASSES = [('soldier', soldier), ('cavalry', cavalry), ('mystic', mystic), ('guardian', guardian),
           ('sorceress', sorceress), ('sovereign', sovereign)]

class FigureCanvas:
    """The drawing calls of the figure builders, forwarded to a canvas with an offset (a slumping figure)."""

    def __init__(self, canvas, dx=0, dy=0):
        self.canvas = canvas
        self.dx = dx
        self.dy = dy

    def shifted(self, dx, dy):
        return FigureCanvas(self.canvas, self.dx + dx, self.dy + dy)

    def set(self, x, y, color):
        self.canvas.set(int(x) + self.dx, int(y) + self.dy, color)

    def hline(self, x0, x1, y, color):
        self.canvas.hline(int(x0) + self.dx, int(x1) + self.dx, int(y) + self.dy, color)

    def vline(self, x, y0, y1, color):
        self.canvas.vline(int(x) + self.dx, int(y0) + self.dy, int(y1) + self.dy, color)

    def rect(self, x, y, w, h, color):
        self.canvas.rect(int(x) + self.dx, int(y) + self.dy, w, h, color)

    def circle(self, x, y, r, color, filled=True):
        self.canvas.circle(int(x) + self.dx, int(y) + self.dy, r, color, filled)

    def ellipse(self, x, y, rx, ry, color, filled=True):
        self.canvas.ellipse(int(x) + self.dx, int(y) + self.dy, rx, ry, color, filled)

    def polygon(self, points, color):
        self.canvas.polygon([(int(x) + self.dx, int(y) + self.dy) for x, y in points], color)

    def line(self, x0, y0, x1, y1, color):
        self.canvas.line(int(x0) + self.dx, int(y0) + self.dy, int(x1) + self.dx, int(y1) + self.dy, color)

    def thick_line(self, x0, y0, x1, y1, color, thickness=2):
        self.canvas.thick_line(int(x0) + self.dx, int(y0) + self.dy, int(x1) + self.dx, int(y1) + self.dy, color,
                               thickness)


def build_figure(builder, frame):
    """One 64x64 frame of a class."""
    target = Canvas(64, 64)
    builder(FigureCanvas(target), frame)
    return target


def sheet_for(builder):
    sheet = Canvas(64, 64 * 6)
    for frame in range(6):
        figure = build_figure(builder, frame)
        figure.outline(OUT)
        sheet.blit(figure, 0, frame * 64)
    return sheet


# Battle stage: a flagstone floor strip with a gold trim (repeated across the screen) and the actors' shadows.
STAGE_PAL = [P.TRANSPARENT, (14, 10, 24), (40, 36, 56), (62, 58, 84), (92, 88, 118), (124, 120, 150),
             (232, 196, 104), (150, 112, 48), (26, 22, 38)] + [(0, 0, 0)] * 7


def stage_floor():
    """32x32: rows 0-1 trim, two courses of flagstones (8 and 10 px, the nearer one taller), then shade."""
    c = Canvas(32, 32)
    c.hline(0, 31, 0, 6)
    c.hline(0, 31, 1, 7)
    for top, height, offset in ((2, 8, 0), (10, 10, 8)):
        for x0 in range(-16 + offset, 32, 16):
            for y in range(top, top + height):
                for x in range(max(x0, 0), min(x0 + 16, 32)):
                    edge_x = x == x0 or x == x0 + 15
                    edge_y = y == top + height - 1
                    if edge_x or edge_y:
                        color = 1
                    elif y == top or x == x0 + 1:
                        color = 4 if top == 2 else 5
                    else:
                        color = 3 if (x * 7 + y * 3) % 11 else 2
                    c.set(x, y, color)
    for y in range(20, 32):
        for x in range(32):
            c.set(x, y, 8 if (x + y) % 2 else 1)
    return c


def stage_shadow():
    """32x8 dark oval under an actor's feet."""
    c = Canvas(32, 8)
    for y in range(8):
        for x in range(32):
            d = ((x - 15.5) / 13.5) ** 2 + ((y - 3.5) / 3.2) ** 2
            if d <= 0.55 or (d <= 1.0 and (x + y) % 2 == 0):
                c.set(x, y, 1)
    return c


def generate(root):
    graphics = os.path.join(root, 'graphics')
    write_bmp(os.path.join(graphics, 'battle_floor.bmp'), stage_floor(), STAGE_PAL, bpp=4)
    write_json(os.path.join(graphics, 'battle_floor.json'), {'type': 'sprite', 'height': 32})
    write_bmp(os.path.join(graphics, 'battle_shadow.bmp'), stage_shadow(), STAGE_PAL, bpp=4)
    write_json(os.path.join(graphics, 'battle_shadow.json'), {'type': 'sprite', 'height': 8})
    sheets = []
    for name, builder in CLASSES:
        sheet = sheet_for(builder)
        write_bmp(os.path.join(graphics, 'battle_' + name + '.bmp'), sheet, P.ARMY_WHITE, bpp=4)
        write_json(os.path.join(graphics, 'battle_' + name + '.json'), {'type': 'sprite', 'height': 64})
        sheets.append(sheet)

    # Preview: every class and frame, white army on top, dark army below.
    width, height = 6 * 66, 12 * 66
    background = (60, 70, 96)
    rows = [[background] * width for _ in range(height)]
    for army, palette in enumerate((P.ARMY_WHITE, P.ARMY_BLACK)):
        for i, sheet in enumerate(sheets):
            for frame in range(6):
                for y in range(64):
                    for x in range(64):
                        index = sheet.get(x, frame * 64 + y)
                        if index:
                            rows[(army * 6 + i) * 66 + 1 + y][frame * 66 + 1 + x] = palette[index]
    write_png_rgb(os.path.join(root, 'assets_source', 'previews', 'battle.png'), width, height, rows, scale=1)


if __name__ == '__main__':
    generate(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    print('battle characters generated')
