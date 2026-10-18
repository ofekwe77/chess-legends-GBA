"""Original pixel fonts for CHESS LEGENDS: ARCANE WARS.

small: 8x8 cells, variable width, 6-pixel capitals, 2-pixel descenders, automatic drop shadow.
Glyph images feed Butano sprite fonts; widths and the UTF-8 extra characters are emitted as a C++ header.
"""

import os
import sys

sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), 'gfx'))

from canvas import Canvas  # noqa: E402
from imageio import write_bmp, write_json, write_png  # noqa: E402
import palettes as P  # noqa: E402

GLYPH = 1
SHADOW = 2
HIGHLIGHT = 3

# Each glyph: list of rows ('#' = ink), top row = cell row 0. Capitals occupy rows 0-5, descenders rows 6-7.
SMALL = {
    'A': ['.##.', '#..#', '#..#', '####', '#..#', '#..#'],
    'B': ['###.', '#..#', '###.', '#..#', '#..#', '###.'],
    'C': ['.##.', '#..#', '#...', '#...', '#..#', '.##.'],
    'D': ['###.', '#..#', '#..#', '#..#', '#..#', '###.'],
    'E': ['####', '#...', '###.', '#...', '#...', '####'],
    'F': ['####', '#...', '###.', '#...', '#...', '#...'],
    'G': ['.##.', '#..#', '#...', '#.##', '#..#', '.###'],
    'H': ['#..#', '#..#', '####', '#..#', '#..#', '#..#'],
    'I': ['###', '.#.', '.#.', '.#.', '.#.', '###'],
    'J': ['..##', '...#', '...#', '...#', '#..#', '.##.'],
    'K': ['#..#', '#.#.', '##..', '#.#.', '#..#', '#..#'],
    'L': ['#...', '#...', '#...', '#...', '#...', '####'],
    'M': ['#...#', '##.##', '#.#.#', '#...#', '#...#', '#...#'],
    'N': ['#..#', '##.#', '#.##', '#..#', '#..#', '#..#'],
    'O': ['.##.', '#..#', '#..#', '#..#', '#..#', '.##.'],
    'P': ['###.', '#..#', '#..#', '###.', '#...', '#...'],
    'Q': ['.##.', '#..#', '#..#', '#..#', '#.#.', '.#.#'],
    'R': ['###.', '#..#', '#..#', '###.', '#.#.', '#..#'],
    'S': ['.###', '#...', '.##.', '...#', '...#', '###.'],
    'T': ['#####', '..#..', '..#..', '..#..', '..#..', '..#..'],
    'U': ['#..#', '#..#', '#..#', '#..#', '#..#', '.##.'],
    'V': ['#...#', '#...#', '#...#', '.#.#.', '.#.#.', '..#..'],
    'W': ['#...#', '#...#', '#...#', '#.#.#', '##.##', '#...#'],
    'X': ['#...#', '.#.#.', '..#..', '..#..', '.#.#.', '#...#'],
    'Y': ['#...#', '#...#', '.#.#.', '..#..', '..#..', '..#..'],
    'Z': ['####', '...#', '..#.', '.#..', '#...', '####'],
    'a': ['....', '....', '.###', '#..#', '#..#', '.###'],
    'b': ['#...', '#...', '###.', '#..#', '#..#', '###.'],
    'c': ['...', '...', '.##', '#..', '#..', '.##'],
    'd': ['...#', '...#', '.###', '#..#', '#..#', '.###'],
    'e': ['....', '....', '.##.', '####', '#...', '.###'],
    'f': ['.##', '#..', '###', '#..', '#..', '#..'],
    'g': ['....', '....', '.###', '#..#', '#..#', '.###', '...#', '.##.'],
    'h': ['#...', '#...', '###.', '#..#', '#..#', '#..#'],
    'i': ['#', '.', '#', '#', '#', '#'],
    'j': ['..#', '...', '..#', '..#', '..#', '..#', '#.#', '.#.'],
    'k': ['#...', '#...', '#..#', '###.', '#.#.', '#..#'],
    'l': ['#.', '#.', '#.', '#.', '#.', '.#'],
    'm': ['.....', '.....', '####.', '#.#.#', '#.#.#', '#.#.#'],
    'n': ['....', '....', '###.', '#..#', '#..#', '#..#'],
    'o': ['....', '....', '.##.', '#..#', '#..#', '.##.'],
    'p': ['....', '....', '###.', '#..#', '#..#', '###.', '#...', '#...'],
    'q': ['....', '....', '.###', '#..#', '#..#', '.###', '...#', '...#'],
    'r': ['...', '...', '#.#', '##.', '#..', '#..'],
    's': ['....', '....', '.###', '##..', '..##', '###.'],
    't': ['.#..', '.#..', '###.', '.#..', '.#..', '..##'],
    'u': ['....', '....', '#..#', '#..#', '#..#', '.###'],
    'v': ['.....', '.....', '#...#', '#...#', '.#.#.', '..#..'],
    'w': ['.....', '.....', '#...#', '#.#.#', '#.#.#', '.#.#.'],
    'x': ['....', '....', '#..#', '.##.', '.##.', '#..#'],
    'y': ['....', '....', '#..#', '#..#', '#..#', '.###', '...#', '.##.'],
    'z': ['....', '....', '####', '..#.', '.#..', '####'],
    '0': ['.##.', '#..#', '#.##', '##.#', '#..#', '.##.'],
    '1': ['.#.', '##.', '.#.', '.#.', '.#.', '###'],
    '2': ['.##.', '#..#', '..#.', '.#..', '#...', '####'],
    '3': ['###.', '...#', '.##.', '...#', '...#', '###.'],
    '4': ['#..#', '#..#', '####', '...#', '...#', '...#'],
    '5': ['####', '#...', '###.', '...#', '...#', '###.'],
    '6': ['.##.', '#...', '###.', '#..#', '#..#', '.##.'],
    '7': ['####', '...#', '..#.', '.#..', '.#..', '.#..'],
    '8': ['.##.', '#..#', '.##.', '#..#', '#..#', '.##.'],
    '9': ['.##.', '#..#', '#..#', '.###', '...#', '.##.'],
    '!': ['#', '#', '#', '#', '.', '#'],
    '"': ['#.#', '#.#'],
    '#': ['.....', '.#.#.', '#####', '.#.#.', '#####', '.#.#.'],
    '$': ['..#..', '.####', '#.#..', '.###.', '..#.#', '####.', '..#..'],
    '%': ['#..#', '...#', '..#.', '.#..', '#...', '#..#'],
    '&': ['.#..', '#.#.', '.#..', '#.#.', '#..#', '.##.'],
    "'": ['#', '#'],
    '(': ['.#', '#.', '#.', '#.', '#.', '.#'],
    ')': ['#.', '.#', '.#', '.#', '.#', '#.'],
    '*': ['.....', '#.#.#', '.###.', '#.#.#'],
    '+': ['...', '...', '.#.', '###', '.#.'],
    ',': ['.', '.', '.', '.', '.', '#', '#'],
    '-': ['...', '...', '...', '###'],
    '.': ['.', '.', '.', '.', '.', '#'],
    '/': ['...#', '..#.', '..#.', '.#..', '.#..', '#...'],
    ':': ['.', '#', '.', '.', '#'],
    ';': ['.', '#', '.', '.', '#', '#'],
    '<': ['...', '..#', '.#.', '#..', '.#.', '..#'],
    '=': ['...', '...', '###', '...', '###'],
    '>': ['...', '#..', '.#.', '..#', '.#.', '#..'],
    '?': ['.##.', '#..#', '..#.', '.#..', '....', '.#..'],
    '@': ['.###.', '#...#', '#.###', '#.#.#', '#..##', '.##..'],
    '[': ['##', '#.', '#.', '#.', '#.', '##'],
    '\\': ['#...', '.#..', '.#..', '..#.', '..#.', '...#'],
    ']': ['##', '.#', '.#', '.#', '.#', '##'],
    '^': ['.#.', '#.#'],
    '_': ['....', '....', '....', '....', '....', '....', '####'],
    '`': ['#.', '.#'],
    '{': ['.##', '.#.', '#..', '.#.', '.#.', '.##'],
    '|': ['#', '#', '#', '#', '#', '#', '#'],
    '}': ['##.', '.#.', '..#', '.#.', '.#.', '##.'],
    '~': ['....', '.#.#', '#.#.'],
}

