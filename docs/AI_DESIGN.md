# The computer opponent

The computer plays by exactly the same rules as the player. It sees no hidden information and gets no extra mana,
no scripted victories and no random outcomes. Its strength comes from a search, and four difficulty levels
change how much it searches and how carefully it evaluates. All of it lives in `core/` (`cl_search*.cpp`,
`cl_eval.*`, `cl_ai.*`), so the console and the host simulator run the same code and, for a given seed, make the
same moves.

## 1. Search

`Searcher::search` (core/cl_search.cpp, nodes in core/cl_search_nodes.cpp) is a negamax alpha-beta search:

* **Iterative deepening** from depth 1 until the depth limit or the node budget is reached. The best move of the last
  completed iteration is played, unless the interrupted iteration found a different move that beat it with a
  full-window search.
* **Root move ordering** by the move-ordering heuristics, then by the previous iteration's scores.
* **Principal variation search**: the first move with a full window, the others with a zero window, re-searched
  when they fail high.
* **Transposition table**: 4096 entries on the console (32768 in the host tools), depth-preferred replacement that
  keeps fresher information for the same position. A 6-bit generation per entry replaces clearing the table
  before every search; entries of earlier searches behave exactly like empty ones, so results are identical to
  a cleared table.
* **Move ordering**: transposition move, captures by victim and attacker value (MVV/LVA), promotions, two killer
  moves per ply, history heuristic (aged by halving).
* **Null-move pruning** (depth 3 and more, not in check, not at principal-variation nodes, never twice in a row, only
  when the side to move has pieces other than pawns and its static score already reaches beta; reduction 2, or 3
  from depth 6).
* **Late-move reductions** of quiet moves after the fourth legal move (1 ply, 2 for late moves at depth 5 and more),
  never for checks, evasions or killers; re-searched at full depth when they beat alpha.
* **Check extension** (bounded by the ply limit) and **mate-distance pruning**.
* **Quiescence search**: captures and promotions with delta pruning; every evasion when in check (no stand-pat in
  check); at its first ply, a bounded number of magical actions that give check (a quiet repositioning can create
  a decisive check that pure capture search would miss).
* **Draws inside the search**: repetitions (game history and search path, with a hash filter), dead material,
  the seventy-five-move rule and the Arcane move limit are scored as draws; personalities shift the draw score
  (contempt).
* **Stack guard**: a node whose stack address falls near the bottom of the AI stack is evaluated statically.

### Magic in the search

Magical actions are ordinary members of the legal action set at every node: the move generator produces Shadow
Step, Phase Shift, Ghost Walk, Castle Swap, Royal Command and Royal Escape whenever the side to move can afford
them and their conditions hold. Mana is part of the position (and of its hash), so spending it, saving it and
regenerating it are seen by the search. Checkmate detection counts Royal Escape among the legal replies of a king in
check.

## 2. Evaluation

`evaluate()` (core/cl_eval.bn_iwram.cpp) scores a position in centipawns for the side to move, tapering every
term between a middlegame and an endgame value according to the material left:

| Term | Content |
|---|---|
| Material and placement | piece values (pawn 100, knight 320, bishop 330, rook 500, queen 900), piece-square tables, bishop pair, tempo |
| Pawn structure | doubled and isolated pawns |
| Passed pawns | bonus by rank (stronger in the endgame), king proximity to the promotion square |
| Mobility | safe squares per piece type |
| Piece activity | rooks on open and half-open files and on the seventh rank, outposts (better when pawn-protected) |
| King safety | pawn shelter against enemy pressure; king tropism weighted by attacker type with a non-linear attacker-count multiplier |
| Development | early queen adventures while the minor pieces are still at home; a castled king, or an uncastled king that has lost its rights |
| Mana | the mana each side will act with (the side not to move regenerates first), concave: the last point before the cap is worth least |
| Royal Escape | an unused charge with enough mana is a defensive resource, worth more when the king is in danger |
| Magic threats | vulnerability to the opponent's Phase Shift forks and checks (enemy knights one teleport away from a fork) |
| Endgames | mop-up (drive a lone king to the edge, bring the own king closer); drawish material scaled toward a draw |

