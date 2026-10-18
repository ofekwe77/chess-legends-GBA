"""Original soundtrack and sound effects of CHESS LEGENDS: ARCANE WARS, generated deterministically.

Instruments are synthesized (single-cycle waveforms with volume envelopes, one-shot drums) and written into genuine
FastTracker II XM modules (assets_source/audio/xm.py) that maxmod converts for the GBA; sound effects are WAV files.

The thirteen themes share motifs: the CROWN motif (D minor: D-A-D-F-E, D-C-A-G-A) opens the title theme and returns
transformed in the world map (major), the Crown Nexus (augmented, choral), the boss battle (its rhythm) and the
ending (major, slow). Every background theme has at least two contrasting sections and loops at a section start.
"""
import math
import os
import struct
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'audio'))

from xm import Instrument, Module, NOTE_OFF, Pattern, note_value  # noqa: E402

TAU = 2 * math.pi
DRUM_RATE = 16726           # drums play at C-4 with relative note +12
SFX_RATE = 16000

# ----------------------------------------------------------------------------------------------- synthesis


class Noise:
    """Deterministic white noise (LCG)."""

    def __init__(self, seed=12345):
        self.state = seed

    def next(self):
        self.state = (self.state * 1103515245 + 12345) & 0x7FFFFFFF
        return (self.state >> 15) / 32767.5 - 1.0


def normalize(values, peak=104):
    top = max(1e-9, max(abs(v) for v in values))
    return [int(round(v / top * peak)) for v in values]


def cycle(harmonics, length=128):
    """One band-limited cycle from (multiple, amplitude) pairs."""
    return normalize([sum(a * math.sin(TAU * m * i / length) for m, a in harmonics) for i in range(length)])


def saw_harmonics(count=14, bright=1.0):
    return [(k, (1.0 / k) ** (1.0 / bright)) for k in range(1, count + 1)]


def square_harmonics(count=13):
    return [(k, 1.0 / k) for k in range(1, count + 1, 2)]


def melodic(name, harmonics, envelope, sustain=None, fadeout=0, volume=48, vibrato=(0, 0, 0, 0)):
    data = cycle(harmonics)
    # 128-sample cycles sound at C-2 when played at the C-4 rate: +24 semitones restores the written pitch.
    return Instrument(name, data, 0, len(data), volume, 24, 0, 128, envelope, sustain, fadeout, vibrato)


def drum(name, data, volume=52):
    return Instrument(name, normalize(data, 110), 0, 0, volume, 12, 0, 128)


def kick():
    out, phase = [], 0.0
    for i in range(int(0.30 * DRUM_RATE)):
        t = i / DRUM_RATE
        phase += TAU * (44 + 90 * math.exp(-t * 32)) / DRUM_RATE
        out.append(math.exp(-t * 8.5) * math.sin(phase) + (0.25 * math.exp(-t * 200)))
    return out


def snare():
    noise, out = Noise(7), []
    for i in range(int(0.22 * DRUM_RATE)):
        t = i / DRUM_RATE
        tone = math.sin(TAU * 185 * t) * math.exp(-t * 30)
        out.append(0.75 * noise.next() * math.exp(-t * 17) + 0.55 * tone)
    return out


def hat(length=0.05, decay=70):
    noise, out, previous = Noise(99), [], 0.0
    for i in range(int(length * DRUM_RATE)):
        value = noise.next()
        out.append((value - previous) * math.exp(-i / DRUM_RATE * decay))
        previous = value
    return out


def tom():
    out, phase = [], 0.0
    for i in range(int(0.34 * DRUM_RATE)):
        t = i / DRUM_RATE
        phase += TAU * (92 + 80 * math.exp(-t * 14)) / DRUM_RATE
        out.append(math.exp(-t * 7.5) * math.sin(phase))
    return out


def crash():
    noise, out, previous = Noise(4242), [], 0.0
    for i in range(int(0.9 * DRUM_RATE)):
        t = i / DRUM_RATE
        value = noise.next()
        out.append((0.6 * value + 0.4 * (value - previous)) * math.exp(-t * 4.0))
        previous = value
    return out


def instrument_bank():
    bell_env = [(0, 64), (3, 50), (14, 32), (40, 16), (100, 6), (180, 0)]
    pluck_env = [(0, 64), (3, 42), (12, 24), (36, 9), (80, 0)]
    return {
        'bell': melodic('bell', [(1, 1.0), (2, 0.45), (3, 0.25), (4.0, 0.12), (6, 0.08)], bell_env, volume=50),
        'strings': melodic('strings', saw_harmonics(12, 0.8), [(0, 0), (10, 36), (22, 54), (40, 50)], 3, 900,
                           volume=34, vibrato=(0, 20, 3, 10)),
        'bass': melodic('bass', [(1, 1.0), (2, 0.55), (3, 0.3), (4, 0.18), (5, 0.1)],
                        [(0, 64), (6, 48), (20, 40)], 2, 3200, volume=50),
        'pluck': melodic('pluck', [(1, 1.0), (3, 0.11), (5, 0.04), (2, 0.2)], pluck_env, volume=44),
        'flute': melodic('flute', [(1, 1.0), (2, 0.12), (3, 0.05)], [(0, 0), (6, 44), (14, 52)], 2, 1400,
                         volume=46, vibrato=(0, 24, 5, 12)),
        'brass': melodic('brass', saw_harmonics(10, 1.2), [(0, 12), (5, 58), (12, 46)], 2, 1700, volume=40,
                         vibrato=(0, 30, 3, 9)),
        'choir': melodic('choir', [(1, 1.0), (2, 0.3), (3, 0.42), (5, 0.12), (4, 0.08)],
                         [(0, 0), (18, 40), (32, 50)], 2, 700, volume=36, vibrato=(0, 40, 4, 8)),
        'organ': melodic('organ', [(1, 1.0), (2, 0.6), (4, 0.35), (8, 0.15)], [(0, 46), (4, 44)], 1, 1200,
                         volume=34),
        'lead': melodic('lead', square_harmonics(11), [(0, 54), (6, 46)], 1, 2200, volume=34,
                        vibrato=(0, 36, 3, 14)),
        'kick': drum('kick', kick(), 58),
        'snare': drum('snare', snare(), 46),
        'hat': drum('hat', hat(), 26),
        'open_hat': drum('open_hat', hat(0.32, 9), 24),
        'tom': drum('tom', tom(), 50),
        'crash': drum('crash', crash(), 32),
    }

