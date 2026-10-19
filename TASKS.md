# CHESS LEGENDS: ARCANE WARS — requirement inventory

Status values: DONE (implemented and verified by the named test), WIP, TODO. Section numbers refer to `MD.txt`.
Emulator scenarios live in `tests/emu/` (run all with `bash tools/cl.sh emu-all`).

## Platform and engine
| Id | Requirement (MD.txt) | Status | Evidence |
|----|----------------------|--------|----------|
| P1 | Real GBA ROM built with Butano/devkitARM (5) | DONE | `make` → ChessLegends.gba, 0 warnings (`cl.sh check`) |
| P2 | Headless emulator verification (116) | DONE | tools/harness, 10 scenarios |
| P3 | Engine independent of graphics, host-runnable (6) | DONE | core/ + tests/ host build |
| P4 | No heavy allocation in search, fixed arrays (7) | DONE | SearchMemory, MoveBuffer, EWRAM search stack |
| P5 | UI responsive while AI thinks (7, 42) | DONE | ai_think yields every frame (smoke.txt, gameplay.txt) |

## Rules (12, 13-31)
| Id | Requirement | Status | Evidence |
|----|-------------|--------|----------|
| R1 | Full classic chess incl. castling, ep, promotion, underpromotion | DONE | test_rules.cpp, test_perft.cpp (36 positions) |
| R2 | Draws: 3/5-fold, 50/75, dead positions, agreement, resignation | DONE | test_rules.cpp; stalemate in gameplay.txt |
| R3 | Mana system, single authoritative transition | DONE | test_arcane.cpp |
| R4 | Six abilities with all restrictions | DONE | test_arcane.cpp; all six cast in spells.txt |
| R5 | Cross-system tests 1-17 (97) | DONE | test_arcane.cpp (1-15), test_save.cpp (16-17) |
| R6 | Save/reload during check / after regeneration (97.16-17) | DONE | test_save.cpp reload_during_check_and_after_regeneration |
| R7 | Animation skipping keeps state (97.18) | DONE | battle.txt: same position hash watched vs skipped |
| R8 | AI cancellation (97.19) | DONE (host) | test_ai.cpp search_can_be_cancelled |
| R9 | Repeated matches without leakage (97.20) | DONE | test_ai.cpp; emulator suites play many matches per run |

## AI (37-45)
| Id | Requirement | Status | Evidence |
|----|-------------|--------|----------|
| A1 | Negamax/AB/ID/PVS/TT/killers/history/quiescence/check ext. | DONE | core/cl_search*.cpp |
| A2 | 4 difficulties, personalities, opening choices | DONE | cl_ai.cpp, test_ai.cpp |
| A3 | Host AI-vs-AI simulator with metrics (45) | DONE | tools/balance/simulate.cpp |
| A4 | Balance report from executed simulations (107-110) | DONE | docs/BALANCE_REPORT.md: 3020 games, 23 experiments, logs in docs/balance_logs |

## Modes, UI, content
| Id | Requirement | Status | Evidence |
|----|-------------|--------|----------|
| M1 | Classic chess: human vs AI / hot-seat, color, difficulty, board | DONE | gameplay.txt (vs AI), battle.txt (hot-seat) |
| M2 | Arcane chess mode | DONE | smoke.txt, spells.txt, battle.txt |
| M3 | Story mode: 6 kingdoms, 18 encounters, dialogue, bosses, progression | DONE | story.txt, gameplay.txt; validate |
| M4 | Challenge mode ≥12 validated puzzles | DONE | 14 challenges proven by validate; modes.txt |
| M5 | Boss Gauntlet | DONE | modes.txt; validate (gauntlet positions) |
| M6 | Gallery, achievements (≥24), options, credits | DONE | 32 achievements; extras.txt |
| M7 | Tutorial (10 topics) | DONE | 10 lessons; tutorial.txt; validate (lesson goals) |
| M8 | Pause menu items (77) | DONE | match pause menu: resume, settings, rules, hint, claim/offer draw, save & quit, resign, quit |
| M9 | Saves: versioned, CRC, alternating records, suspend/resume (85-88) | DONE | test_save.cpp; story.txt save & quit / resume |

## Presentation
| Id | Requirement | Status | Evidence |
|----|-------------|--------|----------|
| V1 | Pieces, fonts, board themes, markers, cursor, UI panels, icons | DONE | assets_source/gen_*.py |
| V2 | Title logo, menu/map/kingdom/credits backgrounds | DONE | gen_logo.py (title logo), gen_backdrops.py (8 backdrops) |
| V3 | Portraits (6 bosses + player + supporting) | DONE | 14 portraits x 3 expressions |
| V4 | Battle sprites (6 classes, idle/attack/defeat), battle scenes | DONE | gen_battle.py; battle.txt |
| V5 | Spell effects (6), particles, atmosphere | DONE | spells.txt; particle fields in cinematics and map |
| V6 | Cinematics: intro, 6 boss intros, ending + credits | DONE | cinema.txt; boss introductions in story |
| S1 | 13 music tracks (tracker modules), SFX list (66-70) | DONE | 13 XM, 23 SFX; audio.txt |
| S2 | Audio options persist | DONE | audio.txt; profile save |

## Release
| Id | Requirement | Status | Evidence |
|----|-------------|--------|----------|
| Q1 | Performance measurements (100, 117) | DONE | tests/emu/perf.txt (with regression bounds); docs/TEST_REPORT.md section 5 |
| D1 | README, RULES, ARCHITECTURE, AI_DESIGN, ART, AUDIO, BALANCE, TEST, BUILDING docs | DONE | README.md, docs/*.md |
| D2 | release/ChessLegends.gba + final report (124) | DONE | tools/verify_release.sh passes; ROM committed in release/ |
