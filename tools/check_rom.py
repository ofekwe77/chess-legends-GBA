"""Inspects a built ROM as a Game Boy Advance binary instead of trusting its extension (MD.txt 114).

Usage: python3 tools/check_rom.py ROM [--map build/ChessLegends.map] [--build build]
Checks the cartridge header (entry branch, Nintendo logo, fixed byte, complement checksum, printable title and
codes), the size, the save-type marker emulators use to pick the save memory, and that every graphics and audio
asset of the project was converted and linked. Exit status 1 on any failure.
"""
import glob
import os
import re
import sys

LOGO_START = bytes.fromhex('24ffae51699aa2213d84820a84e409ad')    # first 16 bytes of the boot logo


def main():
    args = sys.argv[1:]
    if not args:
        print(__doc__)
        return 2
    rom_path = args[0]
    map_path = args[args.index('--map') + 1] if '--map' in args else 'build/ChessLegends.map'
    build_dir = args[args.index('--build') + 1] if '--build' in args else 'build'
    failures = []

    def check(condition, label, detail=''):
        print('%-5s %s%s' % ('ok' if condition else 'FAIL', label, (': ' + detail) if detail else ''))
        if not condition:
            failures.append(label)

    rom = open(rom_path, 'rb').read()
    check(0x200 <= len(rom) <= 32 * 1024 * 1024, 'size', '%d bytes (%.1f KB)' % (len(rom), len(rom) / 1024))
    check(rom[3] == 0xEA, 'entry point is an ARM branch', rom[0:4].hex())
    check(rom[4:4 + len(LOGO_START)] == LOGO_START, 'boot logo present')
    check(rom[0xB2] == 0x96, 'fixed header byte 0x96')
    complement = (-(sum(rom[0xA0:0xBD]) + 0x19)) & 0xFF
    check(rom[0xBD] == complement, 'header complement checksum', '0x%02X' % rom[0xBD])
    title, code, maker = rom[0xA0:0xAC], rom[0xAC:0xB0], rom[0xB0:0xB2]
    printable = all(32 <= b < 127 or b == 0 for b in title + code + maker)
    check(printable, 'title / game code / maker',
          '%s / %s / %s' % (title.rstrip(b'\0').decode('ascii', 'replace'), code.decode('ascii', 'replace'),
                            maker.decode('ascii', 'replace')))
    save = re.search(rb'(SRAM_V|SRAM_F_V|EEPROM_V|FLASH_V|FLASH512_V|FLASH1M_V)\d{3}', rom)
    check(save is not None and save.group(1) == b'SRAM_V', 'save type marker', save.group(0).decode() if save else '')

    # Assets: every graphics BMP converted and linked, every module / effect converted, the soundbank linked.
    if os.path.isfile(map_path):
        text = open(map_path, encoding='utf-8', errors='replace').read()
        marker = text.find('Linker script and memory map')
        linked = text[marker:] if marker >= 0 else text
        names = sorted(os.path.splitext(os.path.basename(p))[0] for p in glob.glob('graphics/*.bmp'))
        missing = [n for n in names if (n + '_bn_gfx') not in linked]
        check(not missing, 'graphics linked', '%d of %d%s' % (len(names) - len(missing), len(names),
                                                            (' (missing: ' + ', '.join(missing) + ')') if missing else ''))
        check('bn_audio_soundbank_bin' in linked, 'soundbank linked')
    else:
        check(False, 'linker map found', map_path)

    def items(header, kind):
        path = os.path.join(build_dir, header)
        text = open(path, encoding='utf-8').read() if os.path.isfile(path) else ''
        return set(re.findall(r'%s (\w+)\(' % kind, text))

    music = sorted(os.path.splitext(os.path.basename(p))[0] for p in glob.glob('audio/*.xm'))
    sounds = sorted(os.path.splitext(os.path.basename(p))[0] for p in glob.glob('audio/*.wav'))
    music_items = items('bn_music_items.h', 'music_item')
    sound_items = items('bn_sound_items.h', 'sound_item')
    music_missing = [m for m in music if m not in music_items]
    sound_missing = [s for s in sounds if s not in sound_items]
    check(not music_missing, 'music modules converted', '%d of %d' % (len(music) - len(music_missing), len(music)))
    check(not sound_missing, 'sound effects converted', '%d of %d' % (len(sounds) - len(sound_missing), len(sounds)))

    print('%d check(s) failed' % len(failures) if failures else 'ROM checks passed')
    return 1 if failures else 0


if __name__ == '__main__':
    sys.exit(main())
