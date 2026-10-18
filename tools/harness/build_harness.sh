#!/bin/bash
# Builds the headless mGBA test harness. Needs the static libmgba in /opt/mgba-dev (see build_libmgba.sh).
set -e
cd "$(dirname "$0")"
export PATH="/ucrt64/bin:$PATH"
MGBA=${MGBA_DEV:-/opt/mgba-dev}

if [ ! -f "$MGBA/libmgba.a" ]; then
    bash ./build_libmgba.sh
fi

mkdir -p build
gcc -std=c11 -O2 -Wall -Wextra -I"$MGBA/include" cl_harness.c "$MGBA/libmgba.a" \
    -lws2_32 -lshlwapi -lshell32 -lole32 -luuid -lwinmm -static -o build/cl_harness.exe
echo "built tools/harness/build/cl_harness.exe"
