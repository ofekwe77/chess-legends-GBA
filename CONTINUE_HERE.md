# CONTINUE HERE — CHESS LEGENDS: ARCANE WARS development journal

Read this first when resuming work. The specification is `MD.txt`. This file records what is actually done and
verified; nothing here is aspirational. Requirement inventory: `TASKS.md`.

## Build environment (verified 2026-10-18)

- Toolchain inside MSYS2 at `C:\msys64\opt`: devkitARM GCC 16.1, Butano 21.9.0, mGBA 0.10.5 (+ static libmgba
  in `/opt/mgba-dev` for headless testing). Host compiler: `C:\msys64\ucrt64\bin\g++` (GCC 16.2).
- The project path contains spaces/Hebrew: it is mapped to `Q:` with `subst`. All builds run from `Q:`.
- ROM (PowerShell): `$env:MSYSTEM="MSYS"; & C:\msys64\usr\bin\bash.exe -lc "cd /q && make -j8"`.
- Everything else through the dispatcher (Git Bash):
  `/c/msys64/usr/bin/bash.exe -lc "cd /q && bash tools/cl.sh <command>"` with `tests`, `validate`, `analyze`,
  `sim`, `harness`, `emu --rom ChessLegends.gba --script tests/emu/<name>.txt --out build_emu/<name>`.
- Python asset generators: `py -3.12 assets_source/gen_*.py` (stdlib only). When patching files from Python on
  Windows, always open with `newline='\n'` (text mode writes CRLF otherwise). Bash heredocs mangle backslashes:
  write patch scripts with the editor tools instead.

## Git

- Repository on branch `develop` (the user created the repo on `main`, still empty); commit after every logical
  unit with the Co-Authored-By trailer. `.gitattributes` keeps LF everywhere.

## Layout

- `core/` platform-independent engine and content (rules, Arcane abilities, game state, AI, saves, campaign,
  challenges, solvers, achievements, help texts, text layout). Never includes Butano; host-tested.
- `src/` GBA front end (Butano): scenes, board view, storage, art helpers.
- `assets_source/` art generators -> `graphics/`, `include/`, `core/cl_metrics_*.h`, previews.
- `tests/` host tests (`tests/emu/` emulator scenarios). `tools/` dispatcher, harness, balance, validation.

## Done and verified

1. Classic chess: 36 perft reference positions pass.
2. Arcane Chess: mana, all six abilities, authoritative transition, every interaction rule (castling, en
   passant, promotion, Royal Escape in mate detection, stalemate with spells, repetition identity, Arcane
   repetition window, dead positions, 50/75-move rules, 600-ply limit).
3. AI: negamax alpha-beta with the full feature set, 4 difficulties x 7 personalities, opening book,
   deterministic node budgets (Apprentice 350, Knight 1400, Sorcerer 3200, Grandmaster 6000), ~1000-1500
   nodes/s on the console, EWRAM search stack. Never use 64-bit division in ROM code.
4. Saves: versioned CRC records, A/B slots, profile + suspended match; SRAM layer with verified writes and
   recovery (`src/cl_storage.cpp`). Save-and-quit / resume verified in the emulator (identical position hash).
5. Campaign: 6 worlds, 18 encounters, dialogue for everything; 14 challenges; 6 gauntlet stages.
   `tools/cl.sh validate` proves every challenge and story puzzle with exhaustive solvers and checks every
   string against the real font metrics (2448 checks, 0 failures).
6. Achievements (32), statistics, gallery and board-skin unlock rules (`core/cl_progress.*`).
7. GBA: title, two-column main menu, match setup, match scene for every mode (objectives, achievements banner,
   statistics, results, pause menu per spec, R views, SELECT rule cards), story map, dialogue with portraits and
   typewriter text, per-kingdom boss introductions, briefing / reward / retry cards.
8. Art: pieces and armies, 8 board themes, fonts, UI, 14 portraits x 3 expressions, 8 backdrops, particles.
9. Tests: host 71 tests / 9167 checks; emulator scenarios `smoke` and `story` PASS.

## Next

- Scenes still to build: challenges list, gauntlet, gallery, achievements, options, credits, tutorial,
  intro and ending cinematics (create_scene currently routes them to the main menu).
- Audio (XM writer, 13 tracks, SFX, options), capture battle scenes and spell effects, title logo art.
- Test-interface mailbox commands, emulator scenarios for every mode, balance simulations, docs, release.