# ----------------------------------------------------------------------------------------------- music theory

PITCH = {'C': 0, 'D': 2, 'E': 4, 'F': 5, 'G': 7, 'A': 9, 'B': 11}
QUALITY = {'': (0, 4, 7), 'm': (0, 3, 7), '7': (0, 4, 7, 10), 'm7': (0, 3, 7, 10), 'maj7': (0, 4, 7, 11),
           'dim': (0, 3, 6), 'dim7': (0, 3, 6, 9), 'aug': (0, 4, 8), 'sus4': (0, 5, 7), 'sus2': (0, 2, 7),
           '5': (0, 7)}


def parse_chord(symbol):
    root = PITCH[symbol[0]]
    rest = symbol[1:]
    if rest[:1] == '#':
        root += 1
        rest = rest[1:]
    elif rest[:1] == 'b':
        root -= 1
        rest = rest[1:]
    return root % 12, QUALITY[rest]


def midi(name):
    """'C4' -> semitone number (C0 = 0)."""
    return note_value(name) - 1


def voice(chord, previous, voices, low=midi('A3'), high=midi('E5')):
    """Chord tones for `voices` parts, close to the previous voicing (simple voice leading)."""
    root, intervals = chord
    pitches = [n for n in range(low, high + 1) if (n - root) % 12 in intervals]
    best, best_cost = None, None
    for start in range(len(pitches) - voices + 1):
        candidate = pitches[start:start + voices]
        cost = sum(abs(a - b) for a, b in zip(candidate, previous)) if previous else abs(candidate[0] - 57)
        if best is None or cost < best_cost:
            best, best_cost = candidate, cost
    return best

# ----------------------------------------------------------------------------------------------- song builder


