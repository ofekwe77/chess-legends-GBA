"""Minimal FastTracker II extended module (XM, version 1.04) writer used by gen_audio.py.

Only the features the soundtrack needs: patterns with notes / instruments / volume / effects (packed), and
instruments with one 8-bit sample each, a volume envelope, auto-vibrato and fadeout. Output follows the published
XM specification exactly, so mmutil (maxmod) and every XM player can read it.
"""
import struct

NOTE_OFF = 97


def note_value(name):
    """'C-4' / 'C#4' / 'Db4' -> XM note number (C-0 = 1)."""
    names = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
    letter = name[0].upper()
    rest = name[1:]
    semitone = names[letter]
    if rest and rest[0] in '#b-':
        if rest[0] == '#':
            semitone += 1
        elif rest[0] == 'b':
            semitone -= 1
        rest = rest[1:]
    octave = int(rest)
    value = octave * 12 + semitone + 1
    assert 1 <= value <= 96, name
    return value


class Cell:
    __slots__ = ('note', 'instrument', 'volume', 'effect', 'param')

    def __init__(self, note=0, instrument=0, volume=0, effect=0, param=0):
        self.note = note
        self.instrument = instrument
        self.volume = volume        # 0 = none, otherwise 0x10 + (0..64)
        self.effect = effect
        self.param = param

    def empty(self):
        return not (self.note or self.instrument or self.volume or self.effect or self.param)


class Pattern:
    def __init__(self, rows, channels):
        self.rows = rows
        self.channels = channels
        self.cells = [[Cell() for _ in range(channels)] for _ in range(rows)]

    def set(self, row, channel, note=0, instrument=0, volume=None, effect=0, param=0):
        cell = self.cells[row][channel]
        cell.note = note
        cell.instrument = instrument
        cell.volume = 0 if volume is None else 0x10 + max(0, min(64, volume))
        cell.effect = effect
        cell.param = param

    def packed(self):
        out = bytearray()
        for row in self.cells:
            for cell in row:
                if cell.empty():
                    out.append(0x80)
                    continue
                flags = 0x80
                fields = bytearray()
                if cell.note:
                    flags |= 1
                    fields.append(cell.note)
                if cell.instrument:
                    flags |= 2
                    fields.append(cell.instrument)
                if cell.volume:
                    flags |= 4
                    fields.append(cell.volume)
                if cell.effect:
                    flags |= 8
                    fields.append(cell.effect)
                if cell.param:
                    flags |= 16
                    fields.append(cell.param)
                out.append(flags)
                out += fields
        return bytes(out)


class Instrument:
    def __init__(self, name, data, loop_start=0, loop_length=0, volume=64, relative_note=0, finetune=0,
                 panning=128, envelope=None, sustain=None, fadeout=0, vibrato=(0, 0, 0, 0)):
        """data: list of ints -128..127. envelope: list of (tick, level 0-64); sustain: index of the sustain point."""
        self.name = name
        self.data = data
        self.loop_start = loop_start
        self.loop_length = loop_length
        self.volume = volume
        self.relative_note = relative_note
        self.finetune = finetune
        self.panning = panning
        self.envelope = envelope or []
        self.sustain = sustain
        self.fadeout = fadeout
        self.vibrato = vibrato      # (type, sweep, depth, rate)

    def encode(self):
        out = bytearray()
        name = self.name.encode('ascii')[:22].ljust(22, b'\0')
        header = bytearray()
        header += name
        header.append(0)                         # type
        header += struct.pack('<H', 1)          # one sample
        header += struct.pack('<I', 40)         # sample header size
        header += bytes(96)                      # keymap: every note -> sample 0
        points = bytearray()
        for tick, level in self.envelope[:12]:
            points += struct.pack('<HH', tick, level)
        points = points.ljust(48, b'\0')
        header += points
        header += bytes(48)                      # panning envelope
        header.append(len(self.envelope[:12]))
        header.append(0)                         # panning points
        header.append(self.sustain if self.sustain is not None else 0)
        header.append(0)                         # volume loop start
        header.append(0)                         # volume loop end
        header += bytes(3)                       # panning sustain / loop
        volume_type = 0
        if self.envelope:
            volume_type |= 1
            if self.sustain is not None:
                volume_type |= 2
        header.append(volume_type)
        header.append(0)                         # panning type
        header += bytes(self.vibrato)
        header += struct.pack('<H', self.fadeout)
        header += bytes(22)                      # reserved
        out += struct.pack('<I', 4 + len(header))
        out += header
        assert 4 + len(header) == 263, len(header)

        loop = 1 if self.loop_length else 0
        sample = bytearray()
        sample += struct.pack('<III', len(self.data), self.loop_start, self.loop_length)
        sample.append(self.volume)
        sample += struct.pack('<b', self.finetune)
        sample.append(loop)                      # 8-bit, forward loop or none
        sample.append(self.panning)
        sample += struct.pack('<b', self.relative_note)
        sample.append(0)
        sample += self.name.encode('ascii')[:22].ljust(22, b'\0')
        out += sample
        previous = 0
        for value in self.data:                  # delta encoding
            value = max(-128, min(127, int(value)))
            out += struct.pack('<b', ((value - previous + 128) % 256) - 128)
            previous = value
        return bytes(out)


class Module:
    def __init__(self, name, channels, speed, bpm):
        assert channels % 2 == 0 and channels <= 32
        self.name = name
        self.channels = channels
        self.speed = speed
        self.bpm = bpm
        self.patterns = []
        self.order = []
        self.restart = 0
        self.instruments = []

    def add_pattern(self, pattern):
        self.patterns.append(pattern)
        return len(self.patterns) - 1

    def add_instrument(self, instrument):
        self.instruments.append(instrument)
        return len(self.instruments)            # XM instrument numbers start at 1

    def encode(self):
        out = bytearray()
        out += b'Extended Module: '
        out += self.name.encode('ascii')[:20].ljust(20, b' ')
        out.append(0x1A)
        out += b'gen_audio.py'.ljust(20, b' ')
        out += struct.pack('<H', 0x0104)
        order = self.order or [0]
        out += struct.pack('<IHHHHHHHH', 276, len(order), self.restart, self.channels, len(self.patterns),
                           len(self.instruments), 1, self.speed, self.bpm)
        out += bytes(order).ljust(256, b'\0')
        for pattern in self.patterns:
            data = pattern.packed()
            out += struct.pack('<IBHH', 9, 0, pattern.rows, len(data))
            out += data
        for instrument in self.instruments:
            out += instrument.encode()
        return bytes(out)

    def save(self, path):
        with open(path, 'wb') as f:
            f.write(self.encode())