# Extra glyphs (UTF-8). Value: (rows, palette roles) where 'G' = glyph color, 'S' = dark, '#' = glyph color.
EXTRA_ORDER = ['◆', '◇', '→', '←', '★', '✦', 'Ⓐ', 'Ⓑ', 'Ⓛ', 'Ⓡ', '…', '♦']
EXTRA = {
    '◆': ['..#..', '.###.', '#####', '.###.', '..#..'],
    '◇': ['..#..', '.#.#.', '#...#', '.#.#.', '..#..'],
    '→': ['......', '...#..', '.....#', '######', '.....#', '...#..'],
    '←': ['......', '..#...', '#.....', '######', '#.....', '..#...'],
    '★': ['..#..', '..#..', '#####', '.###.', '.#.#.', '#...#'],
    '✦': ['..#..', '..#..', '##.##', '..#..', '..#..'],
    # Button caps: filled key with the letter cut out (letter drawn in the shadow color).
    'Ⓐ': ['.#####.', '##SSS##', '#S###S#', '#SSSSS#', '#S###S#', '.#####.'],
    'Ⓑ': ['.#####.', '#SSSS##', '#S##S##', '#SSSS##', '#S##SS#', '.#####.'],
    'Ⓛ': ['.#####.', '#S#####', '#S#####', '#S#####', '#SSSS##', '.#####.'],
    'Ⓡ': ['.#####.', '#SSSS##', '#S###S#', '#SSSS##', '#S##S##', '.#####.'],
    '…': ['.....', '.....', '.....', '.....', '.....', '#.#.#'],
    '♦': ['.#.', '###', '.#.'],
}


