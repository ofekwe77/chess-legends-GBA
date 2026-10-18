"""Indexed-color pixel canvas with the drawing primitives used by every CHESS LEGENDS art generator.

Pixels are palette indices (0-255). Index 0 is the transparent color for sprites. All drawing is exact,
pixel-level and deterministic: generators build intentional pixel art from shapes, hand-written ASCII
templates and controlled shading, never from random noise.
"""


class Canvas:
    def __init__(self, width, height, fill=0):
        self.width = width
        self.height = height
        self.pixels = bytearray([fill]) * (width * height)

    # ------------------------------------------------------------------ basics
    def copy(self):
        result = Canvas(self.width, self.height)
        result.pixels[:] = self.pixels
        return result

    def inside(self, x, y):
        return 0 <= x < self.width and 0 <= y < self.height

    def get(self, x, y, default=0):
        if 0 <= x < self.width and 0 <= y < self.height:
            return self.pixels[y * self.width + x]
        return default

    def set(self, x, y, color):
        if 0 <= x < self.width and 0 <= y < self.height:
            self.pixels[y * self.width + x] = color

    def set_if(self, x, y, color, only_on=None):
        """Sets a pixel only when the current value is in only_on (a set) or, if None, only when opaque."""
        if not self.inside(x, y):
            return
        current = self.pixels[y * self.width + x]
        if only_on is None:
            if current != 0:
                self.pixels[y * self.width + x] = color
        elif current in only_on:
            self.pixels[y * self.width + x] = color

    def fill(self, color):
        for i in range(len(self.pixels)):
            self.pixels[i] = color

    def rect(self, x, y, w, h, color):
        for yy in range(max(0, y), min(self.height, y + h)):
            row = yy * self.width
            for xx in range(max(0, x), min(self.width, x + w)):
                self.pixels[row + xx] = color

    def frame(self, x, y, w, h, color):
        for xx in range(x, x + w):
            self.set(xx, y, color)
            self.set(xx, y + h - 1, color)
        for yy in range(y, y + h):
            self.set(x, yy, color)
            self.set(x + w - 1, yy, color)

    def hline(self, x0, x1, y, color):
        if x0 > x1:
            x0, x1 = x1, x0
        for x in range(x0, x1 + 1):
            self.set(x, y, color)

    def vline(self, x, y0, y1, color):
        if y0 > y1:
            y0, y1 = y1, y0
        for y in range(y0, y1 + 1):
            self.set(x, y, color)

    def line(self, x0, y0, x1, y1, color):
        dx = abs(x1 - x0)
        dy = -abs(y1 - y0)
        sx = 1 if x0 < x1 else -1
        sy = 1 if y0 < y1 else -1
        err = dx + dy
        while True:
            self.set(x0, y0, color)
            if x0 == x1 and y0 == y1:
                break
            e2 = 2 * err
            if e2 >= dy:
                err += dy
                x0 += sx
            if e2 <= dx:
                err += dx
                y0 += sy

    def thick_line(self, x0, y0, x1, y1, color, thickness=2):
        r = thickness / 2.0
        for oy in range(-thickness, thickness + 1):
            for ox in range(-thickness, thickness + 1):
                if ox * ox + oy * oy <= r * r + 0.25:
                    self.line(x0 + ox, y0 + oy, x1 + ox, y1 + oy, color)

    # ------------------------------------------------------------------ shapes
    def ellipse(self, cx, cy, rx, ry, color, filled=True):
        """Ellipse centred on (cx, cy) (may be half-integers) with radii rx, ry."""
        if rx <= 0 or ry <= 0:
            return
        x_min = int(cx - rx - 1)
        x_max = int(cx + rx + 1)
        y_min = int(cy - ry - 1)
        y_max = int(cy + ry + 1)
        for y in range(y_min, y_max + 1):
            for x in range(x_min, x_max + 1):
                nx = (x + 0.5 - cx) / rx
                ny = (y + 0.5 - cy) / ry
                d = nx * nx + ny * ny
                if filled:
                    if d <= 1.0:
                        self.set(x, y, color)
                else:
                    # pixel on the boundary: inside, but a 4-neighbour is outside
                    if d <= 1.0:
                        outside = False
                        for ox, oy in ((1, 0), (-1, 0), (0, 1), (0, -1)):
                            mx = (x + ox + 0.5 - cx) / rx
                            my = (y + oy + 0.5 - cy) / ry
                            if mx * mx + my * my > 1.0:
                                outside = True
                                break
                        if outside:
                            self.set(x, y, color)

    def circle(self, cx, cy, r, color, filled=True):
        self.ellipse(cx, cy, r, r, color, filled)

    def polygon(self, points, color):
        """Even-odd scanline fill of a polygon given as [(x, y), ...] (pixel centres)."""
        if len(points) < 3:
            return
        ys = [p[1] for p in points]
        y_min = int(min(ys))
        y_max = int(max(ys)) + 1
        n = len(points)
        for y in range(y_min, y_max + 1):
            sy = y + 0.5
            xs = []
            for i in range(n):
                x0, y0 = points[i]
                x1, y1 = points[(i + 1) % n]
                if (y0 <= sy < y1) or (y1 <= sy < y0):
                    t = (sy - y0) / (y1 - y0)
                    xs.append(x0 + t * (x1 - x0))
            xs.sort()
            for i in range(0, len(xs) - 1, 2):
                xa = int(round(xs[i]))
                xb = int(round(xs[i + 1])) - 1
                for x in range(xa, xb + 1):
                    self.set(x, y, color)

    def polyline(self, points, color):
        for i in range(len(points) - 1):
            self.line(int(round(points[i][0])), int(round(points[i][1])),
                      int(round(points[i + 1][0])), int(round(points[i + 1][1])), color)

    def flood(self, x, y, color):
        if not self.inside(x, y):
            return
        target = self.get(x, y)
        if target == color:
            return
        stack = [(x, y)]
        while stack:
            px, py = stack.pop()
            if not self.inside(px, py) or self.get(px, py) != target:
                continue
            self.set(px, py, color)
            stack.extend(((px + 1, py), (px - 1, py), (px, py + 1), (px, py - 1)))

    # ------------------------------------------------------------------ composition
    def blit(self, src, x, y, transparent=0, flip_h=False, flip_v=False, remap=None):
        for sy in range(src.height):
            for sx in range(src.width):
                c = src.pixels[sy * src.width + sx]
                if c == transparent:
                    continue
                if remap is not None:
                    c = remap.get(c, c)
                tx = x + (src.width - 1 - sx if flip_h else sx)
                ty = y + (src.height - 1 - sy if flip_v else sy)
                self.set(tx, ty, c)

    def crop(self, x, y, w, h):
        result = Canvas(w, h)
        for yy in range(h):
            for xx in range(w):
                result.pixels[yy * w + xx] = self.get(x + xx, y + yy)
        return result

    def flipped_h(self):
        result = Canvas(self.width, self.height)
        result.blit(self, 0, 0, transparent=-1, flip_h=True)
        return result

    def replace(self, old, new):
        for i, c in enumerate(self.pixels):
            if c == old:
                self.pixels[i] = new

    def remap(self, mapping):
        for i, c in enumerate(self.pixels):
            if c in mapping:
                self.pixels[i] = mapping[c]

    def opaque(self, x, y):
        return self.get(x, y) != 0

    def outline(self, color, diagonal=False, only_outside=True):
        """Adds a 1-pixel outline of `color` around every opaque pixel (on transparent pixels)."""
        result = self.copy()
        neighbours = [(1, 0), (-1, 0), (0, 1), (0, -1)]
        if diagonal:
            neighbours += [(1, 1), (-1, 1), (1, -1), (-1, -1)]
        for y in range(self.height):
            for x in range(self.width):
                if self.get(x, y) != 0:
                    continue
                for ox, oy in neighbours:
                    if self.get(x + ox, y + oy) != 0:
                        result.set(x, y, color)
                        break
        self.pixels[:] = result.pixels

    def inner_edge(self, color, side='top-left', protect=None):
        """Recolors opaque pixels that border transparency on the given side (rim lighting / shading)."""
        offsets = {
            'top-left': [(-1, 0), (0, -1)],
            'bottom-right': [(1, 0), (0, 1)],
            'top': [(0, -1)],
            'bottom': [(0, 1)],
            'left': [(-1, 0)],
            'right': [(1, 0)],
        }[side]
        result = self.copy()
        for y in range(self.height):
            for x in range(self.width):
                c = self.get(x, y)
                if c == 0 or (protect and c in protect):
                    continue
                for ox, oy in offsets:
                    if self.get(x + ox, y + oy) == 0:
                        result.set(x, y, color)
                        break
        self.pixels[:] = result.pixels

    # ------------------------------------------------------------------ templates
    @staticmethod
    def from_ascii(rows, mapping):
        """Builds a canvas from a list of equal-length strings; mapping: char -> palette index ('.' = 0)."""
        height = len(rows)
        width = max(len(r) for r in rows)
        result = Canvas(width, height)
        for y, row in enumerate(rows):
            for x, ch in enumerate(row):
                if ch in mapping:
                    result.set(x, y, mapping[ch])
                elif ch not in '. ':
                    raise ValueError('unmapped template character %r' % ch)
        return result


# ---------------------------------------------------------------------- shading helpers
BAYER4 = (
    (0, 8, 2, 10),
    (12, 4, 14, 6),
    (3, 11, 1, 9),
    (15, 7, 13, 5),
)


def dither_pick(x, y, t, a, b):
    """Ordered dither between colors a (t=0) and b (t=1)."""
    threshold = (BAYER4[y & 3][x & 3] + 0.5) / 16.0
    return b if t > threshold else a


def vertical_gradient(canvas, x, y, w, h, colors, dither=True):
    """Fills a rectangle with a vertical gradient through the given color list."""
    steps = len(colors) - 1
    for yy in range(h):
        pos = yy / max(1, h - 1) * steps
        i = min(int(pos), steps - 1) if steps > 0 else 0
        t = pos - i
        for xx in range(w):
            if steps == 0:
                c = colors[0]
            elif dither:
                c = dither_pick(x + xx, y + yy, t, colors[i], colors[i + 1])
            else:
                c = colors[i] if t < 0.5 else colors[i + 1]
            canvas.set(x + xx, y + yy, c)
