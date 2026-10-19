# Balance report

The magical system was tested by simulation, not assumed balanced (MD.txt 107-110). This report covers
**3020 complete games** in 23 experiments, played by the game's own AI with the console's node budgets. Every
number comes from the logs in `docs/balance_logs/`; the tables at the end are generated from those logs by
`tools/balance/summarize.py`. Rerun everything with `bash tools/cl.sh balance` (5 minutes on a PC; the results
reproduce exactly).

## Verdict

* Magic matters: a side with the six abilities beat a side without them by 135 wins to 53 (12 draws).
* No ability dominates. Shadow Step is the most valuable (removing it costs 14 points of score), and the other
  abilities change results by less than the measurement noise when removed one at a time.
* The economy works as designed: mana is spent all game long and refills, games stay decisive (about 90% end in
  checkmate at Knight level) and finite (no game came near the 600-ply limit).
* The difficulty levels are clearly ordered.
* **No rule value was changed.** The data showed no defect that a cost or limit change would fix without new
  risks (see "Adjustments").

## Method

* **Simulator**: `tools/balance/simulate.cpp`, the AI against itself on a PC. The levels use exactly the console's
  deterministic node budgets (350 / 1400 / 3200 / 6000 nodes), so the PC plays the same moves the console would.
* **Variety**: each game starts with 4 seeded random legal opening plies (the opening book is off), and each side
  has its own seed for the lower levels' controlled imperfection. With `--alternate`, games come in pairs with
  the same opening and the two configurations exchanging colors, so first-move advantage cancels out of
  configuration comparisons.
* **Limits**: the simulator stops a game at 400 plies and counts it as a draw ("ply-capped"). This happened in 4
  of 3020 games. The game's own limit is 600 plies.
* **Metrics** (MD.txt 108): results by color and by configuration, game length, how games end, uses of each
  ability (Royal Escape included), mana stored when a decision is made, the caster's material balance four plies
  after each cast, the conversion of decisive leads (a lead of three pawns or more held over a full move), and
  search speed.
* **Uncertainty**: shares are given with their standard error. With 100 to 200 games, differences below 8 to 10
  points of score are not distinguishable from chance.

## Findings

### 1. Arcane and Classic Chess (e1, e2)

Knight against Knight, 200 games each:

| | Arcane | Classic |
|---|---|---|
| White / Black / Draw | 100 / 85 / 15 | 112 / 74 / 14 |
| White's score | 53.8% | 59.5% |
| Average length | 125.4 plies | 108.7 plies |
| Checkmates | 92.5% | 93.0% |

Arcane games are about 17 plies longer: spells reposition instead of trading. White's score is lower in Arcane
(53.8% against 59.5%), which is consistent with Black's magic offsetting part of the first move. The difference is
only about one standard error, though, so this is a tendency, not a proven effect. Draw rates are the same.

### 2. What magic is worth (e3, e4)

| Configuration A against B | A's score |
|---|---|
| All abilities against none | 70.5% (±3.2) |
| All against all but Shadow Step | 64.2% (±4.4) |
| All against all but Royal Escape | 54.2% (±4.5) |
| All against all but Phase Shift | 52.1% (±4.6) |
| All against all but Royal Command | 49.2% (±4.6) |
| All against all but Castle Swap | 50.0% (±4.6) |
| All against all but Ghost Walk | 47.5% (±4.6) |

Abilities are a large advantage together. Taken one at a time, only Shadow Step shows a clear effect at this
level. It is the cheapest ability, available to the most pieces, and it reshapes pawn structures, which the
evaluation understands well. Royal Escape may help a little (54%), but within the noise.

### 3. How often each ability is used (e1)

In 200 Knight-level Arcane games: Shadow Step 1383 casts, Phase Shift 616, Ghost Walk 424, Royal Escape 221,
Royal Command 33, Castle Swap 25. That is 13.5 casts per game, about one action in nine, and 36 mana spent per
game. Royal Escape was used in 221 of 400 player-games (55%), which is expected for an emergency defense that
needs check, 5 mana and an unused charge.

Castle Swap and Royal Command are rare. They depend on particular piece arrangements (a rook in line with a nearby
bishop or knight; a queen next to an ally with a useful step), and a depth-2 search seldom finds them best. Rare
use is not weakness by itself (MD.txt 108). Removing either changed nothing measurable here, and two challenges
show positions where each is the winning move.

### 4. Mana economy (e1, e5, e6)

* The AI holds 5.5 of 6 mana on average when it decides. Mana is seldom the limit at this level: the computer keeps
  a reserve, in particular for Royal Escape (cost 5).
