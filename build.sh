#!/usr/bin/env bash
set -e

#TODO think about how assets are gonna work..
#TODO add flags: -nodefaultlibs -nostdlib -fno-builtin (SDL3 needs libc currently...)

PKGS="sdl3 glew"
CFLAGS="-Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wswitch-enum \
  -pedantic -fno-exceptions -fstack-protector -g -fsanitize=address"
CC="clang"

# -----------------------------
# Parse arguments
# Usage: ./build.sh gd=../my_game od=out clean=0
# -----------------------------
for arg in "$@"; do
  case $arg in
  gd=*)
    GAME_DIR="${arg#*=}"
    ;;
  od=*)
    OUTPUT_DIR="${arg#*=}"
    ;;
  clean=*)
    CLEAN="${arg#*=}"
    ;;
  *)
    echo "Unknown option: $arg"
    exit 1
    ;;
  esac
done

# -----------------------------
# Defaults (can be overridden)
# -----------------------------
ENGINE_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
GAME_DIR="$(realpath "$GAME_DIR")"
OUTPUT_DIR="$(realpath -m "$OUTPUT_DIR")"
CLEAN=1

echo "Game dir:   $GAME_DIR"
echo "Output dir: $OUTPUT_DIR"
echo

# -----------------------------
# Prepare output directory
# -----------------------------
if [ "$CLEAN" -eq 1 ]; then
  rm -rf "$OUTPUT_DIR"
fi

mkdir -p "$OUTPUT_DIR"

export ASAN_OPTIONS=detect_stack_use_after_return=1
export LSAN_OPTIONS=suppressions=lsan_ignore.txt

# -----------------------------
# Build the game shared library
# -----------------------------
echo "Building game..."

$CC $CFLAGS -O0 -std=gnu23 \
-Iext -I"$ENGINE_DIR"/src -fPIC -shared \
"$ENGINE_DIR"/src/core/input.c "$GAME_DIR"/*.c "$ENGINE_DIR"/src/gui/*.c \
-o "$OUTPUT_DIR/game.so"

if [ $? -eq 0 ]; then
  echo "Game Build succeeded. ✅"
else
  echo "Game Build failed. ❌"
  exit 1
fi

# -----------------------------
# Build the engine executable
# -----------------------------
echo "Building engine..."

pushd "$ENGINE_DIR" > /dev/null

$CC $CFLAGS -O0 -std=gnu23 \
$(pkg-config --cflags $PKGS) \
-L"$OUTPUT_DIR" \
-Iext -Isrc -I"$GAME_DIR" \
src/base/*.c src/core/*.c \
-o "$OUTPUT_DIR/prototype" \
$(pkg-config --libs $PKGS)

popd > /dev/null

if [ $? -eq 0 ]; then
  echo "Core Build succeeded. ✅"
else
  echo "Core Build failed. ❌"
  exit 1
fi
