"""Nearest-neighbour upscaler for emulator screenshots (inspection aid). Usage: upscale.py in.png out.png [scale]"""
import struct
import sys
import zlib


def read_png(path):
    data = open(path, 'rb').read()
    pos = 8
    width = height = 0
    idat = b''
    while pos < len(data):
        length = struct.unpack('>I', data[pos:pos + 4])[0]
        tag = data[pos + 4:pos + 8]
        body = data[pos + 8:pos + 8 + length]
        if tag == b'IHDR':
            width, height = struct.unpack('>II', body[:8])
        elif tag == b'IDAT':
            idat += body
        pos += 12 + length
    raw = zlib.decompress(idat)
    rows = []
    stride = width * 3
    prev = bytearray(stride)
    p = 0
    for _ in range(height):
        f = raw[p]
        line = bytearray(raw[p + 1:p + 1 + stride])
        p += 1 + stride
        if f == 1:
            for i in range(3, stride):
                line[i] = (line[i] + line[i - 3]) & 255
        elif f == 2:
            for i in range(stride):
                line[i] = (line[i] + prev[i]) & 255
        elif f == 3:
            for i in range(stride):
                left = line[i - 3] if i >= 3 else 0
                line[i] = (line[i] + (left + prev[i]) // 2) & 255
        elif f == 4:
            for i in range(stride):
                a = line[i - 3] if i >= 3 else 0
                b = prev[i]
                c = prev[i - 3] if i >= 3 else 0
                pa, pb, pc = abs(b - c), abs(a - c), abs(a + b - 2 * c)
                pred = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[i] = (line[i] + pred) & 255
        rows.append(line)
        prev = line
    return width, height, rows


def write_png(path, width, height, rows):
    raw = bytearray()
    for r in rows:
        raw.append(0)
        raw += r

    def chunk(tag, body):
        return struct.pack('>I', len(body)) + tag + body + struct.pack('>I', zlib.crc32(tag + body) & 0xFFFFFFFF)

    out = b'\x89PNG\r\n\x1a\n' + chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
    out += chunk(b'IDAT', zlib.compress(bytes(raw), 9)) + chunk(b'IEND', b'')
    open(path, 'wb').write(out)


def main():
    src, dst = sys.argv[1], sys.argv[2]
    scale = int(sys.argv[3]) if len(sys.argv) > 3 else 3
    crop = None
    if len(sys.argv) > 7:
        crop = tuple(int(v) for v in sys.argv[4:8])
    w, h, rows = read_png(src)
    if crop:
        x0, y0, cw, ch = crop
        rows = [r[x0 * 3:(x0 + cw) * 3] for r in rows[y0:y0 + ch]]
        w, h = cw, ch
    out = []
    for r in rows:
        line = bytearray()
        for x in range(w):
            line += r[x * 3:x * 3 + 3] * scale
        for _ in range(scale):
            out.append(line)
    write_png(dst, w * scale, h * scale, out)


if __name__ == '__main__':
    main()