* **Cheaper costs** (every cost minus one): more casts (Phase Shift +54%, Royal Command +79%), longer games (131.4
  plies) and more draws (10.5% against 7.5%).
* **Higher costs** (every cost plus one): fewer casts (Phase Shift -30%, Ghost Walk -35%), similar length and draws.
* **Mana cap 4**: Royal Command and Royal Escape become impossible (both cost 5): shorter games (115.6 plies), fewer
  draws (5%). **Cap 8**: more of the expensive spells (Royal Command 84 casts), longer games (132.2 plies).

The default costs sit between the variants. Making magic cheaper adds spell use but also draws and length;
making it dearer removes much of the magic. Neither variant fixes anything.

### 5. Material after a cast

The caster's material balance four plies after a cast is, on average, slightly negative for every ability:
-12 to -37 centipawns, and -72 for Royal Escape (e1). This is expected. No ability captures, so a cast never wins
material by itself, and spells are often cast in tense positions to defend, relocate or escape. Royal Escape is
cast only while in check, the most dangerous moment of a game. The metric found no ability that wins material
directly, as the design intends ("magic changes the geometry, chess decides captures").

### 6. Decisive leads

A side that reached a lead of three pawns or more (held over a full move) went on to win 79.5% of Arcane games
(140 of 176), drew 3%, and lost 17%. In Classic Chess the figures are 83% won, 4% drawn and 13% lost. Comebacks
are a little more frequent with magic, but a material lead still decides most games. At Grandmaster level
(mirror, 40 games) leads were converted 34 times in 38.

### 7. Difficulty ladder (e7)

| Match | Stronger level's score |
|---|---|
| Knight against Apprentice | 96.5% (96 wins, 3 losses, 1 draw) |
| Sorcerer against Knight | 84.5% (83, 14, 3) |
| Grandmaster against Sorcerer | 65.0% (38, 20, 2) |

Every step is a clear improvement. The Grandmaster mirror (40 games) ended 18 to 18 between the configurations,
with White winning 14 and Black 22. That color split is within the noise of 40 games.

### 8. Personalities (e8)

Against Balanced at Knight level (100 games each): Master 55.0%, Aggressive 49.5%, Tactical 49.5%, Trickster
45.0%, Positional 44.0%, Defensive 41.0%. Personalities change style more than strength. Defensive is the only one
noticeably below Balanced (about 1.8 standard errors). It is used for encounters where the opponent should feel
solid rather than sharp, and difficulty is set by the level, not the personality.

## Design review (MD.txt 109)

| Question | Answer from the data |
|---|---|
| Repeatable dominant strategy? | None found: no ability decides games alone; Shadow Step is the strongest and cheapest, and pawn-bound. |
| Easy infinite loops? | No: no game approached the 600-ply limit; fivefold repetition ended 0 to 4% of the games of an experiment. |
| Unreasonably defensive positions? | Draw rates stayed between 1% and 10.5% per experiment; mates dominate. |
| Excessively long matches? | Average 95 to 137 plies by experiment; 4 of 3020 games passed 400 plies. |
| Useless spells? | Castle Swap and Royal Command are rare and situational, not useless (see section 3). |
| Forced wins through rule exploits? | None observed in the games; the rules are covered by the host tests (every interaction of MD.txt 97). |
| AI misunderstanding mana? | It spends mana every game and keeps a reserve for Royal Escape; it may hold more than ideal (5.5 of 6). |
| AI wasting powerful abilities? | Royal Escape is used in about half of player-games, always while in check (a rule); expensive spells are not spammed. |

## Fairness (MD.txt 110)

The computer obeys the same rules as the player: same move generator, same mana, same limits. There are no hidden
random outcomes, no secret bonuses, no paid upgrades, no permanent stat increases and no scripted victories.
Scenario modifiers (a mana cap, sealed abilities) are announced in each encounter's briefing and apply as stated.

## Adjustments

* **Rules: none.** The ability costs (2, 3, 3, 4, 5, 5), the mana cap (6), the regeneration (1 per turn) and Royal
  Escape's single use are the design values. Two changes were considered:
  * Lowering Royal Command to 4, to make it more common: cheaper costs raised its use from 33 to 59 casts, still
    rare, while adding draws and length.
  * A cap of 8: more expensive spells, but longer games.

  Neither addressed a defect. Changing costs would also mean re-proving, and possibly redesigning, the campaign
  and challenge puzzles, which were built for these values.
* **AI**: the study is run on the current AI. Before it, the lower levels' controlled-imperfection pass was rebuilt
  (it could stall the console; see AI_DESIGN.md). That changed which near-best moves the Apprentice and the
  Knight pick, and all logs come from the rebuilt version.

