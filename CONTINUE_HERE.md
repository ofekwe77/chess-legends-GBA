# CONTINUE HERE — CHESS LEGENDS: ARCANE WARS development journal

Read this first when resuming work. The specification is `MD.txt`. This file records what is actually done and
verified; nothing here is aspirational.

## Build environment (verified 2026-10-17)

- Toolchain inside MSYS2 at `C:\msys64\opt`: devkitARM GCC 16.1, Butano 21.9.0, mGBA 0.10.5 (+ static libmgba
  in `/opt/mgba-dev` for headless testing). Host compiler: `C:\msys64\ucrt64\bin\g++` (GCC 16.2).
- The project path contains spaces/Hebrew: `build.ps1` maps it to `Q:` with `subst`. All builds run from `Q:`.
- Commands (PowerShell): `.\build.ps1 tests`, `.\build.ps1 rom`, `.\build.ps1 sim --games 20 ...`.
  From Git Bash: `/c/msys64/usr/bin/bash.exe -lc "cd /q && ./tools/cl.sh tests"`.
- Python helpers: use `py -3.12` (Windows) or MSYS2 `python3`; stdlib only (no Pillow / numpy available).

## Layout

- `core/` platform-independent engine (rules, Arcane abilities, game state, AI). Never includes Butano.
- `src/` GBA code (Butano). `tests/` host tests. `tools/balance/` AI simulator. `tools/cl.sh` dispatcher.

## Done and verified

1. Classic chess rules: 36 perft reference positions pass (start d5, Kiwipete d4, positions 3-6, edge cases).
2. Arcane Chess: mana (start 1/0, +1 per turn, cap 6), all six abilities, authoritative transition, castling /
   en passant / promotion interactions, Royal Escape in checkmate detection, stalemate with spells, repetition
   identity incl. mana / escapes / pawn double-step eligibility, Arcane repetition window (pawn magic is
   reversible), dead positions, 50/75-move rules, Arcane 600-ply safety limit. Host tests: 47 tests, 7019 checks.
3. AI: negamax alpha-beta, iterative deepening, PVS, TT, killers, history, check extension, LMR, null move
   (mana-aware), check-aware quiescence with bounded magical checks, eval cache, lazy eval, pins-based legality
   skip (verified by host debug assertion), deterministic node budgets, 4 difficulties x 7 personalities, opening
   book with style tags, seeded controlled imperfection for low levels, draw-claim policy, cancellation.
4. Console performance (measured in mGBA): ~1000-1500 nodes/s. Hot code in IWRAM (ARM): position make/unmake +
   attacks, move generation, evaluation. AI search runs on a dedicated 8 KB EWRAM stack (`src/cl_stack_switch.s`)
   with a stack guard. IMPORTANT: never use 64-bit division in ROM code (links a 3 KB routine into IWRAM); keep
   big globals in EWRAM (`BN_DATA_EWRAM_BSS`), plain globals land in IWRAM .bss.
5. Budgets (nodes): Apprentice 350, Knight 1400, Sorcerer 3200, Grandmaster 6000.

## In progress / next

- `src/main.cpp` is still the benchmark entry point; next: GBA game shell (scenes, input, text, board UI), art
  pipeline (`assets_source/`), audio pipeline, campaign, saves, harness (`tools/harness/`).
