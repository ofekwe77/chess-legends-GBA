# Building CHESS LEGENDS: ARCANE WARS

The finished ROM is `release/ChessLegends.gba`; you do not need to build anything to play it. This document is
for rebuilding the ROM, regenerating the assets and running the tests.

## Toolchain

The project was built and verified on Windows 11 with this toolchain inside MSYS2 (`C:\msys64`):

| Tool | Version | Location | Used for |
|---|---|---|---|
| devkitARM (devkitPro) | GCC 16.1.0 | `/opt/devkitpro` | the GBA compiler |
| Butano | 21.9.0 | `/opt/butano` (git checkout) | the GBA engine (C++23) |
| MSYS2 UCRT64 GCC | 16.2 | `/ucrt64/bin` | host tests, validator, simulator, harness |
| Python | 3.12 (standard library only) | MSYS2 `python3` or Windows `py -3.12` | asset generators, checks |
| mGBA | 0.10.5 | `/opt/mgba` (player), `/opt/mgba-dev` (static libmgba) | playing, headless tests |

On Linux or macOS the same tools work; only the path mapping below is Windows specific. Set `LIBBUTANO` (Makefile)
if Butano lives elsewhere.

Butano's own requirements apply (devkitARM with its `gba-dev` packages, Python 3 for the asset conversion). The
headless harness needs a static libmgba: `tools/harness/build_libmgba.sh` downloads mGBA 0.10.5 and builds the GBA
core only (cmake and ninja from MSYS2 UCRT64).

### Paths with spaces or non-ASCII characters

GNU make and GCC cannot handle a project path with spaces or non-ASCII characters (the original folder has both).
`build.ps1` maps the project folder to an ASCII drive letter with `subst` (default `Q:`, change with `CL_DRIVE`).
The mapping lasts for the Windows session and needs no administrator rights.

## Commands

From PowerShell in the project folder:

```powershell
.\build.ps1                 # regenerate the assets, build the ROM, copy it to release\ChessLegends.gba
.\build.ps1 rom             # build the ROM only (the generated assets are committed)
.\build.ps1 tests           # host test suite
.\build.ps1 verify          # full release verification (see below)
```

Inside MSYS2 (from the project root, an ASCII path), every command goes through the dispatcher:

```bash
bash tools/cl.sh all        # assets + ROM + release copy
bash tools/cl.sh rom        # make -j8, then copy to release/
bash tools/cl.sh assets     # python assets_source/generate_all.py
bash tools/cl.sh tests      # build_host/run_tests.exe: rules, abilities, AI, saves, campaign, progress
bash tools/cl.sh validate   # prove every puzzle, fit every string to the fonts (--quick: shallower proofs)
bash tools/cl.sh harness    # build the headless mGBA harness (tools/harness/build/cl_harness.exe)
bash tools/cl.sh emu --rom ChessLegends.gba --script tests/emu/smoke.txt --out build_emu/smoke
bash tools/cl.sh emu-all    # every emulator scenario in tests/emu/
bash tools/cl.sh check      # pre-commit gate: tests, quick validation, warning-free ROM, all scenarios
bash tools/cl.sh sim --games 20 --white 1 --black 1 --alternate   # AI against AI (tools/balance)
bash tools/cl.sh balance    # the whole balance study (logs in docs/balance_logs)
bash tools/cl.sh verify     # tools/verify_release.sh
bash tools/cl.sh clean
```

Plain `make -j8` in the project root also builds `ChessLegends.gba` (Butano's makefile, title `CHESSLEGENDS`, game
code `CLAW`, SRAM save).

## Release verification

`tools/verify_release.sh` performs, and logs to `build_host/verify_release.txt`:

1. the host test suite;
2. the full content validation;
3. a ROM build that fails on any compiler warning, copied to `release/ChessLegends.gba`;
4. `tools/check_rom.py`: the cartridge header (entry branch, boot logo, fixed byte, checksum, title and codes), the
   SRAM save-type marker, every graphics asset linked and every module and effect converted;
5. every emulator scenario of `tests/emu/` run against the release ROM;
6. the release ROM's size and SHA-256.

## Assets

Every graphic and sound is generated from source by deterministic Python scripts in `assets_source/` (standard
library only): fonts, pieces, boards, interface, icons, portraits, backdrops, effects, battle characters, the
title logo, the 13 music modules (written as XM files by `assets_source/audio/xm.py`) and the 23 sound effects.
`generate_all.py` runs them all; the outputs (`graphics/`, `audio/`, `include/`, `core/cl_metrics_*.h`) are
committed, so building the ROM does not require Python beyond what Butano needs. Previews of the art are written
to `assets_source/previews/`.

## Layout

```
core/            platform-independent engine and content (no Butano): rules, abilities, AI, saves, campaign
src/             GBA front end (Butano): scenes, board view, battles, effects, audio, storage, test mailbox
assets_source/   art and audio generators          graphics/, audio/   generated assets (Butano input)
tests/           host tests                        tests/emu/          emulator scenarios
tools/           dispatcher, harness, validation, balance simulator, checks
docs/            documentation, balance logs       release/            the ROM
```

## Running the ROM

Open `release/ChessLegends.gba` in mGBA (or any accurate GBA emulator, or a flash cartridge). The game saves to
battery-backed cartridge SRAM (it uses 10 KB); emulators keep it in a `.sav` file next to the ROM.