## Limitations

* The games are computer against computer, mostly at Knight level. People plan differently and may favor the rare
  abilities more than the AI does.
* Sample sizes of 100 to 200 games per experiment detect only effects larger than about 8 to 10 points of score.
* Material after a cast is measured over four plies; longer-term effects of repositioning are not captured.

## Data

Generated from `docs/balance_logs/` by `tools/balance/summarize.py`. "Config A" is the configuration given as
"white" on the command line (with `--alternate` it plays White in odd games and Black in even ones); its score
counts a draw as half a point.

Games simulated in total: 3020 (23 experiments)

| Experiment | Games | White / Black / Draw | Config A / B | Plies | Checkmates | Draws |
|---|---|---|---|---|---|---|
| e1_arcane_knight | 200 | 100 / 85 / 15 | 91 / 94 | 125.4 | 92.5% (±1.9) | 7.5% (±1.9) |
| e2_classic_knight | 200 | 112 / 74 / 14 | 92 / 94 | 108.7 | 93.0% (±1.8) | 7.0% (±1.8) |
| e3_magic_vs_none | 200 | 92 / 96 / 12 | 135 / 53 | 121.1 | 94.0% (±1.7) | 6.0% (±1.7) |
| e4_without_castle_swap | 120 | 59 / 55 / 6 | 57 / 57 | 126.7 | 95.0% (±2.0) | 5.0% (±2.0) |
| e4_without_ghost_walk | 120 | 57 / 55 / 8 | 53 / 59 | 130.7 | 93.3% (±2.3) | 6.7% (±2.3) |
| e4_without_phase_shift | 120 | 62 / 49 / 9 | 58 / 53 | 123.5 | 92.5% (±2.4) | 7.5% (±2.4) |
| e4_without_royal_cmd | 120 | 58 / 54 / 8 | 55 / 57 | 125.6 | 93.3% (±2.3) | 6.7% (±2.3) |
| e4_without_royal_escape | 120 | 63 / 45 / 12 | 59 / 49 | 121.9 | 90.0% (±2.7) | 10.0% (±2.7) |
| e4_without_shadow_step | 120 | 63 / 51 / 6 | 74 / 40 | 118.8 | 95.0% (±2.0) | 5.0% (±2.0) |
| e5_costs_cheaper | 200 | 86 / 93 / 21 | 96 / 83 | 131.4 | 89.5% (±2.2) | 10.5% (±2.2) |
| e5_costs_pricier | 200 | 99 / 85 / 16 | 83 / 101 | 126.7 | 92.0% (±1.9) | 8.0% (±1.9) |
| e6_mana_cap_4 | 200 | 106 / 84 / 10 | 92 / 98 | 115.6 | 95.0% (±1.5) | 5.0% (±1.5) |
| e6_mana_cap_8 | 200 | 100 / 84 / 16 | 94 / 90 | 132.2 | 92.0% (±1.9) | 8.0% (±1.9) |
| e7_grandmaster_mirror | 40 | 14 / 22 / 4 | 18 / 18 | 137.1 | 90.0% (±4.7) | 10.0% (±4.7) |
| e7_grandmaster_vs_sorcerer | 60 | 33 / 25 / 2 | 38 / 20 | 126.0 | 96.7% (±2.3) | 3.3% (±2.3) |
| e7_knight_vs_apprentice | 100 | 52 / 47 / 1 | 96 / 3 | 94.5 | 99.0% (±1.0) | 1.0% (±1.0) |
| e7_sorcerer_vs_knight | 100 | 48 / 49 / 3 | 83 / 14 | 123.3 | 97.0% (±1.7) | 3.0% (±1.7) |
| e8_aggressive_vs_balanced | 100 | 57 / 38 / 5 | 47 / 48 | 130.8 | 95.0% (±2.2) | 5.0% (±2.2) |
| e8_defensive_vs_balanced | 100 | 52 / 42 / 6 | 38 / 56 | 125.6 | 94.0% (±2.4) | 6.0% (±2.4) |
| e8_master_vs_balanced | 100 | 58 / 34 / 8 | 51 / 41 | 134.1 | 92.0% (±2.7) | 8.0% (±2.7) |
| e8_positional_vs_balanced | 100 | 61 / 35 / 4 | 42 / 54 | 126.5 | 96.0% (±2.0) | 4.0% (±2.0) |
| e8_tactical_vs_balanced | 100 | 48 / 45 / 7 | 46 / 47 | 130.2 | 93.0% (±2.6) | 7.0% (±2.6) |
| e8_trickster_vs_balanced | 100 | 54 / 40 / 6 | 42 / 52 | 134.3 | 94.0% (±2.4) | 6.0% (±2.4) |

