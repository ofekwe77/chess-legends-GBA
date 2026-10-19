# Architecture

CHESS LEGENDS: ARCANE WARS is written in C++23 for the Game Boy Advance with the Butano engine. The code is split
so that everything that decides the game can run and be tested on a PC:

```
core/   the game itself: rules, Arcane abilities, game state, AI, saves, campaign, progress   (no Butano)
src/    the GBA front end: scenes, board, battles, effects, audio, storage, test mailbox     (Butano)
tools/  dispatcher, headless emulator harness, content validator, balance simulator, checks
tests/  host tests (core) and emulator scenarios (tests/emu, the whole ROM)
assets_source/  generators of every graphic and sound (Python, deterministic)
```

`core/` never includes a Butano header. It is compiled twice: into the ROM, and on the host into the test suite,
the validator (`tools/validation`) and the simulator (`tools/balance`). A rule therefore has exactly one
implementation, used by the console, the tests and the tools.

## 1. The rules core

| Module | Responsibility |
|---|---|
| `cl_types.h`, `cl_config.h` | squares (0x88 board), pieces, colors, piece values; every tunable number of the rules |
| `cl_position.*` | the position: board, piece lists, side, castling, en passant, mana, Royal Escape uses, pawn history, halfmove / fullmove, Zobrist hash; FEN with Arcane fields; `make` / `unmake` |
| `cl_movegen.*` | pseudo-legal and legal generation of normal moves and of the six abilities; attack detection; legality filters |
| `cl_move.h` | a 32-bit packed action: from, to, auxiliary square (Royal Command's queen), kind, capture flag, promotion |
| `cl_game.*` | the match: the single authoritative transition `Game::apply`, the result and its reason, repetition window, draw claims, resignation, agreement, scenario results, restoring a saved game |
| `cl_notation.*` | coordinate notation (`ss:d2c2`, `rc:e2f2@d1`), SAN-style display text, names |

The files named `*.bn_iwram.cpp` (position, move generation, evaluation) are compiled by Butano as ARM code in the
GBA's fast internal RAM: they dominate the cost of a search node.

### The one transition

`Game::apply(move, &info)` is the only function that changes a match. It rejects an illegal action, plays it
(`Position::make`: pieces, rights, counters, mana cost, turn passed, the next player's mana regeneration), appends
the new position to the repetition window and updates the result (checkmate, stalemate, automatic draws). It fills
an `ActionInfo` describing what happened: mover, moving piece, captured piece and square, mana spent, check.

Everything else only reads: the match scene animates `ActionInfo`, the objective tracker and the achievement
recorder observe it, the AI searches copies of the position. Because the state is final before any presentation,
skipping an animation cannot change it. An emulator test compares the position hash after a watched and after a
skipped duel.

## 2. Content and progress (core)

| Module | Responsibility |
|---|---|
| `cl_campaign.*`, `cl_campaign_data.cpp` | worlds, characters, the 18 encounters (positions, objectives, modifiers, dialogue), 14 challenges, 6 gauntlet stages; the objective tracker |
| `cl_solver.*` | exhaustive solvers (forced mate, forced promotion, forced material gain) used by the validator to prove every puzzle |
| `cl_tutorial.*` | the 10 lessons: pages, positions and goals |
| `cl_progress.*` | 32 achievements, the match recorder (what happened in a match, packed into 20 bytes for suspended matches), statistics, gallery and board unlock rules derived from progress facts |
| `cl_help.*` | rule cards, help pages, spell demonstrations, credits |
| `cl_text_layout.*`, `cl_metrics_*.h`, `cl_ui_text.h` | font metrics (generated), word wrapping and the layout widths shared by the game and the validator; the fixed wording of the match panel |
| `cl_save.*` | the save format: versioned records with CRC-32, profile and suspended match serialization, validation of every field on load |

Only facts are saved (cleared encounters, medals, unlocked achievements, statistics); unlocks such as gallery
entries are computed from them, so an unlock can never disagree with the progress that earned it.

## 3. The AI (core)

`cl_eval.*` (evaluation), `cl_search*.cpp` (the search), `cl_ai.*` (levels, personalities, opening book,
controlled imperfection). See [AI_DESIGN.md](AI_DESIGN.md). The search is deterministic for a given position,
level and seed: budgets are counted in nodes, never in time.

## 4. The GBA front end (src/)

### Main loop and scenes

`main.cpp` paints the free stack (for the headroom measurement), initializes Butano, loads the profile and opens
the intro (first boot) or the title. Each frame it lets the test mailbox act, then updates the current scene. When a
scene asks for another one, the loop fades out, destroys the scene, runs one empty frame (so Butano releases the
old scene's graphics before the new scene allocates its own), creates the next scene and fades in.

| Scene (`SceneId`) | File |
|---|---|
| intro, ending | `cl_scene_cinema.cpp` |
| title, main menu, match setup | `cl_scene_menus.cpp` |
| match (every mode) | `cl_scene_match.*` |
| story map, story dialogue | `cl_scene_story.cpp` |
| challenges, gauntlet | `cl_scene_modes.cpp` |
| gallery, achievements and statistics, options, credits | `cl_scene_extras.cpp` |
| tutorial lesson list | `cl_scene_tutorial.cpp` |

`frame_end()` (`cl_scene.cpp`) finishes every frame: statistics, test interface, audio pacing, `bn::core::update()`.

### The match scene

`MatchScene` plays Classic, Arcane, story, challenge, gauntlet and tutorial matches. It is a state machine:

```
HUMAN -> SELECTED -> (MAGIC / COMMAND) -> PROMOTION?      the player's input
ANIMATING -> BATTLE?                                       presentation of the applied action
AI                                                         the computer's turn (search runs with the UI alive)
HANDOFF                                                    two players: pass the console
PAUSE / SETTINGS / HELP / CONFIRM / GAME_OVER / LESSON     overlays
```

An overlay never handles the key press that opened it. Supporting objects:

* `BoardView` (`cl_board_view.*`): the board and marker layers (tile maps kept in RAM, only changed squares
  rewritten), piece sprites kept per square (a move touches two or three sprites), cursor, highlights.
* `PanelLayer` (`cl_layers.*`): window frames on a tile layer.
* `TextCache` (`cl_text.*`): panel and status lines that keep their sprites while unchanged.
* `EffectLayer` (`cl_art.*`): short-lived particles and 16x16 animated effects for spells and impacts.
* `BattleScene` (`cl_battle.*`): the capture duel. It is a data-driven timeline of keys (pose, run, flash, shake,
  hit-stop, sound, burst, effect, spell circle, projectile, fade, fall) for each class's attack and defeat, plus an
  entrance, a camera that follows the attacker and a victory pose.
* `ObjectiveTracker`, `MatchRecorder` (core): scenario objectives, achievements and statistics.

### The AI on the console

`cl_ai_runner.cpp` runs the search on its own 8 KB stack in external RAM, so deep recursion never threatens the small
internal stack. The search calls a poll function at every node. Once 75% of the current frame has been used, the
poll calls back into the match scene (`think_frame`), which ends the frame, then animates, reads START (pause) and
redraws what changed at the start of the next one. The game keeps 60 fps while the computer thinks. A pause
request cancels the search; it restarts deterministically on resume.

### Other services

| Module | Responsibility |
|---|---|
| `cl_storage.*` | SRAM access: profile and suspended match in two alternating slots each, every write read back and verified, load status (fresh, loaded, recovered from the backup, reset) |
| `cl_audio.*` | music and sound requests, honoring the audio settings; rate-limits sounds so bursts never distort |
| `cl_art.*` | backdrops, portraits, particle fields, the mini board, the effect layer |
| `cl_text.*` | the two sprite fonts (small, title), alignment, wrapping, the text cache |
| `cl_globals.*` | the global state in external RAM: game, search memory, transposition table, profile, suspended match scratch, match setup, story flow |
| `cl_test_commands.*` | the diagnostic mailbox of the test interface (section 6) |

## 5. Memory and hardware budgets

| Resource | Use |
|---|---|
| IWRAM (32 KB) | ARM code of position / move generation / evaluation, static data, the stack. Large objects (`MoveList`, `Game`, save records) never go on the stack. The headroom is measured on the console: about 3.4 KB never touched across all scenarios. |
| EWRAM (256 KB) | `Globals` (game, search memory, 4096-entry transposition table, profile), the AI stack, Butano's heap (about 41 to 57 KB left free during play) |
| Sprites | at most 128; the board releases its piece sprites during a duel. Measured peak: 70 |
| Sprite tile items | raised to 256 (`BN_CFG_SPRITE_TILES_MAX_ITEMS`) for the many small text sprites |
| Background layers | at most 4; the match uses board, markers, panel, and a backdrop during duels |
| SRAM | profile A/B at 0x0000 / 0x0400 (1 KB each), suspended match A/B at 0x0800 / 0x1800 (4 KB each) |

Code avoids 64-bit divisions (they would link a large routine into IWRAM) and integer divisions in hot paths (the
ARM7 has no divide instruction).

## 6. The test interface

`core/cl_test_interface.h` defines a C structure that the ROM keeps in external RAM and refreshes every frame:
scene, match state, position hash, mana, results, AI statistics, audio state, save state, CPU and memory
measurements, profile counters. Two magic words let the headless harness (`tools/harness/cl_harness.c`, built on
libmgba) find it. Scenarios (`tests/emu/*.txt`) press buttons, wait for fields, compare values, take screenshots,
record audio and post diagnostic commands (`cmd N args`), for example: start a match or an encounter, play a packed
move, set the animation mode, unlock everything, wipe the save, set up a test position, make the computer resign.
No button combination reaches the mailbox; normal play never depends on it.

## 7. Determinism and randomness

The rules have no randomness at all. The AI is deterministic for a given seed. The match seed varies the computer's
opening choices and its lower levels' imperfections; the rules never see it. Cosmetic effects (particles, stars) use
a separate generator.
