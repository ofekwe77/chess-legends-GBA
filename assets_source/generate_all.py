"""Regenerates every asset of CHESS LEGENDS: ARCANE WARS from source (deterministic, standard library only).

Graphics go to graphics/ (Butano BMP + JSON), generated headers to include/ and core/, audio to audio/ (XM modules
and WAV effects), previews to assets_source/previews/. Usage: py -3.12 assets_source/generate_all.py
"""
import importlib
import os
import sys
import time

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
sys.path.insert(0, HERE)
sys.path.insert(0, os.path.join(HERE, 'gfx'))

GENERATORS = ['gen_fonts', 'gen_font_big', 'gen_pieces', 'gen_board', 'gen_ui', 'gen_icons', 'gen_portraits',
              'gen_backdrops', 'gen_effects', 'gen_audio']


def main():
    for name in GENERATORS:
        start = time.time()
        module = importlib.import_module(name)
        module.generate(ROOT)
        print('%-14s %5.1f s' % (name, time.time() - start))
    print('all assets generated')


if __name__ == '__main__':
    main()
