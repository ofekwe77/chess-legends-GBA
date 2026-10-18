#!/bin/bash
# CHESS LEGENDS: ARCANE WARS command dispatcher. Runs inside MSYS2 from the project root (an ASCII-only path:
# see build.ps1). Usage: tools/cl.sh <command> [args]
#
#   all        regenerate assets, build the ROM, copy it to release/
#   assets     regenerate graphics / audio from assets_source/ generators
#   rom        build the ROM only
#   tests      build and run the host-side test suite (optional filters: group or test names)
#   sim        build and run the AI / balance simulator (tools/balance)
#   analyze    build and run the puzzle candidate analyzer (tools/validation/analyze.cpp)
#   validate   build and run the content validator (all encounters and challenges)
#   harness    build the headless mGBA test harness
#   emu        run an emulator scenario: tools/cl.sh emu --script tests/emu/x.txt [...]
#   emu-all    run every emulator scenario in tests/emu (fails if any fails)
#   verify     full release verification
#   clean      remove build outputs
set -e
cd "$(dirname "$0")/.."

if [ -f /etc/profile.d/devkit-env.sh ]; then
    source /etc/profile.d/devkit-env.sh
fi

JOBS=${JOBS:-8}
HOSTCXX=${HOSTCXX:-/ucrt64/bin/g++}
export PATH="/ucrt64/bin:$PATH"
HOST_FLAGS="-std=c++23 -O2 -g -Wall -Wextra -Wshadow -Wpedantic -DCL_HOST=1 -Icore"
if [ -z "$PYTHON" ]; then
    if command -v python3 >/dev/null 2>&1; then PYTHON=python3; else PYTHON="py -3.12"; fi
fi

cmd_assets() {
    $PYTHON assets_source/generate_all.py
}

cmd_rom() {
    make -j"$JOBS"
    if [ ! -f ChessLegends.gba ]; then
        echo "ERROR: ChessLegends.gba was not produced" >&2
        exit 1
    fi
    mkdir -p release
    cp ChessLegends.gba release/ChessLegends.gba
    ls -l release/ChessLegends.gba
}

cmd_tests() {
    mkdir -p build_host
    "$HOSTCXX" $HOST_FLAGS -Itests core/*.cpp tests/*.cpp -static -o build_host/run_tests.exe
    ./build_host/run_tests.exe "$@"
}

cmd_sim() {
    mkdir -p build_host
    "$HOSTCXX" $HOST_FLAGS -Itools/balance core/*.cpp tools/balance/*.cpp -static -o build_host/simulate.exe
    ./build_host/simulate.exe "$@"
}

cmd_analyze() {
    mkdir -p build_host
    "$HOSTCXX" $HOST_FLAGS core/*.cpp tools/validation/analyze.cpp -static -o build_host/analyze.exe
    ./build_host/analyze.exe "$@"
}

cmd_validate() {
    mkdir -p build_host
    "$HOSTCXX" $HOST_FLAGS core/*.cpp tools/validation/validate.cpp -static -o build_host/validate.exe
    ./build_host/validate.exe "$@"
    $PYTHON tools/check_glyphs.py
}

cmd_harness() {
    bash tools/harness/build_harness.sh
}

cmd_emu() {
    # Create the --out directory (screenshots, audio captures) before the run.
    local previous=""
    for arg in "$@"; do
        if [ "$previous" = "--out" ]; then
            mkdir -p "$arg"
        fi
        previous="$arg"
    done
    ./tools/harness/build/cl_harness.exe "$@"
}

cmd_emu_all() {
    # Every emulator scenario in tests/emu; non-zero exit if any fails.
    local failed=0 total=0
    for script in tests/emu/*.txt; do
        local name
        name=$(basename "$script" .txt)
        total=$((total + 1))
        mkdir -p "build_emu/$name"
        if ./tools/harness/build/cl_harness.exe --rom ChessLegends.gba --script "$script" --out "build_emu/$name" \
                --quiet > "build_emu/$name/log.txt" 2>&1; then
            echo "PASS $name"
        else
            echo "FAIL $name (see build_emu/$name/log.txt)"
            tail -3 "build_emu/$name/log.txt"
            failed=$((failed + 1))
        fi
    done
    echo "emulator scenarios: $((total - failed)) / $total passed"
    [ "$failed" -eq 0 ]
}

cmd_verify() {
    bash tools/verify_release.sh "$@"
}

cmd_clean() {
    make clean || true
    rm -rf build_host
}

case "${1:-all}" in
    all)     cmd_assets; cmd_rom ;;
    assets)  cmd_assets ;;
    rom)     cmd_rom ;;
    tests)   shift; cmd_tests "$@" ;;
    sim)     shift; cmd_sim "$@" ;;
    analyze) shift; cmd_analyze "$@" ;;
    validate) shift; cmd_validate "$@" ;;
    harness) cmd_harness ;;
    emu)     shift; cmd_emu "$@" ;;
    emu-all) cmd_emu_all ;;
    verify)  shift; cmd_verify "$@" ;;
    clean)   cmd_clean ;;
    *) echo "unknown command: $1" >&2; exit 2 ;;
esac
