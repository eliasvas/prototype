#!/usr/bin/env bash
set -e

#TODO think about how assets are gonna work..
#TODO add flags: -nodefaultlibs -nostdlib -fno-builtin (SDL3 needs libc currently...)

PKGS="sdl3 glew"
CFLAGS="-Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wswitch-enum \
  -pedantic -fno-exceptions -fstack-protector -g -fsanitize=address"
CC="clang"

source "build_common.sh"

export ASAN_OPTIONS=detect_stack_use_after_return=1
export LSAN_OPTIONS=suppressions=lsan_ignore.txt

# -----------------------------
# Build the game shared library
# -----------------------------
echo "Building game..."

CFLAGS="${CFLAGS:-} -std=gnu23"
DEBUG_FLAGS="-O0 -g"
RELEASE_FLAGS="-O2"

LDFLAGS="${LDFLAGS:-}"
INCLUDE_DIRS="-Iext -I$ENGINE_DIR/src"

$CC $CFLAGS $DEBUG_FLAGS $INCLUDE_DIRS -fPIC -shared \
"$ENGINE_DIR"/src/core/input.c \
"$GAME_DIR"/*.c \
"$ENGINE_DIR"/src/gui/*.c \
-o "$OUTPUT_DIR/game.so"

[ $? -eq 0 ] && echo "Game Build succeeded. ✅" || { echo "Game Build failed. ❌"; exit 1; }

# -----------------------------
# Build the engine executable
# -----------------------------
echo "Building engine..."

PKG_CFLAGS="$(pkg-config --cflags $PKGS)"
PKG_LIBS="$(pkg-config --libs $PKGS)"

pushd "$ENGINE_DIR" > /dev/null

$CC $CFLAGS $DEBUG_FLAGS $PKG_CFLAGS \
    -Iext -Isrc -I"$GAME_DIR" \
    src/core/*.c \
    -o "$OUTPUT_DIR/prototype" \
    -L"$OUTPUT_DIR" \
    $PKG_LIBS

popd > /dev/null

[ $? -eq 0 ] && echo "Core Build succeeded. ✅" || { echo "Core Build failed. ❌"; exit 1; }