The Apprentice uses the cheap part only (material, tables, bishop pair, tempo, simple king safety).

## 3. Difficulty levels

| Level | Node budget | Depth limit | Quiescence depth | Magical checks in quiescence | Evaluation | Noise | Imperfection |
|---|---|---|---|---|---|---|---|
| Apprentice | 350 | 2 | 4 | 2 | cheap | 40 cp | 22% of moves, within 90 cp |
| Knight | 1400 | 4 | 6 | 4 | full | 10 cp | 6% of moves, within 35 cp |
| Sorcerer | 3200 | 8 | 8 | 6 | full | none | none |
| Grandmaster | 6000 | 16 | 8 | 8 | full | none | none |

* **Budgets are node counts**, never time, so a level plays identically on the console and in the host tools. The
  campaign can scale a budget per encounter (`node_scale`).
* **Noise** is a deterministic pseudo-random term of the evaluation, seeded per match.
* **Controlled imperfection**: with the stated probability (seeded), the lower levels play another move whose quick
  score (a quiescence search) is within the margin of the quick score of the move they found. The pass uses
  zero-window tests, polls like the search and never spends more than the level's node budget again. A mate found
  by the search is never thrown away.

Measured strength (docs/BALANCE_REPORT.md): Knight beats Apprentice 96 to 3, Sorcerer beats Knight 83 to 14,
Grandmaster beats Sorcerer 38 to 20.

## 4. Personalities

Seven personalities scale the evaluation terms within bounded limits and set the contempt for draws: Balanced,
Aggressive (king attack, activity; avoids draws), Defensive (king safety, pawn structure), Tactical (more magical
checks in quiescence), Positional (structure and activity), Trickster (cheaper mana valuation, more magical checks),
Master (a little of everything). Bosses use the personality that fits their character. Personalities change style
more than strength; at Knight level Defensive and Positional score a little below Balanced (balance report, e8).

## 5. Opening book

From the standard starting position only, for the first five plies, the computer may choose among book replies
(about 17 positions, up to four candidates each). Each candidate carries style tags (balanced, aggressive,
defensive, positional, tactical), and a personality prefers the replies of its style. The choice is seeded by the
match seed, so openings vary between matches.

## 6. Performance safeguards on the console

* The search runs on its own 8 KB stack in external RAM; the stack guard keeps a safety margin.
* It polls at every node. Once 75% of the current frame has been used (measured from the start of the frame, so
  the interface's own work counts), it yields: the scene ends the frame, animates, reads input and redraws only
  what changed at the start of the next one. Measured: computer against computer misses 6 frames in 900.
* Before playing its move, the AI yields once more if the frame is already well used, so applying and presenting
  the move starts in a fresh frame.
* No clearing of large tables per search (generations instead), no 64-bit divisions, divisions in hot paths replaced
  by multiplications; position, move generation and evaluation run as ARM code from internal RAM.
* A pause (START) cancels the search; it restarts from scratch, deterministically, on resume.

### Measured speed

| Level | Middlegame move on the console (tests/emu/perf.txt) |
|---|---|
| Apprentice | 350 nodes, depth 1, 0.4 s |
| Knight | about 1150 nodes, depth 2, 2 s |
| Sorcerer | 3200 nodes, depth 2, 5 s |
| Grandmaster | 5173 nodes, depth 3, 7.9 s |

That is 550 to 850 nodes per second while the interface runs at 60 fps. The host tools search 1.1 to 2.3 million
nodes per second.

## 7. Known limitations

* The searches are shallow (depth 1 to 3 in the middlegame on the console), so long-term plans and deep tactics
  are beyond the lower levels and only partly within the higher ones.
* The computer rarely casts Castle Swap and Royal Command (25 and 33 times in 200 Knight-level games); taking
  either ability away from one side changed results by less than the measurement noise. They are situational
  tools, and the shallow searches seldom find positions where they are the best move.
* The time per move follows the node budget, not the difficulty of the position.
* The computer never resigns and offers no draws; it claims a draw by repetition or the fifty-move rule when it
  is worse.
* No endgame tablebases; the mop-up term and the search handle basic mates.
