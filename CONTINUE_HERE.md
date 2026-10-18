# CONTINUE HERE — CHESS LEGENDS: ARCANE WARS development journal

Read this first when resuming work. The specification is `MD.txt`. This file records what is actually done and
verified; nothing here is aspirational. Requirement inventory with evidence: `TASKS.md`.

## Build environment (verified 2026-10-18)

- Toolchain inside MSYS2 at `C:\msys64\opt`: devkitARM GCC 16.1, Butano 21.9.0, mGBA 0.10.5 (+ static libmgba
  in `/opt/mgba-dev` for headless testing). Host compiler: `C:\msys64\ucrt64\bin\g++` (GCC 16.2).
- The project path contains spaces/Hebrew: it is mapped to `Q:` with `subst` (`build.ps1` does this).
- ROM (PowerShell): `$env:MSYSTEM="MSYS"; & C:\msys64\usr\bin\bash.exe -lc "cd /q && make -j8"`.
- Everything else through the dispatcher (Git Bash):
  `/c/msys64/usr/bin/bash.exe -lc "cd /q && bash tools/cl.sh <command>"` with `tests`, `validate [--quick]`,
  `analyze`, `sim`, `harness`, `emu --rom ChessLegends.gba --script tests/emu/<name>.txt --out build_emu/<name>`,
  `emu-all`, `check`.
- `bash tools/cl.sh check` is the pre-commit gate (host tests, quick validation, glyph + XM checks, warning-free
  ROM, every emulator scenario). It must exit 0 before every commit.
- Python asset generators: `py -3.12 assets_source/gen_*.py` (stdlib only). Open files with `newline='\n'`.
  Bash heredocs with apostrophes break: write patch scripts to the scratchpad with the editor tool.
- Small host tools need the UCRT64 environment: `$env:MSYSTEM="UCRT64"` before calling MSYS2 bash.

## Pitfalls learned the hard way

- Never stream a `char` array into `bn::ostringstream`: the template overload appends the whole array
  (terminator and garbage included). Wrap it in `bn::string_view`.
- A glyph missing from the font crashes Butano: `tools/check_glyphs.py` checks literals; the validator checks
  panel strings and formatted worst cases. Side panel text is 84 px wide (`layout::panel_text_width`).
- Big objects never on the IWRAM stack (`MoveList` 1.3 KB, `SuspendedMatch`, `Game`): stack headroom is
  measured on the console (`stack_free`, about 3.5 KB never touched across the suites).
- Sprite tiles items are raised to 256; sprites are capped at 128 (the board releases its pieces during duels).
- One `frame_end()` between destroying a scene and creating the next.

## Git

- Branch `develop` (the user's `main` is empty). Commit after every logical unit with the Co-Authored-By
  trailer. `.gitattributes` keeps LF everywhere.

## Layout

- `core/` platform-independent engine and content (rules, Arcane abilities, game state, AI, saves, campaign,
  challenges, solvers, achievements, help, tutorial, text layout, UI wording). Never includes Butano; host-tested.
- `src/` GBA front end (Butano): scenes, board view, battle scenes, effects, audio, storage, test mailbox.
- `assets_source/` art and audio generators -> `graphics/`, `audio/`, `core/cl_metrics_*.h`, previews.
- `tests/` host tests; `tests/emu/` emulator scenarios. `tools/` dispatcher, harness, balance, validation.

## Done and verified

1. Classic chess (36 perft positions) and Arcane Chess (mana, six abilities, every interaction rule).
2. AI: negamax alpha-beta, 4 difficulties x 7 personalities, opening book, deterministic node budgets.
3. Saves: versioned CRC records, A/B slots, profile + suspended match, verified SRAM writes.
4. Content: 6 kingdoms, 18 encounters with dialogue, 14 challenges, 6 gauntlet stages, 10 tutorial lessons,
   32 achievements, gallery, statistics; every puzzle proven and every string fitted by `tools/cl.sh validate`.
5. Every scene: intro and ending cinematics, title, main menu, match setup, match (all modes), story map and
   dialogue, challenges, gauntlet, gallery, achievements, options, credits, tutorial.
6. Audio: 13 original XM tracks and 23 sound effects, played throughout, options persisted.
7. Capture battle scenes (6 classes, attacks, defeat reactions, camera, floor, hit-stop, skip) and board spell
   effects for all six abilities (16x16 effect frames, tint palettes).
8. Tests: host 72 tests / 9222 checks; validation 3157 checks; emulator scenarios 10/10 (audio, battle,
   cinema, extras, gameplay, modes, smoke, spells, story, tutorial).

## Next

- Performance pass (MD.txt 100/117): explain `fps_drops` / `cpu_max` (scene loads, saves, AI slices),
  measure AI speed, transitions, saves, long matches, repeated scene loading.
- Balance simulations and BALANCE_REPORT (MD.txt 107-110).
- Art polish: thicker battle figures, a title logo.
- Docs: README, RULES, ARCHITECTURE, AI_DESIGN, ART_DIRECTION, AUDIO_DESIGN, BALANCE_REPORT, TEST_REPORT,
  BUILDING. Release: `release/ChessLegends.gba`, `tools/verify_release.sh`. Final 13-section report.