| Experiment | Config A score | ss | ps | gw | cs | rc | re | Stored mana |
|---|---|---|---|---|---|---|---|---|
| e1_arcane_knight | 49.2% | 1383 | 616 | 424 | 25 | 33 | 221 | 5.48 |
| e2_classic_knight | 49.5% | 0 | 0 | 0 | 0 | 0 | 0 | 0.00 |
| e3_magic_vs_none | 70.5% | 660 | 342 | 197 | 18 | 21 | 72 | 5.69 |
| e4_without_castle_swap | 50.0% | 827 | 434 | 250 | 4 | 23 | 131 | 5.47 |
| e4_without_ghost_walk | 47.5% | 792 | 365 | 105 | 17 | 23 | 126 | 5.58 |
| e4_without_phase_shift | 52.1% | 934 | 225 | 251 | 17 | 21 | 122 | 5.51 |
| e4_without_royal_cmd | 49.2% | 877 | 405 | 257 | 14 | 16 | 132 | 5.45 |
| e4_without_royal_escape | 54.2% | 814 | 401 | 243 | 19 | 23 | 65 | 5.51 |
| e4_without_shadow_step | 64.2% | 402 | 382 | 245 | 12 | 35 | 132 | 5.50 |
| e5_costs_cheaper | 53.2% | 1545 | 949 | 525 | 21 | 59 | 238 | 5.68 |
| e5_costs_pricier | 45.5% | 1071 | 429 | 274 | 15 | 19 | 199 | 5.38 |
| e6_mana_cap_4 | 48.5% | 1204 | 479 | 437 | 9 | 0 | 0 | 3.77 |
| e6_mana_cap_8 | 51.0% | 1498 | 764 | 468 | 28 | 84 | 248 | 7.12 |
| e7_grandmaster_mirror | 50.0% | 390 | 218 | 94 | 8 | 16 | 40 | 5.35 |
| e7_grandmaster_vs_sorcerer | 65.0% | 494 | 240 | 146 | 8 | 16 | 70 | 5.40 |
| e7_knight_vs_apprentice | 96.5% | 447 | 247 | 167 | 22 | 17 | 91 | 5.43 |
| e7_sorcerer_vs_knight | 84.5% | 714 | 350 | 217 | 20 | 23 | 95 | 5.45 |
| e8_aggressive_vs_balanced | 49.5% | 657 | 343 | 194 | 7 | 29 | 106 | 5.50 |
| e8_defensive_vs_balanced | 41.0% | 622 | 285 | 210 | 9 | 17 | 111 | 5.50 |
| e8_master_vs_balanced | 55.0% | 725 | 325 | 197 | 13 | 18 | 108 | 5.51 |
| e8_positional_vs_balanced | 44.0% | 721 | 325 | 190 | 11 | 18 | 94 | 5.49 |
| e8_tactical_vs_balanced | 49.5% | 726 | 291 | 202 | 10 | 21 | 102 | 5.51 |
| e8_trickster_vs_balanced | 45.0% | 788 | 294 | 226 | 15 | 18 | 104 | 5.48 |

