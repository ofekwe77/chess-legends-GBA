"""Combines emulator screenshots (PNG, any size) into one contact sheet for visual review.

Usage: py -3.12 tools/contact_sheet.py OUT.png IN1.png [IN2.png ...] [--cols N] [--scale S]
Standard library only (zlib PNG decoding of 8-bit RGB / RGBA, all filter types).
"""
import struct
import sys
import zlib


def read_png(path):
    data = open(path, 'rb').read()
    pos = 8
    width = height = color_type = 0
    idat = b''
    while pos < len(data):
        length = struct.unpack('>I', data[pos:pos + 4])[0]
        tag = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + length]
        pos += 12 + length
        if tag == b'IHDR':
            width, height, _, color_type = struct.unpack('>IIBB', body[:10])
        elif tag == b'IDAT':
            idat += body
    bpp = 3 if color_type == 2 else 4
    raw = zlib.decompress(idat)
    stride = width * bpp
    rows = []
    previous = bytearray(stride)
    for y in range(height):
        kind = raw[y * (stride + 1)]
        line = bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - bpp] if i >= bpp else 0
            b = previous[i]
            c = previous[i - bpp] if i >= bpp else 0
            if kind == 1:
                line[i] = (line[i] + a) & 255
            elif kind == 2:
                line[i] = (line[i] + b) & 255
            elif kind == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif kind == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[i] = (line[i] + (a if pa <= pb and pa <= pc else (b if pb <= pc else c))) & 255
        rows.append([tuple(line[x * bpp:x * bpp + 3]) for x in range(width)])
        previous = line
    return width, height, rows


def write_png(path, width, height, rows):
    raw = b''.join(b'\x00' + bytes(v for pixel in row for v in pixel) for row in rows)

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body) & 0xFFFFFFFF)

    header = struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0)
    with open(path, 'wb') as f:
        f.write(b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', header) + chunk(b'IDAT', zlib.compress(raw, 9)) +
                chunk(b'IEND', b''))


def main(argv):
    cols, scale, files = 2, 2, []
    i = 0
    while i < len(argv):
        if argv[i] == '--cols':
            cols = int(argv[i + 1])
            i += 2
        elif argv[i] == '--scale':
            scale = int(argv[i + 1])
            i += 2
        else:
            files.append(argv[i])
            i += 1
    if len(files) < 2:
        print(__doc__)
        return 2
    out, inputs = files[0], [read_png(path) for path in files[1:]]
    cell_w = max(w for w, _, _ in inputs) * scale
    cell_h = max(h for _, h, _ in inputs) * scale
    rows_count = (len(inputs) + cols - 1) // cols
    sheet = [[(0, 0, 0)] * (cell_w * cols) for _ in range(cell_h * rows_count)]
    for index, (w, h, rows) in enumerate(inputs):
        ox, oy = (index % cols) * cell_w, (index // cols) * cell_h
        for y in range(h * scale):
            source = rows[y // scale]
            target = sheet[oy + y]
            for x in range(w * scale):
                target[ox + x] = source[x // scale]
    write_png(out, cell_w * cols, cell_h * rows_count, sheet)
    return 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
