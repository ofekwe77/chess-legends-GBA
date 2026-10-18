#!/usr/bin/env bash
# CHESS LEGENDS: ARCANE WARS - the balance study of docs/BALANCE_REPORT.md (MD.txt 107-110).
#
# Runs every experiment with the host simulator and writes each log to docs/balance_logs/<name>.txt. The report
# quotes these logs; rerunning this script reproduces them exactly (the AI is deterministic for a given seed).
# Usage (from the project root, inside MSYS2): bash tools/balance/run_experiments.sh [--quick]
set -euo pipefail

cd "$(dirname "$0")/../.."
HOSTCXX=${HOSTCXX:-/ucrt64/bin/g++}
HOST_FLAGS="-std=c++23 -O2 -Wall -Wextra -Wshadow -Wpedantic -DCL_HOST=1 -Icore"
OUT=docs/balance_logs
mkdir -p build_host "$OUT"

"$HOSTCXX" $HOST_FLAGS -Itools/balance core/*.cpp tools/balance/*.cpp -static -o build_host/simulate.exe

# --quick divides the game counts by 10 (smoke run of the script itself).
SCALE=1
if [ "${1:-}" = "--quick" ]; then
    SCALE=10
fi

run() {
    local name=$1
    local games=$2
    shift 2
    local count=$(( games / SCALE ))
    [ "$count" -lt 2 ] && count=2
    echo "== $name ($count games): simulate --games $count $*"
    {
        echo "# simulate --games $count $*"
        ./build_host/simulate.exe --games "$count" --quiet "$@"
    } > "$OUT/$name.txt"
    grep -E "White wins|Config A|Average length|Decisive" "$OUT/$name.txt" | sed 's/^/   /'
}

# 1. Reference: Arcane and Classic Chess, Knight against Knight, colors alternated.
run e1_arcane_knight        200 --white 1 --black 1 --alternate
run e2_classic_knight       200 --white 1 --black 1 --alternate --classic

# 2. What magic is worth: all six abilities against none, and against all but one.
run e3_magic_vs_none        200 --white 1 --black 1 --alternate --abilities-b 0
run e4_without_shadow_step  120 --white 1 --black 1 --alternate --abilities-b 62
run e4_without_phase_shift  120 --white 1 --black 1 --alternate --abilities-b 61
run e4_without_ghost_walk   120 --white 1 --black 1 --alternate --abilities-b 59
run e4_without_castle_swap  120 --white 1 --black 1 --alternate --abilities-b 55
run e4_without_royal_cmd    120 --white 1 --black 1 --alternate --abilities-b 47
run e4_without_royal_escape 120 --white 1 --black 1 --alternate --abilities-b 31

# 3. Economy variants: every cost one lower / one higher; a smaller and a larger mana reserve.
run e5_costs_cheaper        200 --white 1 --black 1 --alternate --costs 1,2,2,3,4,4
run e5_costs_pricier        200 --white 1 --black 1 --alternate --costs 3,4,4,5,6,6
run e6_mana_cap_4           200 --white 1 --black 1 --alternate --max-mana 4
run e6_mana_cap_8           200 --white 1 --black 1 --alternate --max-mana 8

# 4. Difficulty ladder (the stronger level is configuration A).
run e7_knight_vs_apprentice 100 --white 1 --black 0 --alternate
run e7_sorcerer_vs_knight   100 --white 2 --black 1 --alternate
run e7_grandmaster_vs_sorcerer 60 --white 3 --black 2 --alternate
run e7_grandmaster_mirror    40 --white 3 --black 3 --alternate

# 5. Personalities against Balanced (Knight level).
run e8_aggressive_vs_balanced 100 --white 1 --black 1 --alternate --personality-white 1
run e8_defensive_vs_balanced  100 --white 1 --black 1 --alternate --personality-white 2
run e8_tactical_vs_balanced   100 --white 1 --black 1 --alternate --personality-white 3
run e8_positional_vs_balanced 100 --white 1 --black 1 --alternate --personality-white 4
run e8_trickster_vs_balanced  100 --white 1 --black 1 --alternate --personality-white 5
run e8_master_vs_balanced     100 --white 1 --black 1 --alternate --personality-white 6

echo "logs written to $OUT"