def glyph_canvas(rows, extra=False):
    cell = Canvas(8, 8)
    for y, row in enumerate(rows):
        for x, ch in enumerate(row):
            if ch == '#':
                cell.set(x, y, GLYPH)
            elif ch == 'S':
                cell.set(x, y, SHADOW)
    width = max(len(r) for r in rows) if rows else 0
    if not extra:
        # Drop shadow one pixel right and down (never covering ink).
        shadow = Canvas(8, 8)
        for y in range(8):
            for x in range(8):
                if cell.get(x, y) == GLYPH and cell.get(x + 1, y + 1) == 0:
                    shadow.set(x + 1, y + 1, SHADOW)
        for y in range(8):
            for x in range(8):
                if shadow.get(x, y) and cell.get(x, y) == 0:
                    cell.set(x, y, SHADOW)
    else:
        # Extra icons get a shadow below only.
        for y in range(7, 0, -1):
            for x in range(8):
                if cell.get(x, y) == 0 and cell.get(x, y - 1) == GLYPH:
                    cell.set(x, y, SHADOW)
    return cell, width


def build_small():
    chars = [chr(c) for c in range(33, 127)]
    count = len(chars) + len(EXTRA_ORDER)
    sheet = Canvas(8, 8 * count)
    widths = [4]   # space (character 32) advance
    for i, ch in enumerate(chars):
        cell, w = glyph_canvas(SMALL[ch])
        sheet.blit(cell, 0, i * 8)
        widths.append(w + 2)
    for j, ch in enumerate(EXTRA_ORDER):
        cell, w = glyph_canvas(EXTRA[ch], extra=True)
        sheet.blit(cell, 0, (len(chars) + j) * 8)
        widths.append(w + 1)
    return sheet, widths


