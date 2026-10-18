"""Checks that every non-ASCII character in the ROM's string literals exists in the fonts.

Butano stops with an error when asked to draw a glyph its font does not have, so a stray character in a source
literal would crash the game at the moment it is shown. The host validator covers core/ content; this script
covers the literals in src/ (and core/) source files. Exit code 1 lists every offending literal.
"""
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def font_extras(header):
    text = open(os.path.join(ROOT, 'core', header), encoding='utf-8').read()
    block = text[text.index('_extras[] = {'):]
    block = block[:block.index('};')]
    return set(re.findall(r'"([^"]+)"', block))


def main():
    sys.stdout.reconfigure(encoding='utf-8', errors='replace')
    allowed = font_extras('cl_metrics_small.h') | font_extras('cl_metrics_big.h')
    literal = re.compile(r'"((?:[^"\\\n]|\\.)*)"')
    problems = 0

    for folder in ('src', 'core'):
        for name in sorted(os.listdir(os.path.join(ROOT, folder))):
            if not name.endswith(('.cpp', '.h')):
                continue

            path = os.path.join(ROOT, folder, name)

            for number, line in enumerate(open(path, encoding='utf-8'), 1):
                for match in literal.finditer(line):
                    for char in match.group(1):
                        if ord(char) > 126 and char not in allowed:
                            print('%s:%d: character %r (U+%04X) is not in the fonts: "%s"'
                                  % (os.path.relpath(path, ROOT), number, char, ord(char), match.group(1)))
                            problems += 1

    print('glyph check: %d problem(s)' % problems)
    return 1 if problems else 0


if __name__ == '__main__':
    sys.exit(main())
