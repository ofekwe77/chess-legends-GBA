# Test report

Every result below comes from a run on 2026-10-19 of the commands shown, on the final sources and the release
ROM. Verified facts are stated as such; expectations and untested areas are listed separately at the end.

## Summary

| Layer | Command | Result |
|---|---|---|
| Host tests | `bash tools/cl.sh tests` | 72 tests, 9225 checks, 0 failures |
| Content validation (full proofs) | `bash tools/cl.sh validate` | 3210 checks, 0 failures |
| Glyph check | (part of validate) | 0 problems |
| Music module check | `tools/check_xm.py` (part of validate) | 0 failures, 13 modules |
| ROM build | `make -j8` | built, 0 warnings |
| ROM inspection | `tools/check_rom.py release/ChessLegends.gba` | all checks passed |
| Emulator scenarios | `tools/verify_release.sh` (on the release ROM) | 11 of 11 passed |
| Balance simulations | `bash tools/cl.sh balance` | 3020 games in 23 experiments, 0 illegal moves |
| Release verification | `bash tools/verify_release.sh` | passed |

Release ROM: `release/ChessLegends.gba`, 865,092 bytes, SHA-256
`73923bd5499baee14528828aec28685d4033091791d255ea3458b12750aa0164`.

## 1. Host tests (tests/, 72 tests)

The engine (`core/`) is compiled on the PC with every warning enabled and tested directly.

| File | Tests | What they establish |
|---|---|---|
| `test_perft.cpp` | 2 | move generation against 36 perft reference positions (exact node counts at depth); make / unmake restores every field |
| `test_rules.cpp` | 14 | piece movement, pawns, pins, check and double check, checkmate and stalemate, castling conditions, en passant, promotion and underpromotion, repetition claims and fivefold, the 50 and 75 move rules, dead positions, resignation and agreement, FEN validation, SAN |
| `test_arcane.cpp` | 19 | mana start / regeneration / cap; no mana for captures or checks; illegal actions cost nothing; each of the six abilities with all its restrictions; checkmate counting Royal Escape; stalemate counting spells; spells resolving check; en passant expiring after a spell; halfmove counter rules; position identity including mana and Royal Escape; repetition requiring equal mana; the repetition window across pawn magic; one action per turn; Arcane perft consistency |
| `test_save.cpp` | 8 | blank and garbage memory read as empty; profile round trip; corruption and unsupported versions rejected; alternating records surviving torn writes; position serialization; a suspended Arcane match resuming exactly; reloading during check and after regeneration; corrupted suspend records rejected |
| `test_ai.cpp` | 9 | always legal moves; deterministic and leak-free searches; mates in one and two; a magical mate; Royal Escape as the only defense; winning hanging material; cancellation; bounded personalities and difficulties; opening book personalities |
| `test_eval.cpp` | 3 | color symmetry; material and mana valued; lazy evaluation exact inside its window |
| `test_campaign.cpp` | 9 | every encounter, gauntlet stage and challenge sets up legally; encounter regeneration; quick proofs of the one-move challenges; objective rules; the material objective across a dead position; tutorial lessons and goals; text wrapping like the screen |
| `test_progress.cpp` | 8 | the 32 achievements; actions credited only to the person (and to nobody in computer-against-computer games); victories and strategy achievements; mode restrictions; statistics and unlocks committed once; the match recorder packing; campaign progress and unlocks; gallery texts |

The cross-system tests of MD.txt 97 are covered: 1 to 15 in `test_arcane.cpp`, 16 and 17 in `test_save.cpp`, 18
(skipping keeps the state) in the `battle` emulator scenario, 19 (cancelling the AI) in `test_ai.cpp`, 20 (repeated
matches) in `test_ai.cpp` and the long emulator runs.

## 2. Content validation (tools/validation/validate.cpp)

* **Puzzles proven by exhaustive search**: each of the 14 challenges and every story encounter with a forced
  objective (mate in N, forced promotion, forced material gain, survival) is solved by the solvers in
  `core/cl_solver.cpp`. The intended key move must work within the limit with the announced abilities.
* **Encounters, tutorial and gauntlet**: every position sets up, every lesson goal is reachable, every gauntlet stage
  is legal.
* **Text**: every string the game shows (dialogue, briefings, hints, lessons, help pages, rule cards, achievements,
  gallery lore, credits, the match panel's fixed strings and the worst case of each formatted panel and status
  line, duel captions) is checked against the real font metrics and the layout widths, and for glyphs missing from
  the fonts.

## 3. ROM build and inspection (MD.txt 114)

* Build command: `make -j8` (Butano 21.9.0, devkitARM GCC 16.1.0): succeeded with no warnings.
* `tools/check_rom.py` inspected the output as a GBA binary:
  * an ARM branch at the entry point, the boot logo, the fixed header byte 0x96 and a correct header checksum;
  * title `CHESSLEGENDS`, game code `CLAW`, maker `01`, and the save-type marker `SRAM_V113`;
  * all 83 graphics linked (from the linker map), the soundbank linked, all 13 music modules and 23 sound effects
    converted.

## 4. Emulator verification (MD.txt 115, 116)

The ROM was executed in mGBA 0.10.5 (libmgba, headless) by `tools/harness/cl_harness.c`. Scenarios press buttons,
wait for game state, compare values, take screenshots and record audio. All 11 passed on the release ROM.

