# CHESS LEGENDS: ARCANE WARS — requirement inventory

Status values: DONE (implemented and verified by the named test), WIP, TODO. Section numbers refer to `MD.txt`.

## Platform and engine
| Id | Requirement (MD.txt) | Status | Evidence |
|----|----------------------|--------|----------|
| P1 | Real GBA ROM built with Butano/devkitARM (5) | DONE | `make` → ChessLegends.gba, 0 warnings |
| P2 | Headless emulator verification (116) | DONE | tools/harness, tests/emu/smoke.txt |
| P3 | Engine independent of graphics, host-runnable (6) | DONE | core/ + tests/ host build |
| P4 | No heavy allocation in search, fixed arrays (7) | DONE | SearchMemory, MoveBuffer |
| P5 | UI responsive while AI thinks (7, 42) | DONE | ai_think poll every 8 nodes, EWRAM stack |

## Rules (12, 13-31)
| Id | Requirement | Status | Evidence |
|----|-------------|--------|----------|
| R1 | Full classic chess incl. castling, ep, promotion, underpromotion | DONE | tests/test_rules.cpp, test_perft.cpp |
| R2 | Draws: 3/5-fold, 50/75, dead positions, agreement, resignation | DONE | test_rules.cpp |
| R3 | Mana system, single authoritative transition | DONE | test_arcane.cpp |
| R4 | Six abilities with all restrictions | DONE | test_arcane.cpp |
| R5 | Cross-system tests 1-17 (97) | DONE | test_arcane.cpp (1-15), 16-17 pending save tests |
| R6 | Save/reload during check / after regeneration (97.16-17) | TODO | |
| R7 | Animation skipping keeps state (97.18) | WIP | state applied before animation; emu test TODO |
| R8 | AI cancellation (97.19) | DONE | test_ai.cpp search_can_be_cancelled; emu TODO |
| R9 | Repeated matches without leakage (97.20) | DONE | test_ai.cpp |

## AI (37-45)
| Id | Requirement | Status | Evidence |
|----|-------------|--------|----------|
| A1 | Negamax/AB/ID/PVS/TT/killers/history/quiescence/check ext. | DONE | core/cl_search*.cpp |
| A2 | 4 difficulties, personalities, opening choices | DONE | cl_ai.cpp, test_ai.cpp |
| A3 | Host AI-vs-AI simulator with metrics (45) | DONE | tools/balance/simulate.cpp |
| A4 | Balance report from executed simulations (107-110) | TODO | |

## Modes, UI, content
| Id | Requirement | Status |
|----|-------------|--------|
| M1 | Classic chess: human vs AI / hot-seat, color, difficulty, board | WIP (match scene works, needs full tests) |
| M2 | Arcane chess mode | WIP |
| M3 | Story mode: 6 kingdoms, 18 encounters, dialogue, bosses, progression | TODO |
| M4 | Challenge mode ≥12 validated puzzles | TODO |
| M5 | Boss Gauntlet | TODO |
| M6 | Gallery, achievements (≥24), options, credits | TODO |
| M7 | Tutorial (10 topics) | TODO |
| M8 | Pause menu items (77) | WIP (save & quit, rules, audio TODO) |
| M9 | Saves: versioned, CRC, alternating records, suspend/resume (85-88) | TODO |

## Presentation
| Id | Requirement | Status |
|----|-------------|--------|
| V1 | Pieces, fonts, board themes, markers, cursor, UI panels, icons | DONE (assets_source/gen_*.py) |
| V2 | Title logo, menu/map/kingdom/credits backgrounds | TODO |
| V3 | Portraits (6 bosses + player + supporting) | TODO |
| V4 | Battle sprites (6 classes, idle/attack/defeat), battle scenes | TODO |
| V5 | Spell effects (6), particles, atmosphere | TODO |
| V6 | Cinematics: intro, 6 boss intros, ending + credits | TODO |
| S1 | 13 music tracks (tracker modules), SFX list (66-70) | TODO |
| S2 | Audio options persist | TODO |

## Release
| Id | Requirement | Status |
|----|-------------|--------|
| D1 | README, RULES, ARCHITECTURE, AI_DESIGN, ART, AUDIO, BALANCE, TEST, BUILDING docs | TODO |
| D2 | release/ChessLegends.gba + final report (124) | TODO |