| Experiment | Material change 4 plies after a cast (cp): ss / ps / gw / cs / rc / re | Decisive leads: converted / drawn / reversed |
|---|---|---|
| e1_arcane_knight | -37 / -24 / -28 / -12 / -16 / -72 | 140 of 176 / 6 / 30 |
| e2_classic_knight | n/a / n/a / n/a / n/a / n/a / n/a | 145 of 174 / 7 / 22 |
| e3_magic_vs_none | -33 / -9 / -35 / -22 / +8 / -33 | 150 of 183 / 8 / 25 |
| e4_without_castle_swap | -35 / -34 / -31 / +25 / -85 / -77 | 88 of 106 / 2 / 16 |
| e4_without_ghost_walk | -31 / -10 / -36 / -25 / -10 / -70 | 86 of 112 / 7 / 19 |
| e4_without_phase_shift | -28 / -23 / -32 / +96 / -49 / -87 | 84 of 102 / 1 / 17 |
| e4_without_royal_cmd | -37 / -20 / -49 / +41 / +21 / -78 | 87 of 106 / 6 / 13 |
| e4_without_royal_escape | -34 / -21 / -48 / +12 / -64 / -123 | 84 of 103 / 3 / 16 |
| e4_without_shadow_step | -44 / -15 / -39 / +73 / -89 / -89 | 92 of 106 / 1 / 13 |
| e5_costs_cheaper | -40 / -18 / -36 / +39 / -49 / -96 | 148 of 181 / 10 / 23 |
| e5_costs_pricier | -36 / -26 / -44 / -15 / -52 / -80 | 146 of 180 / 8 / 26 |
| e6_mana_cap_4 | -36 / -13 / -45 / -34 / n/a / n/a | 155 of 180 / 7 / 18 |
| e6_mana_cap_8 | -32 / -23 / -42 / +0 / -35 / -68 | 152 of 184 / 8 / 24 |
| e7_grandmaster_mirror | -36 / -30 / -9 / +0 / -52 / -75 | 34 of 38 / 2 / 2 |
| e7_grandmaster_vs_sorcerer | -17 / -31 / -31 / -66 / -46 / -88 | 47 of 58 / 0 / 11 |
| e7_knight_vs_apprentice | -54 / -22 / -38 / -18 / -43 / -190 | 85 of 95 / 0 / 10 |
| e7_sorcerer_vs_knight | -27 / -8 / -29 / -138 / +26 / -112 | 79 of 94 / 3 / 12 |
| e8_aggressive_vs_balanced | -39 / -17 / -47 / -1 / -15 / -65 | 76 of 90 / 4 / 10 |
| e8_defensive_vs_balanced | -40 / -14 / -47 / -17 / -25 / -108 | 79 of 92 / 2 / 11 |
| e8_master_vs_balanced | -45 / -3 / -38 / +8 / -64 / -34 | 72 of 93 / 4 / 17 |
| e8_positional_vs_balanced | -28 / -9 / -34 / +4 / -42 / -55 | 78 of 91 / 3 / 10 |
| e8_tactical_vs_balanced | -39 / -19 / -37 / +30 / -44 / -79 | 74 of 93 / 6 / 13 |
| e8_trickster_vs_balanced | -29 / -22 / -30 / +25 / +10 / -113 | 74 of 94 / 3 / 17 |

Commands:

- `e1_arcane_knight`: `simulate --games 200 --white 1 --black 1 --alternate`
- `e2_classic_knight`: `simulate --games 200 --white 1 --black 1 --alternate --classic`
- `e3_magic_vs_none`: `simulate --games 200 --white 1 --black 1 --alternate --abilities-b 0`
- `e4_without_castle_swap`: `simulate --games 120 --white 1 --black 1 --alternate --abilities-b 55`
- `e4_without_ghost_walk`: `simulate --games 120 --white 1 --black 1 --alternate --abilities-b 59`
- `e4_without_phase_shift`: `simulate --games 120 --white 1 --black 1 --alternate --abilities-b 61`
- `e4_without_royal_cmd`: `simulate --games 120 --white 1 --black 1 --alternate --abilities-b 47`
- `e4_without_royal_escape`: `simulate --games 120 --white 1 --black 1 --alternate --abilities-b 31`
- `e4_without_shadow_step`: `simulate --games 120 --white 1 --black 1 --alternate --abilities-b 62`
- `e5_costs_cheaper`: `simulate --games 200 --white 1 --black 1 --alternate --costs 1,2,2,3,4,4`
- `e5_costs_pricier`: `simulate --games 200 --white 1 --black 1 --alternate --costs 3,4,4,5,6,6`
- `e6_mana_cap_4`: `simulate --games 200 --white 1 --black 1 --alternate --max-mana 4`
- `e6_mana_cap_8`: `simulate --games 200 --white 1 --black 1 --alternate --max-mana 8`
- `e7_grandmaster_mirror`: `simulate --games 40 --white 3 --black 3 --alternate`
- `e7_grandmaster_vs_sorcerer`: `simulate --games 60 --white 3 --black 2 --alternate`
- `e7_knight_vs_apprentice`: `simulate --games 100 --white 1 --black 0 --alternate`
- `e7_sorcerer_vs_knight`: `simulate --games 100 --white 2 --black 1 --alternate`
- `e8_aggressive_vs_balanced`: `simulate --games 100 --white 1 --black 1 --alternate --personality-white 1`
- `e8_defensive_vs_balanced`: `simulate --games 100 --white 1 --black 1 --alternate --personality-white 2`
- `e8_master_vs_balanced`: `simulate --games 100 --white 1 --black 1 --alternate --personality-white 6`
- `e8_positional_vs_balanced`: `simulate --games 100 --white 1 --black 1 --alternate --personality-white 4`
- `e8_tactical_vs_balanced`: `simulate --games 100 --white 1 --black 1 --alternate --personality-white 3`
- `e8_trickster_vs_balanced`: `simulate --games 100 --white 1 --black 1 --alternate --personality-white 5`