| Scenario | Verifies |
|---|---|
| `smoke` | boot, intro skip, title, main menu, Arcane setup, a move, the computer's reply |
| `gameplay` | Classic Chess against the computer (three moves, three replies); stalemate; checkmate and rematch; campaign progress: a lesson won, a trial lost from the pause menu then retried and won, the first boss beaten and the second kingdom opened; victory and defeat screens; the three panel views |
| `story` | prologue, kingdom introduction, world map, encounter list, briefing, the first lesson match, save and quit, resume from the main menu (CONTINUE) |
| `modes` | challenge list, a challenge solved with a gold medal and an achievement, the next challenge; the boss gauntlet opened, lost and summarized |
| `tutorial` | lesson pages, a selection task, a move task, a wrong move resetting the lesson |
| `spells` | each of the six abilities cast in FULL animation with screenshots of every effect phase |
| `battle` | a capture duel from entrance to victory; skipping a duel gives the same position hash as watching it; FAST mode (no duel for a pawn, a fast duel for a queen); achievements waiting for the board; captured-piece icons |
| `cinema` | every shot of the opening and ending cinematics; the intro remembered across a reboot; the credits |
| `extras` | gallery (locked and unlocked entries, portraits, music, spell demonstrations), achievements and statistics, options (text speed), the erase confirmation, credits |
| `audio` | music playing (measured level) and following the scenes; music off and on; sounds for moves and captures |
| `perf` | the performance measurements of section 5, with regression bounds |

The checklist of MD.txt 115 maps to these scenarios:

| Item | Scenario |
|---|---|
| Classic Chess and Arcane Chess against the AI | gameplay, smoke |
| Local two-player | battle |
| Each magical ability | spells |
| Campaign progress, boss victory | gameplay |
| Challenge completion | modes |
| Achievement unlocking | modes, battle, gameplay |
| Save and reload | story |
| Defeat and retry | gameplay |
| Checkmate and stalemate | gameplay, modes |

The boss victory uses a diagnostic command that makes the computer resign through the normal resignation path:
bosses are full games, so no scripted line can win one. Everything after the resignation (result, progress,
dialogue, world unlock) is the game's normal flow.

## 5. Performance (MD.txt 100, 117)

Measured by `tests/emu/perf.txt` on the release ROM. A missed frame is a display refresh that came without a new
frame; the worst frame is the longest frame in percent of one refresh.

| Situation | Missed frames | Worst frame | Notes |
|---|---|---|---|
| Main menu, 300 frames | 0 | 21% | |
| Story map, 300 frames | 0 | 56% | |
| 12 scene loads | 18 | 287% | while the screen is black; the free heap returned to the same value (no leak) |
| A duel (FULL) | 1 | 127% | the duel's set-up frame |
| Spell effects (Royal Escape) | 0 | 53% | |
| SAVE & QUIT (suspended match and profile written, back to the menu) | 2 | 138% | includes the scene change |
| Computer against computer, animations off, a whole game (125 plies) | 21 | 140% | |
| A second whole game with FULL animations (132 plies, 27 duels, 32 spells, 45 captures) | 71 | 221% | the free heap ended where the first game left it |

* **Search speed**, an Arcane middlegame move per level: Apprentice 350 nodes in 23 frames (0.4 s), Knight 1142
  nodes in 117 frames (2.0 s), Sorcerer 3200 nodes in 306 frames (5.1 s), Grandmaster 5173 nodes in 474 frames
  (7.9 s). The interface keeps running at 60 fps meanwhile.
* **Memory**: the IWRAM stack kept 3452 bytes that were never touched across the whole scenario; the free heap
  was 57 KB in the menus and 41.6 KB during matches, stable across repeated matches. Peak sprite count: 70 of 128.
* **Profile counters** (worst case during computer-against-computer play): applying and presenting a move with
  animations off 92% of a frame, panel redraw 43%, the AI's start-up before its first poll 62%.
* Responsiveness (MD.txt 117): menus and cursor answer the first frame of a press (all scenarios drive them
  with 2-frame taps); no animation freezes the game (every scenario waits for the end of each animation); every
  AI turn completes; repeated matches and scene loads leave the memory unchanged; saving was verified by
  reloading (`story`); music and effects were measured during a match with full animations (`audio`).

Performance work done in response to these measurements:

* an AI pass that could freeze the console for 1.25 s was bounded;
* clearing the AI's tables at every move was replaced by generations;
* the search now yields at every node from a frame budget;
* the board and the panel redraw only what changed.

Missed frames in computer-against-computer play went from about 40% of all frames to under 1%.

## 6. Balance (MD.txt 107-110)

3020 complete games in 23 experiments (`docs/balance_logs/`), with no illegal move and no crash. Results and
interpretation: [BALANCE_REPORT.md](BALANCE_REPORT.md). No rule value was changed.

## 7. Known issues and what was not verified

* **Not run on real hardware.** Every result above is from mGBA. Timing should match closely (mGBA is
  cycle-accurate for most purposes), but flash cartridges and real SRAM were not tried.
* **Audio quality** was verified by measurement (levels, scene changes, keys), not by listening tests.
* **Loading frames**: scene changes take one or two frames while the screen is black.
* **AI**: slow at the highest level (about 8 s per middlegame move); rarely casts Castle Swap and Royal Command.
* **Duel art**: the battle characters are small, simply drawn 64x64 sprites.
* The headless tests cover the scripted paths listed above; free play beyond them (for example every encounter
  played to the end by a person) was not exhaustively performed. The validator proves every puzzle's solution,
  and the simulator plays thousands of complete games through the same rules engine.