def write_header(path, name, item, widths, extras, height):
    lines = []
    lines.append('// Generated by assets_source/gen_fonts.py - do not edit.')
    guard = 'CL_FONT_%s_H' % name.upper()
    lines.append('#ifndef ' + guard)
    lines.append('#define ' + guard)
    lines.append('')
    lines.append('#include "bn_sprite_font.h"')
    lines.append('#include "bn_utf8_characters_map.h"')
    lines.append('#include "bn_sprite_items_%s.h"' % item)
    lines.append('')
    lines.append('namespace cl::fonts')
    lines.append('{')
    lines.append('')
    lines.append('constexpr bn::utf8_character %s_utf8_characters[] = {' % name)
    lines.append('    ' + ', '.join('"%s"' % e for e in extras))
    lines.append('};')
    lines.append('')
    lines.append('constexpr int8_t %s_character_widths[] = {' % name)
    body = []
    for i, w in enumerate(widths):
        body.append('%d' % w)
    for i in range(0, len(body), 16):
        lines.append('    ' + ', '.join(body[i:i + 16]) + ',')
    lines.append('};')
    lines.append('')
    lines.append('constexpr bn::span<const bn::utf8_character> %s_utf8_characters_span(%s_utf8_characters);' % (name, name))
    lines.append('')
    lines.append('constexpr auto %s_utf8_characters_map = bn::utf8_characters_map<%s_utf8_characters_span>();' % (name, name))
    lines.append('')
    lines.append('constexpr bn::sprite_font %s(' % name)
    lines.append('        bn::sprite_items::%s, %s_utf8_characters_map.reference(), %s_character_widths);' % (item, name, name))
    lines.append('')
    lines.append('}')
    lines.append('')
    lines.append('#endif')
    with open(path, 'w', newline='\n', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')


def write_metrics(path, name, widths, extras):
    """Toolkit-independent advance widths, shared by the ROM and the host content validator (core/)."""
    lines = []
    lines.append('// Generated by assets_source/gen_fonts.py - do not edit.')
    guard = 'CL_METRICS_%s_H' % name.upper()
    lines.append('#ifndef ' + guard)
    lines.append('#define ' + guard)
    lines.append('')
    lines.append('#include <stdint.h>')
    lines.append('')
    lines.append('namespace cl::metrics')
    lines.append('{')
    lines.append('')
    lines.append('// Advance widths in pixels: ASCII 32-126, then the extra UTF-8 glyphs in %s_extras order.' % name)
    lines.append('constexpr int8_t %s_widths[] = {' % name)
    body = ['%d' % w for w in widths]
    for i in range(0, len(body), 16):
        lines.append('    ' + ', '.join(body[i:i + 16]) + ',')
    lines.append('};')
    lines.append('')
    lines.append('constexpr const char* %s_extras[] = {' % name)
    lines.append('    ' + ', '.join('"%s"' % e for e in extras))
    lines.append('};')
    lines.append('')
    lines.append('constexpr int %s_extras_count = %d;' % (name, len(extras)))
    lines.append('')
    lines.append('}')
    lines.append('')
    lines.append('#endif')
    with open(path, 'w', newline='\n', encoding='utf-8') as f:
        f.write('\n'.join(lines) + '\n')


def generate(root):
    graphics = os.path.join(root, 'graphics')
    include = os.path.join(root, 'include')
    os.makedirs(include, exist_ok=True)
    sheet, widths = build_small()
    write_bmp(os.path.join(graphics, 'font_small.bmp'), sheet, P.TEXT_WHITE, bpp=4)
    write_json(os.path.join(graphics, 'font_small.json'), {'type': 'sprite', 'height': 8})
    write_header(os.path.join(include, 'cl_font_small.h'), 'small', 'font_small', widths, EXTRA_ORDER, 8)
    write_metrics(os.path.join(root, 'core', 'cl_metrics_small.h'), 'small', widths, EXTRA_ORDER)

    swatch = Canvas(8, 8)
    for i in range(16):
        swatch.set(i % 8, i // 8, i)
    for name, pal in (('text_white', P.TEXT_WHITE), ('text_gold', P.TEXT_GOLD), ('text_gray', P.TEXT_GRAY),
                      ('text_red', P.TEXT_RED), ('text_cyan', P.TEXT_CYAN), ('text_dark', P.TEXT_DARK)):
        write_bmp(os.path.join(graphics, name + '.bmp'), swatch, pal, bpp=4)
        write_json(os.path.join(graphics, name + '.json'), {'type': 'sprite_palette'})

    # Preview: sample text rendered with the widths.
    sample = ['CHESS LEGENDS: ARCANE WARS', 'The quick brown fox jumps over', 'the lazy dog. 0123456789 !?',
              'Royal Escape costs 5 mana.', 'ÂÂ']
    preview = Canvas(240, 8 * 6, fill=0)
    chars = [chr(c) for c in range(33, 127)]
    y = 0
    for line in sample[:4] + ['◆◆◆◇◇◇ Ⓐ Select Ⓑ Back Ⓛ Magic Ⓡ ★']:
        x = 2
        for ch in line:
            if ch == ' ':
                x += widths[0]
                continue
            if ch in chars:
                index = chars.index(ch)
                w = widths[index + 1]
            elif ch in EXTRA_ORDER:
                index = len(chars) + EXTRA_ORDER.index(ch)
                w = widths[len(chars) + 1 + EXTRA_ORDER.index(ch)]
            else:
                continue
            preview.blit(sheet.crop(0, index * 8, 8, 8), x, y)
            x += w
        y += 9
    pal = list(P.TEXT_WHITE)
    pal[0] = (54, 52, 98)
    write_png(os.path.join(root, 'assets_source', 'previews', 'font_small.png'), preview, pal, scale=3,
              transparent_color=None)


if __name__ == '__main__':
    generate(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
    print('fonts generated')
