#!/usr/bin/env bash
# CHESS LEGENDS: ARCANE WARS - full release verification (MD.txt 114-118).
#
# Builds release/ChessLegends.gba from the sources and verifies it end to end:
#   1. host tests (rules, Arcane abilities, AI, saves, campaign, progress)
#   2. full content validation (every puzzle proven by exhaustive search, every string fitted to the fonts)
#   3. warning-free ROM build, copied to release/
#   4. the ROM inspected as a GBA binary (header, logo, checksum, save type, every asset linked)
#   5. every emulator scenario run against the release ROM itself
#   6. the release ROM's size and SHA-256
# Usage (inside MSYS2, from anywhere): bash tools/verify_release.sh   (or: tools/cl.sh verify, build.ps1 verify)
set -euo pipefail
cd "$(dirname "$0")/.."

if [ -f /etc/profile.d/devkit-env.sh ]; then
    source /etc/profile.d/devkit-env.sh
fi

if [ -z "${PYTHON:-}" ]; then
    if command -v python3 >/dev/null 2>&1; then PYTHON=python3; else PYTHON="py -3.12"; fi
fi

mkdir -p build_host release
LOG=build_host/verify_release.txt
: > "$LOG"
step() { echo; echo "== $*" | tee -a "$LOG"; }

step "1. Host tests"
bash tools/cl.sh tests 2>&1 | tee -a "$LOG" | tail -1

step "2. Content validation (full proofs)"
bash tools/cl.sh validate 2>&1 | tee -a "$LOG" | tail -3

step "3. ROM build (warnings are failures)"
make -j"${JOBS:-8}" > build_host/release_build.txt 2>&1 || { cat build_host/release_build.txt; exit 1; }
if grep -E "warning:|error:" build_host/release_build.txt; then
    echo "the ROM build reported warnings or errors" | tee -a "$LOG"
    exit 1
fi
cp ChessLegends.gba release/ChessLegends.gba
echo "release/ChessLegends.gba built without warnings" | tee -a "$LOG"

step "4. ROM inspection"
$PYTHON tools/check_rom.py release/ChessLegends.gba --map build/ChessLegends.map --build build 2>&1 | tee -a "$LOG"

step "5. Emulator scenarios on the release ROM"
failed=0
total=0
for script in tests/emu/*.txt; do
    name=$(basename "$script" .txt)
    total=$((total + 1))
    mkdir -p "build_emu/release_$name"
    if ./tools/harness/build/cl_harness.exe --rom release/ChessLegends.gba --script "$script" \
            --out "build_emu/release_$name" --quiet > "build_emu/release_$name/log.txt" 2>&1; then
        echo "PASS $name" | tee -a "$LOG"
    else
        echo "FAIL $name (see build_emu/release_$name/log.txt)" | tee -a "$LOG"
        failed=$((failed + 1))
    fi
done
echo "emulator scenarios: $((total - failed)) / $total passed" | tee -a "$LOG"
[ "$failed" -eq 0 ]

step "6. Release artifact"
ls -l release/ChessLegends.gba | tee -a "$LOG"
sha256sum release/ChessLegends.gba | tee -a "$LOG"
echo
echo "release verified (log: $LOG)"
