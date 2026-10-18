"""Structural check of the generated XM modules (independent re-parse of every field).

For each module: header magic / version / sizes, order table bounds, every pattern's packed data decoded to exactly
rows x channels cells with valid notes (1-96, 97 note off) and instrument numbers, every instrument header and
sample length consistent with the file. Also reports per-module statistics (notes per channel, length in seconds).
Exit code 1 on any inconsistency.
"""
import os
import struct
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def check(path):
    data = open(path, 'rb').read()
    problems = []
    if data[:17] != b'Extended Module: ' or data[37] != 0x1A:
        return ['bad magic']
    version = struct.unpack_from('<H', data, 58)[0]
    header_size, length, restart, channels, patterns, instruments, flags, speed, bpm = struct.unpack_from(
        '<IHHHHHHHH', data, 60)
    order = list(data[80:80 + length])
    if version != 0x0104 or header_size != 276:
        problems.append('version / header size')
    if restart >= length:
        problems.append('restart beyond song length')
    if any(o >= patterns for o in order):
        problems.append('order references a missing pattern')
    pos = 60 + header_size
    notes_per_channel = [0] * channels
    rows_total = 0
    pattern_rows = []
    for p in range(patterns):
        plen, packing, rows, size = struct.unpack_from('<IBHH', data, pos)
        pos += plen
        end = pos + size
        cells = 0
        channel = 0
        while pos < end:
            flags = data[pos]
            pos += 1
            fields = {}
            if flags & 0x80:
                for bit, key in ((1, 'note'), (2, 'instrument'), (4, 'volume'), (8, 'effect'), (16, 'param')):
                    if flags & bit:
                        fields[key] = data[pos]
                        pos += 1
            else:
                fields['note'] = flags
                fields['instrument'], fields['volume'], fields['effect'], fields['param'] = data[pos:pos + 4]
                pos += 4
            note = fields.get('note', 0)
            if note and not (1 <= note <= 97):
                problems.append('pattern %d: invalid note %d' % (p, note))
            if fields.get('instrument', 0) > instruments:
                problems.append('pattern %d: invalid instrument %d' % (p, fields['instrument']))
            if 1 <= note <= 96:
                notes_per_channel[channel] += 1
            cells += 1
            channel = (channel + 1) % channels
        if pos != end or cells != rows * channels:
            problems.append('pattern %d: %d cells for %d rows x %d channels' % (p, cells, rows, channels))
        pattern_rows.append(rows)
    for o in order:
        rows_total += pattern_rows[o]
    for i in range(instruments):
        size = struct.unpack_from('<I', data, pos)[0]
        samples = struct.unpack_from('<H', data, pos + 27)[0]
        if samples != 1 or size != 263:
            problems.append('instrument %d: %d samples, header %d' % (i + 1, samples, size))
        pos += size
        sample_length, loop_start, loop_length = struct.unpack_from('<III', data, pos)
        if loop_start + loop_length > sample_length:
            problems.append('instrument %d: loop beyond the sample' % (i + 1))
        pos += 40 + sample_length
    if pos != len(data):
        problems.append('%d trailing / missing bytes' % (len(data) - pos))
    seconds = rows_total * speed * 2.5 / bpm
    return problems, channels, patterns, instruments, length, notes_per_channel, seconds


def main():
    folder = os.path.join(ROOT, 'audio')
    failures = 0
    for name in sorted(os.listdir(folder)):
        if not name.endswith('.xm'):
            continue
        result = check(os.path.join(folder, name))
        if len(result) == 1:
            print('%-16s FAIL %s' % (name, result[0]))
            failures += 1
            continue
        problems, channels, patterns, instruments, length, notes, seconds = result
        status = 'ok' if not problems else 'FAIL ' + '; '.join(problems)
        failures += bool(problems)
        print('%-16s %s  %d ch, %d patterns, %d orders, %d instruments, %4.1f s per loop, notes/channel %s'
              % (name, status, channels, patterns, length, instruments, seconds, notes))
    print('xm check: %d failure(s)' % failures)
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