class Section:
    def __init__(self, song, bars):
        self.song = song
        self.bars = bars
        self.rows = bars * song.rows_per_bar
        self.pattern = Pattern(self.rows, song.channels)

    def note(self, row, channel, pitch, instrument, volume=None):
        if 0 <= row < self.rows:
            self.pattern.set(row, channel, pitch + 1, self.song.instrument(instrument), volume)

    def off(self, row, channel):
        if 0 <= row < self.rows and not self.pattern.cells[row][channel].note:
            self.pattern.set(row, channel, NOTE_OFF)

    def melody(self, channel, spec, instrument, volume=None, transpose=0, release=True, start=0):
        """Tokens: 'D5:4' note for 4 rows, 'r:4' rest, '-:2' tie. Bars separated by '|' (ignored)."""
        row = start
        for token in spec.replace('|', ' ').split():
            name, length = token.split(':')
            length = int(length)
            if name == 'r':
                self.off(row, channel)
            elif name != '-':
                self.note(row, channel, midi(name) + transpose, instrument, volume)
                if release and name != '-':
                    pass
            row += length
        if release:
            self.off(row, channel)
        assert row <= self.rows, (spec, row, self.rows)

    def chords(self, channels, progression, instrument, volume=None, low=midi('A3'), high=midi('E5'),
               restrike=False):
        """progression: one entry per bar; 'Gm|A' splits a bar in halves."""
        previous = None
        row = 0
        for entry in progression:
            parts = entry.split('|')
            length = self.song.rows_per_bar // len(parts)
            for symbol in parts:
                if symbol != '-':
                    voicing = voice(parse_chord(symbol), previous, len(channels), low, high)
                    for channel, pitch in zip(channels, voicing):
                        self.note(row, channel, pitch, instrument, volume)
                        if restrike:
                            self.note(row + length // 2, channel, pitch, instrument, volume)
                    previous = voicing
                row += length
        for channel in channels:
            self.off(self.rows - 1, channel)

    def bass(self, channel, progression, rhythm, instrument='bass', volume=None, octave=2):
        """rhythm: one character per row of a bar: R root, O octave, 5 fifth, 3 third, 7 seventh, - off, . hold."""
        row = 0
        for entry in progression:
            parts = entry.split('|')
            length = self.song.rows_per_bar // len(parts)
            for symbol in parts:
                root, intervals = parse_chord(symbol)
                base = octave * 12 + root
                for offset in range(length):
                    character = rhythm[(row + offset) % len(rhythm)]
                    pitch = {'R': base, 'O': base + 12, '5': base + 7, '3': base + intervals[1],
                             '7': base + (intervals[3] if len(intervals) > 3 else 10)}.get(character)
                    if pitch is not None:
                        self.note(row + offset, channel, pitch, instrument, volume)
                    elif character == '-':
                        self.off(row + offset, channel)
                row += length

    def arpeggio(self, channel, progression, instrument, step=2, octave=5, shape=(0, 1, 2, 1), volume=None,
                 spread=(0, 0, 0, 12)):
        row = 0
        for entry in progression:
            parts = entry.split('|')
            length = self.song.rows_per_bar // len(parts)
            for symbol in parts:
                root, intervals = parse_chord(symbol)
                tones = [octave * 12 + root + i for i in intervals[:3]] + [octave * 12 + root + 12]
                for k, offset in enumerate(range(0, length, step)):
                    index = shape[k % len(shape)]
                    self.note(row + offset, channel, tones[index % len(tones)] + spread[index % len(spread)] * 0,
                              instrument, volume)
                row += length

    def drums(self, kick_channel, hat_channel, kick_line, hat_line='', bars=None):
        """kick_line: k kick, s snare, t tom, c crash, x kick+crash; hat_line: h closed, o open. One char per row."""
        mapping = {'k': ('kick', midi('C4')), 's': ('snare', midi('C4')), 't': ('tom', midi('C4')),
                   'T': ('tom', midi('A3')), 'c': ('crash', midi('C4'))}
        for row in range(self.rows if bars is None else bars * self.song.rows_per_bar):
            character = kick_line[row % len(kick_line)] if kick_line else '.'
            if character in mapping:
                name, pitch = mapping[character]
                self.note(row, kick_channel, pitch, name)
            if hat_line:
                character = hat_line[row % len(hat_line)]
                if character == 'h':
                    self.note(row, hat_channel, midi('C4'), 'hat')
                elif character == 'o':
                    self.note(row, hat_channel, midi('C4'), 'open_hat')
                elif character == 'c':
                    self.note(row, hat_channel, midi('C4'), 'crash')


class Song:
    def __init__(self, name, bpm, rows_per_bar=16, channels=8, speed=6):
        self.module = Module(name, channels, speed, bpm)
        self.channels = channels
        self.rows_per_bar = rows_per_bar
        self.bank = instrument_bank()
        self.numbers = {}
        self.sections = {}

    def instrument(self, key):
        if key not in self.numbers:
            self.numbers[key] = self.module.add_instrument(self.bank[key])
        return self.numbers[key]

    def section(self, name, bars):
        section = Section(self, bars)
        self.sections[name] = section
        return section

    def arrange(self, order, restart=None):
        indices = {}
        for name in order:
            if name not in indices:
                indices[name] = self.module.add_pattern(self.sections[name].pattern)
        self.module.order = [indices[name] for name in order]
        self.module.restart = order.index(restart) if restart else 0
        return self.module

# ----------------------------------------------------------------------------------------------- compositions
# Channel plan: 0 melody, 1 second voice, 2-4 harmony, 5 bass, 6 drums, 7 hats / percussion.

CROWN_A = ('D5:4 A4:2 D5:2 F5:4 E5:4 | D5:2 C5:2 A4:4 G4:4 A4:4 | Bb4:4 C5:2 D5:2 F5:4 G5:4 | A5:6 G5:2 F5:4 E5:4 |'
           ' D5:4 A4:2 D5:2 F5:4 E5:4 | D5:2 C5:2 A4:4 Bb4:4 C5:4 | D5:4 E5:2 F5:2 E5:4 C#5:4 | D5:16')
CROWN_CHORDS = ['Dm', 'Am', 'Bb', 'C', 'Dm', 'F', 'Gm|A', 'Dm']


def title_theme():
    song = Song('Crown of Six', 100)
    intro = song.section('intro', 4)
    intro.chords([2, 3, 4], ['Dm', 'Bb', 'Gm', 'A'], 'strings', 40)
    intro.arpeggio(1, ['Dm', 'Bb', 'Gm', 'A'], 'bell', 2, 5, (0, 1, 2, 3, 2, 1), 34)
    intro.bass(5, ['Dm', 'Bb', 'Gm', 'A'], 'R...............', octave=2)
    intro.drums(6, 7, 'k...............k...............k...............k.......t.t.tTTT')

    a = song.section('a', 8)
    a.melody(0, CROWN_A, 'bell', 50)
    a.chords([2, 3, 4], CROWN_CHORDS, 'strings', 34)
    a.bass(5, CROWN_CHORDS, 'R.....R.5...R...')
    a.drums(6, 7, 'k.......s.......k...k...s.......', '..h...h...h...h.')

    b = song.section('b', 8)
    b.melody(0, 'F5:8 D5:4 Bb4:4 | C5:8 A4:8 | Bb4:4 D5:4 G5:8 | F5:12 A4:4 | Bb4:4 D5:4 F5:4 Bb5:4 |'
                ' A5:8 G5:4 E5:4 | E5:8 C#5:4 E5:4 | A5:12 r:4', 'strings', 52)
    b.melody(1, 'D5:8 Bb4:8 | A4:8 F4:8 | G4:8 Bb4:8 | A4:16 | D5:8 F5:8 | E5:8 C5:8 | C#5:8 A4:8 | E5:16',
             'flute', 30)
    b.chords([2, 3, 4], ['Bb', 'F', 'Gm', 'Dm', 'Bb', 'C', 'A', 'A7'], 'choir', 30)
    b.bass(5, ['Bb', 'F', 'Gm', 'Dm', 'Bb', 'C', 'A', 'A7'], 'R...R...5...R.O.')
    b.drums(6, 7, 'k.......s...k...k...k...s...s.s.', 'h.h.h.h.h.h.h.h.')

    a2 = song.section('a2', 8)
    a2.melody(0, CROWN_A, 'bell', 54)
    a2.melody(1, CROWN_A, 'brass', 30, transpose=-12)
    a2.chords([2, 3, 4], CROWN_CHORDS, 'strings', 36, restrike=True)
    a2.bass(5, CROWN_CHORDS, 'R.R.O.R.5.R.O.R.')
    a2.drums(6, 7, 'x.......s.....k.k...k...s...tTtT'.replace('x', 'k'), 'c.h.h.h.h.h.h.h.' + 'h.h.h.h.h.h.o.h.')
    return song.arrange(['intro', 'a', 'b', 'a2'], 'a')


def map_theme():
    song = Song('The Six Roads', 116)
    chords_a = ['F', 'C', 'Dm', 'Bb', 'F', 'C', 'Bb', 'C']
    a = song.section('a', 8)
    a.melody(0, 'F5:3 G5:1 A5:4 C6:4 A5:4 | G5:6 E5:2 C5:8 | D5:3 E5:1 F5:4 A5:4 F5:4 | D5:8 Bb4:4 C5:4 |'
                ' F5:3 G5:1 A5:4 C6:4 D6:4 | C6:6 A5:2 G5:8 | Bb5:4 A5:4 G5:4 F5:4 | G5:12 r:4', 'pluck', 50)
    a.arpeggio(1, chords_a, 'pluck', 2, 4, (0, 1, 2, 1), 22)
    a.chords([2, 3], chords_a, 'strings', 28)
    a.bass(5, chords_a, 'R...5...R...3...')
    a.drums(6, 7, 'k.......s.......', 'h...h...h...h...')
    chords_b = ['Dm', 'Am', 'Bb', 'F', 'Gm', 'Dm', 'Bb', 'C']
    b = song.section('b', 8)
    b.melody(0, 'A5:8 F5:4 D5:4 | E5:8 C5:8 | D5:4 F5:4 Bb5:8 | A5:12 r:4 | G5:4 Bb5:4 D6:4 C6:4 |'
                ' A5:8 F5:8 | G5:4 F5:4 D5:4 Bb4:4 | C5:12 r:4', 'flute', 48)
    b.chords([2, 3, 4], chords_b, 'strings', 30)
    b.bass(5, chords_b, 'R.....R.5...R...')
    b.drums(6, 7, 'k.....k.s.......', 'h.h.h.h.h.h.h.h.')
    c = song.section('c', 8)
    c.melody(0, 'D5:4 A4:2 D5:2 F5:4 E5:4 | D5:2 C5:2 A4:4 G4:4 A4:4 | Bb4:4 C5:2 D5:2 F5:4 G5:4 | A5:16 |'
                ' F5:3 G5:1 A5:4 C6:4 A5:4 | G5:6 E5:2 C5:8 | Bb5:4 A5:4 G5:4 E5:4 | F5:12 r:4', 'bell', 46)
    c.chords([2, 3, 4], ['Dm', 'Am', 'Bb', 'A', 'F', 'C', 'Gm|C', 'F'], 'strings', 30)
    c.bass(5, ['Dm', 'Am', 'Bb', 'A', 'F', 'C', 'Gm|C', 'F'], 'R...R.5.R...5...')
    c.drums(6, 7, 'k.......s...k...', 'h.h.h.h.h.h.h.o.')
    return song.arrange(['a', 'b', 'c', 'a'], 'a')


def verdant_theme():
    song = Song('Emerald Canopy', 100, rows_per_bar=12)
    chords_a = ['Dm', 'C', 'G', 'Dm', 'Dm', 'Am', 'C', 'Dm']
    a = song.section('a', 8)
    a.melody(0, 'D5:4 E5:2 F5:4 A5:2 | G5:6 E5:6 | D5:4 B4:2 G4:6 | A4:12 | D5:4 E5:2 F5:4 G5:2 |'
                ' A5:4 C6:2 A5:6 | G5:4 F5:2 E5:4 C5:2 | D5:12', 'flute', 48)
    a.arpeggio(1, chords_a, 'pluck', 2, 4, (0, 1, 2, 3, 2, 1), 26)
    a.chords([2, 3], chords_a, 'strings', 24)
    a.bass(5, chords_a, 'R.....5.....')
    a.drums(6, 7, '', '......h.....')
    chords_b = ['F', 'C', 'Dm', 'Am', 'Bb', 'F', 'G', 'A']
    b = song.section('b', 8)
    b.melody(0, 'A5:6 F5:6 | G5:6 E5:6 | F5:4 E5:2 D5:6 | E5:12 | F5:4 G5:2 A5:6 | C6:4 A5:2 F5:6 |'
                ' G5:4 B5:2 D6:6 | C#6:12', 'flute', 50)
    b.melody(1, 'F4:12 | E4:12 | D4:12 | C4:12 | D4:12 | C4:12 | B3:12 | C#4:12', 'pluck', 24)
    b.chords([2, 3, 4], chords_b, 'strings', 26)
    b.bass(5, chords_b, 'R.....R.5...')
    b.drums(6, 7, 'k...........', '......h...h.')
    return song.arrange(['a', 'b', 'a', 'b'], 'a')


def inferno_theme():
    song = Song('Iron and Ember', 150)
    chords_a = ['Em', 'F', 'Em', 'D', 'Em', 'F', 'G', 'F']
    a = song.section('a', 8)
    a.melody(0, 'E5:4 r:2 E5:2 F5:4 G5:4 | F5:4 E5:4 D5:8 | E5:4 r:2 E5:2 B5:4 A5:4 | G5:6 F5:2 E5:8 |'
                ' B5:4 C6:4 B5:4 A5:4 | G5:4 F5:4 E5:8 | G5:4 A5:4 B5:4 D6:4 | C6:8 B5:8', 'lead', 46)
    a.chords([2, 3, 4], chords_a, 'brass', 30, restrike=True)
    a.bass(5, chords_a, 'RRORR5RORRORR5RO')
    a.drums(6, 7, 'k.k.s.k.k.k.s.k.' * 7 + 'k.k.s.k.t.t.TTTT', 'h.h.h.h.h.h.h.h.')
    chords_b = ['C', 'D', 'Em', 'Em', 'C', 'D', 'B', 'B7']
    b = song.section('b', 8)
    b.melody(0, 'G5:8 E5:8 | A5:8 F#5:8 | G5:4 F#5:4 E5:4 D5:4 | E5:16 | C6:8 G5:8 | D6:8 A5:8 |'
                ' B5:4 D#6:4 F#6:4 D#6:4 | B5:16', 'brass', 46)
    b.melody(1, 'E5:8 C5:8 | F#5:8 D5:8 | B4:16 | B4:16 | E5:8 C5:8 | F#5:8 D5:8 | D#5:16 | F#5:16', 'strings', 28)
    b.chords([2, 3], chords_b, 'strings', 28)
    b.bass(5, chords_b, 'R.RORR5RR.RORR5R')
    b.drums(6, 7, 'k.s.k.s.kks.k.s.' * 7 + 'tttTtttTTTTTkkkk', 'o...h.h.o...h.h.')
    return song.arrange(['a', 'b', 'a', 'b'], 'a')


def frozen_theme():
    song = Song('Mirror of Winter', 80, rows_per_bar=12)
    chords_a = ['Bm', 'Em', 'F#7', 'Bm', 'G', 'Em', 'F#', 'F#7']
    a = song.section('a', 8)
    a.melody(0, 'F#5:8 B5:4 | G5:8 E5:4 | A#5:8 C#6:4 | B5:12 | D6:8 B5:4 | G5:4 A5:4 B5:4 | A#5:8 F#5:4 |'
                ' F#5:12', 'bell', 50)
    a.arpeggio(1, chords_a, 'pluck', 2, 6, (0, 1, 2, 3, 2, 1), 20)
    a.chords([2, 3, 4], chords_a, 'strings', 26)
    a.bass(5, chords_a, 'R...5...5...')
    chords_b = ['D', 'A', 'Bm', 'F#m', 'G', 'D', 'Em', 'F#']
    b = song.section('b', 8)
    b.melody(0, 'F#5:8 A5:4 | E5:8 C#5:4 | D5:4 E5:4 F#5:4 | C#5:12 | B4:4 D5:4 G5:4 | F#5:8 A5:4 |'
                ' G5:4 F#5:4 E5:4 | F#5:12', 'strings', 50)
    b.arpeggio(1, chords_b, 'bell', 4, 6, (0, 1, 2), 22)
    b.chords([2, 3, 4], chords_b, 'choir', 26)
    b.bass(5, chords_b, 'R.......5...')
    return song.arrange(['a', 'b', 'a', 'b'], 'a')


def umbral_theme():
    song = Song('Forbidden Pages', 72)
    chords_a = ['Cm', 'Ab', 'Fm', 'G7', 'Cm', 'Db', 'Bdim', 'G']
    a = song.section('a', 8)
    a.melody(0, 'C5:6 Gb5:2 Eb5:8 | Ab4:6 D5:2 C5:8 | F5:4 B4:4 Eb5:4 D5:4 | G4:16 | C5:6 F#5:2 G5:8 |'
                ' Db5:6 Ab5:2 F5:8 | D5:4 Ab4:4 B4:4 F5:4 | G4:16', 'choir', 46)
    a.chords([2, 3, 4], chords_a, 'organ', 26)
    a.bass(5, chords_a, 'R...............', octave=1)
    a.drums(6, 7, 'k.........k.....' * 3 + 'k.........k...t.', '')
    b = song.section('b', 8)
    b.melody(0, 'Eb5:8 D5:8 | C5:8 B4:8 | Ab4:4 C5:4 F5:4 Ab5:4 | G5:16 | Gb5:8 F5:8 | Eb5:8 D5:8 |'
                ' C5:4 Eb5:4 Gb5:4 A5:4 | G5:16', 'organ', 40)
    b.arpeggio(1, ['Ab', 'G', 'Fm', 'Cm', 'Gbdim'.replace('dim', ''), 'Fm', 'Adim', 'G7'], 'pluck', 4, 5,
               (0, 2, 1, 3), 22)
    b.chords([2, 3], ['Ab', 'G', 'Fm', 'Cm', 'Gb', 'Fm', 'Adim', 'G7'], 'choir', 24)
    b.bass(5, ['Ab', 'G', 'Fm', 'Cm', 'Gb', 'Fm', 'Adim', 'G7'], 'R.......R.......', octave=1)
    b.drums(6, 7, 'k...............k.........t.t...', '')
    return song.arrange(['a', 'b', 'a', 'b'], 'a')


def storm_theme():
    song = Song('Thunder Over Stone', 156)
    chords_a = ['Am', 'F', 'G', 'E', 'Am', 'F', 'G', 'E7']
    a = song.section('a', 8)
    a.melody(0, 'A5:4 C6:4 E6:4 D6:4 | C6:8 A5:8 | B5:4 D6:4 G6:4 F6:4 | E6:16 | A5:4 C6:4 E6:4 A6:4 |'
                ' F6:8 E6:4 C6:4 | D6:4 B5:4 G5:4 B5:4 | G#5:16', 'brass', 44)
    a.arpeggio(1, chords_a, 'lead', 1, 5, (0, 1, 2, 3), 24)
    a.chords([2, 3], chords_a, 'strings', 28)
    a.bass(5, chords_a, 'R.R.O.R.R.R.O.R.')
    a.drums(6, 7, 'k.s.k.s.kks.k.s.', 'hhhhhhhhhhhhhhhh')
    chords_b = ['Dm', 'Bb', 'C', 'E', 'Dm', 'Bb', 'C', 'E']
    b = song.section('b', 8)
    b.melody(0, 'F5:8 A5:8 | D6:8 Bb5:8 | C6:4 E6:4 G6:8 | G#6:16 | F6:4 D6:4 A5:4 F5:4 | Bb5:8 D6:8 |'
                ' E6:4 C6:4 G5:4 E5:4 | E6:12 r:4', 'strings', 50)
    b.arpeggio(1, chords_b, 'pluck', 1, 5, (0, 1, 2, 3, 2, 1), 26)
    b.chords([2, 3, 4], chords_b, 'brass', 26, restrike=True)
    b.bass(5, chords_b, 'RRRRORRRRRRRO5RR')
    b.drums(6, 7, 'k.s.k.s.kks.ks.s' * 7 + 'tTtTtTtTkkkkcccc', 'h.hhh.hhh.hhh.hh')
    return song.arrange(['a', 'b', 'a', 'b'], 'a')


def nexus_theme():
    song = Song('The Floating Crown', 88)
    chords_a = ['Dm', 'Bb', 'Gm', 'A', 'Dm', 'Bb', 'C', 'A']
    a = song.section('a', 8)
    a.melody(0, 'D5:8 A4:4 D5:4 | F5:8 E5:8 | D5:4 C5:4 A4:8 | E5:16 | D5:8 A4:4 D5:4 | F5:8 G5:8 |'
                ' A5:4 G5:4 E5:8 | A4:16', 'brass', 48)
    a.chords([2, 3, 4], chords_a, 'choir', 32)
    a.bass(5, chords_a, 'R...............', octave=1)
    a.drums(6, 7, 't.......t.......', 'c' + '.' * 127)
    chords_b = ['F', 'C', 'Bb', 'Dm', 'Gm', 'Eb', 'A', 'A7']
    b = song.section('b', 8)
    b.melody(0, 'A5:8 F5:8 | G5:8 E5:8 | F5:8 D5:8 | A5:16 | Bb5:8 G5:8 | Bb5:4 A5:4 G5:8 | A5:4 G5:4 C#5:8 |'
                ' E5:16', 'choir', 50)
    b.melody(1, CROWN_A, 'bell', 26, transpose=-12)
    b.chords([2, 3, 4], chords_b, 'strings', 30)
    b.bass(5, chords_b, 'R.......R.......', octave=1)
    b.drums(6, 7, 't...t...t...t.t.', '')
    return song.arrange(['a', 'b', 'a', 'b'], 'a')


def battle_theme():
    song = Song('Arcane Gambit', 132)
    chords_a = ['Cm', 'Ab', 'Bb', 'G', 'Cm', 'Ab', 'Fm', 'G']
    a = song.section('a', 8)
    a.melody(0, 'C5:2 Eb5:2 G5:4 F5:2 Eb5:2 D5:4 | Eb5:4 C5:4 Ab4:8 | Bb4:2 D5:2 F5:4 Eb5:2 D5:2 C5:4 |'
                ' D5:8 B4:8 | C5:2 Eb5:2 G5:4 Ab5:2 G5:2 F5:4 | Eb5:4 C5:4 Ab5:8 | F5:4 G5:4 Ab5:4 F5:4 | G5:16',
             'lead', 44)
    a.chords([2, 3, 4], chords_a, 'strings', 28)
    a.bass(5, chords_a, 'R.O.R.O.R.O.5.O.')
    a.drums(6, 7, 'k...s..kk.k.s...', 'h.h.h.h.h.h.h.h.')
    chords_b = ['Ab', 'Eb', 'Fm', 'Cm', 'Ab', 'Bb', 'G', 'G7']
    b = song.section('b', 8)
    b.melody(0, 'Ab5:8 C6:8 | Bb5:8 G5:8 | Ab5:4 G5:4 F5:8 | Eb5:16 | C6:8 Ab5:8 | D6:8 Bb5:8 |'
                ' B5:4 D6:4 F6:4 D6:4 | G6:16', 'strings', 48)
    b.arpeggio(1, chords_b, 'pluck', 2, 5, (0, 1, 2, 1), 24)
    b.chords([2, 3], chords_b, 'brass', 26)
    b.bass(5, chords_b, 'R...R.O.R...5.O.')
    b.drums(6, 7, 'k...s...k.k.s...' * 7 + 'k.s.k.s.ttTTkkss', 'h.h.h.h.h.h.o.h.')
    return song.arrange(['a', 'b', 'a', 'b'], 'a')


def boss_theme():
    song = Song("The Ruler's Gambit", 146)
    chords_a = ['Gm', 'D7', 'Eb', 'D', 'Gm', 'Cm', 'D', 'D7']
    motif = ('G5:2 F#5:2 G5:4 D5:4 Eb5:4 | D5:2 C5:2 Bb4:4 A4:8 | G5:2 F#5:2 G5:4 Bb5:4 A5:4 | G5:4 F#5:4 D5:8 |'
             ' G5:2 A5:2 Bb5:4 C6:4 D6:4 | Eb6:4 D6:4 C6:4 Bb5:4 | A5:4 Bb5:4 C6:4 A5:4 | F#5:16')
    a = song.section('a', 8)
    a.melody(0, motif, 'brass', 48)
    a.melody(1, motif, 'lead', 22, transpose=-12)
    a.chords([2, 3, 4], chords_a, 'strings', 30, restrike=True)
    a.bass(5, chords_a, 'RRR5RRORRR5RORR5')
    a.drums(6, 7, 'kk.ks.kkk.kks.k.' * 3 + 'kk.ks.k.t.t.TTTT', 'c.h.h.h.h.h.h.h.' + 'h.h.h.h.h.h.h.h.' * 3)
    chords_b = ['Eb', 'Cm', 'Ab', 'Gm', 'Cm', 'Gm', 'D', 'D7']
    b = song.section('b', 8)
    b.melody(0, 'Bb5:8 A5:8 | G5:8 F#5:8 | Eb5:4 F5:4 G5:4 Ab5:4 | G5:16 | C6:8 Bb5:8 | A5:8 G5:8 |'
                ' F#5:4 A5:4 C6:4 Eb6:4 | D6:16', 'strings', 52)
    b.arpeggio(1, chords_b, 'lead', 1, 5, (0, 1, 2, 3), 20)
    b.chords([2, 3, 4], chords_b, 'choir', 30)
    b.bass(5, chords_b, 'R.R.R5R.R.R.O.RR')
    b.drums(6, 7, 'k.s.kks.k.s.kks.' * 7 + 'tttTTTtttTTTcccc', 'h.hhh.hhh.hhh.hh')
    return song.arrange(['a', 'b', 'a', 'b'], 'a')


def victory_jingle():
    song = Song('Checkmate Fanfare', 120)
    j = song.section('j', 4)
    j.melody(0, 'D5:2 D5:2 D5:2 F#5:4 A5:6 | B5:4 A5:4 F#5:4 A5:4 | D6:16 | r:16', 'brass', 54)
    j.arpeggio(1, ['D', 'G', 'D'], 'bell', 2, 5, (0, 1, 2, 3), 30)
    j.chords([2, 3, 4], ['D', 'G', 'D', '-'], 'strings', 36)
    j.bass(5, ['D', 'G', 'D', 'D'], 'R.......R.......' * 3 + '-' + '.' * 15)
    j.drums(6, 7, 'k.......k.......' + 'k...k...k...tttt' + 'c' + '.' * 31, '')
    return song.arrange(['j'])


def defeat_jingle():
    song = Song('The Fallen King', 66)
    j = song.section('j', 4)
    j.melody(0, 'A4:8 F4:8 | E4:8 C#4:8 | D4:16 | r:16', 'strings', 50)
    j.chords([2, 3, 4], ['Dm', 'A7', 'Dm', '-'], 'choir', 30)
    j.melody(1, 'D3:16 | A2:16 | D3:16 | r:16', 'bell', 40)
    j.bass(5, ['Dm', 'A', 'Dm', 'Dm'], 'R...............' * 3 + '-' + '.' * 15, octave=1)
    return song.arrange(['j'])


def ending_theme():
    song = Song('Six Hands One Crown', 84)
    chords_a = ['D', 'A', 'Bm', 'G', 'D', 'A', 'G|A', 'D']
    major_crown = ('D5:4 A4:2 D5:2 F#5:4 E5:4 | D5:2 C#5:2 A4:4 B4:4 C#5:4 | D5:4 F#5:2 A5:2 B5:4 A5:4 |'
                   ' G5:8 E5:8 | D5:4 A4:2 D5:2 F#5:4 G5:4 | A5:4 B5:2 A5:2 E5:8 | G5:4 F#5:4 E5:4 C#5:4 | D5:16')
    a = song.section('a', 8)
    a.melody(0, major_crown, 'flute', 48)
    a.arpeggio(1, chords_a, 'pluck', 2, 4, (0, 1, 2, 3, 2, 1, 2, 1), 22)
    a.chords([2, 3, 4], chords_a, 'strings', 30)
    a.bass(5, chords_a, 'R.......5.......')
    chords_b = ['G', 'D', 'Em', 'Bm', 'G', 'D', 'E', 'A']
    b = song.section('b', 8)
    b.melody(0, 'B5:8 G5:4 D5:4 | A5:8 F#5:8 | G5:4 B5:4 E6:8 | D6:12 r:4 | B5:4 D6:4 G6:4 F#6:4 | E6:8 D6:8 |'
                ' E6:4 F#6:4 G#6:4 E6:4 | A6:12 r:4', 'strings', 50)
    b.melody(1, 'G4:16 | F#4:16 | E4:16 | F#4:16 | D4:16 | F#4:16 | G#4:16 | A4:16', 'choir', 22)
    b.chords([2, 3, 4], chords_b, 'strings', 28)
    b.bass(5, chords_b, 'R.......R...5...')
    b.drums(6, 7, '', 'c' + '.' * 127)
    c = song.section('c', 8)
    c.melody(0, major_crown, 'bell', 50, transpose=12)
    c.melody(1, major_crown, 'strings', 30)
    c.chords([2, 3, 4], chords_a, 'choir', 30)
    c.bass(5, chords_a, 'R...5...R...5...')
    c.drums(6, 7, 't...............t.......t.......', '')
    return song.arrange(['a', 'b', 'c', 'b'], 'a')


SONGS = [('title', title_theme), ('world_map', map_theme), ('verdant', verdant_theme), ('inferno', inferno_theme),
         ('frozen', frozen_theme), ('umbral', umbral_theme), ('storm', storm_theme), ('nexus', nexus_theme),
         ('battle', battle_theme), ('boss', boss_theme), ('victory', victory_jingle), ('defeat', defeat_jingle),
         ('ending', ending_theme)]

# ----------------------------------------------------------------------------------------------- sound effects


def env(t, attack, decay):
    return min(1.0, t / attack) * math.exp(-t * decay) if attack > 0 else math.exp(-t * decay)


def tone_sweep(duration, f0, f1, wave='sine', decay=6.0, attack=0.002, noise_mix=0.0, seed=1):
    out, phase, noise = [], 0.0, Noise(seed)
    n = int(duration * SFX_RATE)
    for i in range(n):
        t = i / SFX_RATE
        f = f0 + (f1 - f0) * (i / max(1, n - 1))
        phase += TAU * f / SFX_RATE
        if wave == 'square':
            value = 1.0 if math.sin(phase) >= 0 else -1.0
            value *= 0.6
        elif wave == 'saw':
            value = ((phase / TAU) % 1.0) * 2 - 1
        elif wave == 'triangle':
            value = 2 * abs(2 * ((phase / TAU) % 1.0) - 1) - 1
        else:
            value = math.sin(phase)
        value = value * (1 - noise_mix) + noise.next() * noise_mix
        out.append(value * env(t, attack, decay))
    return out


def mix(*tracks, offsets=None):
    offsets = offsets or [0] * len(tracks)
    length = max(len(t) + int(o * SFX_RATE) for t, o in zip(tracks, offsets))
    out = [0.0] * length
    for track, offset in zip(tracks, offsets):
        start = int(offset * SFX_RATE)
        for i, value in enumerate(track):
            out[start + i] += value
    return out


def bell_note(freq, duration=0.5, decay=6.0):
    out = []
    for i in range(int(duration * SFX_RATE)):
        t = i / SFX_RATE
        value = (math.sin(TAU * freq * t) + 0.45 * math.sin(TAU * freq * 2 * t) +
                 0.2 * math.sin(TAU * freq * 3 * t))
        out.append(value * env(t, 0.002, decay))
    return out


def noise_burst(duration, decay, seed=3, lowpass=0.0):
    noise, out, value = Noise(seed), [], 0.0
    for i in range(int(duration * SFX_RATE)):
        sample = noise.next()
        value = value * lowpass + sample * (1 - lowpass)
        out.append(value * math.exp(-i / SFX_RATE * decay))
    return out


def sound_effects():
    note = lambda name: 440.0 * 2 ** ((midi(name) - midi('A4')) / 12)  # noqa: E731
    sparkle = mix(*[bell_note(note(n), 0.25, 14) for n in ('E7', 'B6', 'G#7')], offsets=[0, 0.05, 0.1])
    grind = []
    noise = Noise(77)
    value = 0.0
    for i in range(int(0.36 * SFX_RATE)):
        t = i / SFX_RATE
        value = value * 0.92 + noise.next() * 0.08
        grind.append(value * 6 * (0.6 + 0.4 * math.sin(TAU * 28 * t)) * min(1, t * 20) * math.exp(-t * 3))
    phase_warp = []
    phase = 0.0
    for i in range(int(0.34 * SFX_RATE)):
        t = i / SFX_RATE
        f = 300 + 900 * math.sin(math.pi * t / 0.34)
        phase += TAU * f / SFX_RATE
        phase_warp.append(math.sin(phase) * (0.6 + 0.4 * math.sin(TAU * 32 * t)) * math.sin(math.pi * t / 0.34))
    breath = []
    noise = Noise(55)
    value = 0.0
    for i in range(int(0.42 * SFX_RATE)):
        t = i / SFX_RATE
        sample = noise.next()
        value = value * 0.55 + sample * 0.45
        breath.append((value * 0.7 + 0.3 * math.sin(TAU * 1250 * t)) * math.sin(math.pi * t / 0.42))
    return {
        'cursor': tone_sweep(0.022, 1300, 1300, 'square', 40),
        'menu_move': tone_sweep(0.04, 880, 880, 'triangle', 30),
        'menu_select': mix(tone_sweep(0.06, 660, 660, 'square', 18), tone_sweep(0.09, 990, 990, 'square', 14),
                           offsets=[0, 0.055]),
        'menu_cancel': mix(tone_sweep(0.06, 990, 990, 'square', 18), tone_sweep(0.09, 620, 620, 'square', 14),
                           offsets=[0, 0.055]),
        'piece_select': mix(noise_burst(0.02, 120, 9), tone_sweep(0.06, 520, 480, 'sine', 40)),
        'piece_move': mix(tone_sweep(0.12, 190, 110, 'sine', 22), noise_burst(0.03, 90, 11, 0.6)),
        'capture': mix(noise_burst(0.18, 22, 13, 0.3), tone_sweep(0.2, 150, 90, 'sine', 14),
                       tone_sweep(0.22, 1800, 1700, 'triangle', 18)),
        'invalid': tone_sweep(0.16, 115, 105, 'square', 8),
        'check': mix(tone_sweep(0.11, 880, 880, 'square', 10), tone_sweep(0.14, 880, 880, 'square', 9),
                     offsets=[0, 0.15]),
        'checkmate': mix(*[tone_sweep(0.75, note(n), note(n), 'saw', 4.0) for n in ('D4', 'F4', 'A4', 'D5')],
                         tone_sweep(0.6, 70, 40, 'sine', 5)),
        'stalemate': tone_sweep(0.5, 660, 440, 'sine', 4),
        'promotion': mix(*[bell_note(note(n), 0.4, 8) for n in ('C6', 'E6', 'G6', 'C7')],
                         offsets=[0, 0.07, 0.14, 0.21]),
        'shadow_step': mix(noise_burst(0.22, 12, 21, 0.85), tone_sweep(0.15, 240, 90, 'sine', 18)),
        'phase_shift': phase_warp,
        'ghost_walk': breath,
        'castle_swap': mix(grind, bell_note(note('G6'), 0.35, 9), offsets=[0, 0.3]),
        'royal_command': mix(tone_sweep(0.18, note('G4'), note('G4'), 'saw', 5, 0.02),
                             tone_sweep(0.3, note('D5'), note('D5'), 'saw', 4, 0.02), offsets=[0, 0.17]),
        'royal_escape': mix(tone_sweep(0.32, 200, 1600, 'triangle', 3, 0.01), sparkle, offsets=[0, 0.25]),
        'achievement': mix(*[bell_note(note(n), 0.5, 6) for n in ('E6', 'G#6', 'B6', 'E7')],
                           offsets=[0, 0.08, 0.16, 0.24]),
        'unlock': mix(*[tone_sweep(0.12, note(n), note(n), 'square', 6) for n in ('C5', 'E5', 'G5')],
                      tone_sweep(0.45, note('C6'), note('C6'), 'square', 3.5), offsets=[0, 0.1, 0.2, 0.3]),
        'boss_appear': mix(tone_sweep(0.95, 55, 52, 'saw', 2.5, 0.05), noise_burst(0.5, 4, 31, 0.9),
                           tone_sweep(0.5, 80, 35, 'sine', 5), offsets=[0, 0, 0.45]),
        'victory': mix(*[bell_note(note(n), 0.6, 4) for n in ('D5', 'F#5', 'A5', 'D6')],
                       tone_sweep(0.6, note('D4'), note('D4'), 'saw', 4, 0.02)),
        'defeat': mix(tone_sweep(0.3, note('D4'), note('D4'), 'saw', 4, 0.02),
                      tone_sweep(0.3, note('A3'), note('A3'), 'saw', 4, 0.02),
                      tone_sweep(0.5, note('F3'), note('F3'), 'saw', 3, 0.02), offsets=[0, 0.28, 0.56]),
    }


def write_wav(path, samples):
    peak = max(1e-9, max(abs(v) for v in samples))
    data = bytes(int(max(0, min(255, round(128 + v / peak * 100)))) for v in samples)
    header = b'RIFF' + struct.pack('<I', 36 + len(data)) + b'WAVE'
    header += b'fmt ' + struct.pack('<IHHIIHH', 16, 1, 1, SFX_RATE, SFX_RATE, 1, 8)
    header += b'data' + struct.pack('<I', len(data))
    with open(path, 'wb') as f:
        f.write(header + data)


def generate(root):
    audio = os.path.join(root, 'audio')
    os.makedirs(audio, exist_ok=True)
    for name, builder in SONGS:
        module = builder()
        module.save(os.path.join(audio, name + '.xm'))
    for name, samples in sound_effects().items():
        write_wav(os.path.join(audio, 'sfx_' + name + '.wav'), samples)


if __name__ == '__main__':
    generate(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    print('audio generated')
