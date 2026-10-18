#!/bin/bash
# Builds a minimal static libmgba (GBA core only, no frontends) for the headless test harness.
# Run inside MSYS2 (ucrt64 toolchain, cmake, ninja). Output: /opt/mgba-dev
set -e
export PATH="/ucrt64/bin:$PATH"
VER=0.10.5
SRC=/opt/src/mgba-$VER
OUT=/opt/mgba-dev

if [ ! -d "$SRC" ]; then
    mkdir -p /opt/src
    curl -sSL -o /opt/src/mgba-$VER.tar.gz https://github.com/mgba-emu/mgba/archive/refs/tags/$VER.tar.gz
    tar -C /opt/src -xzf /opt/src/mgba-$VER.tar.gz
fi

cmake -G Ninja -S "$SRC" -B /opt/src/mgba-build \
    -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5 -DCMAKE_INSTALL_PREFIX="$OUT" \
    -DLIBMGBA_ONLY=ON -DBUILD_STATIC=ON -DBUILD_SHARED=OFF \
    -DBUILD_QT=OFF -DBUILD_SDL=OFF -DBUILD_GL=OFF -DBUILD_GLES2=OFF -DBUILD_GLES3=OFF \
    -DUSE_EPOXY=OFF -DUSE_FFMPEG=OFF -DUSE_ZLIB=OFF -DUSE_MINIZIP=OFF -DUSE_PNG=OFF -DUSE_LIBZIP=OFF \
    -DUSE_SQLITE3=OFF -DUSE_ELF=OFF -DUSE_LUA=OFF -DUSE_LZMA=OFF -DUSE_DISCORD_RPC=OFF -DUSE_EDITLINE=OFF \
    -DUSE_GDB_STUB=OFF -DUSE_DEBUGGERS=OFF -DENABLE_SCRIPTING=OFF -DM_CORE_GB=OFF -DM_CORE_GBA=ON -DBUILD_LTO=OFF
cmake --build /opt/src/mgba-build
cmake --install /opt/src/mgba-build
echo "libmgba installed to $OUT"
