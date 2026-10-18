"""Indexed BMP (Butano input) and PNG (preview) writers, standard library only."""

import json
import os
import struct
import zlib


def gba_color(rgb):
    """Rounds an (r, g, b) 0-255 color to the 15-bit GBA grid (multiples of 8) so previews match the console."""
    return tuple((c >> 3) << 3 for c in rgb)


def write_bmp(path, canvas, palette, bpp=4):
    """Writes an indexed BMP. bpp 4 needs <= 16 colors, bpp 8 <= 256. palette: list of (r, g, b)."""
    width, height = canvas.width, canvas.height
    colors = 16 if bpp == 4 else 256
    pal = [gba_color(c) for c in palette] + [(0, 0, 0)] * (colors - len(palette))
    if len(pal) > colors:
        raise ValueError('%s: too many colors for %d bpp' % (path, bpp))
    if bpp == 4:
        row_bytes = (width + 1) // 2
    else:
        row_bytes = width
    padded = (row_bytes + 3) & ~3
    pixel_data = bytearray()
    for y in range(height - 1, -1, -1):
        row = bytearray()
        if bpp == 4:
            for x in range(0, width, 2):
                a = canvas.get(x, y)
                b = canvas.get(x + 1, y) if x + 1 < width else 0
                if a > 15 or b > 15:
                    raise ValueError('%s: index above 15 in a 4 bpp image at (%d, %d)' % (path, x, y))
                row.append((a << 4) | b)
        else:
            for x in range(width):
                row.append(canvas.get(x, y))
        row += b'\0' * (padded - len(row))
        pixel_data += row
    header_size = 14 + 40 + colors * 4
    file_size = header_size + len(pixel_data)
    out = bytearray()
    out += b'BM' + struct.pack('<IHHI', file_size, 0, 0, header_size)
    out += struct.pack('<IiiHHIIiiII', 40, width, height, 1, bpp, 0, len(pixel_data), 2835, 2835, colors, colors)
    for r, g, b in pal:
        out += bytes((b, g, r, 0))
    out += pixel_data
    os.makedirs(os.path.dirname(path) or '.', exist_ok=True)
    with open(path, 'wb') as f:
        f.write(out)


def write_json(path, data):
    with open(path, 'w', newline='\n') as f:
        json.dump(data, f, indent=4)
        f.write('\n')


def _png_chunk(tag, data):
    return struct.pack('>I', len(data)) + tag + data + struct.pack('>I', zlib.crc32(tag + data) & 0xFFFFFFFF)


def write_png(path, canvas, palette, scale=1, transparent_color=(255, 0, 255)):
    """Writes an RGB preview PNG (index 0 shown as magenta unless palette[0] is used as background)."""
    pal = [gba_color(c) for c in palette]
    w, h = canvas.width * scale, canvas.height * scale
    raw = bytearray()
    for y in range(h):
        raw.append(0)
        sy = y // scale
        for x in range(w):
            idx = canvas.get(x // scale, sy)
            if idx == 0 and transparent_color is not None:
                raw += bytes(transparent_color)
            else:
                raw += bytes(pal[idx] if idx < len(pal) else (255, 0, 255))
    png = b'\x89PNG\r\n\x1a\n'
    png += _png_chunk(b'IHDR', struct.pack('>IIBBBBB', w, h, 8, 2, 0, 0, 0))
    png += _png_chunk(b'IDAT', zlib.compress(bytes(raw), 9))
    png += _png_chunk(b'IEND', b'')
    os.makedirs(os.path.dirname(path) or '.', exist_ok=True)
    with open(path, 'wb') as f:
        f.write(png)


def write_png_rgb(path, width, height, rgb_rows, scale=1):
    """Writes a PNG from rows of (r, g, b) tuples (used for composite previews)."""
    raw = bytearray()
    for y in range(height * scale):
        raw.append(0)
        row = rgb_rows[y // scale]
        for x in range(width * scale):
            raw += bytes(gba_color(row[x // scale]))
    png = b'\x89PNG\r\n\x1a\n'
    png += _png_chunk(b'IHDR', struct.pack('>IIBBBBB', width * scale, height * scale, 8, 2, 0, 0, 0))
    png += _png_chunk(b'IDAT', zlib.compress(bytes(raw), 9))
    png += _png_chunk(b'IEND', b'')
    os.makedirs(os.path.dirname(path) or '.', exist_ok=True)
    with open(path, 'wb') as f:
        f.write(png)
