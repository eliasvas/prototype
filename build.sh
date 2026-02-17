PKGS="sdl3 glew"
CFLAGS="-Wall -Wextra -Wno-unused-function -Wno-unused-parameter -Wswitch-enum \
  -pedantic -fno-exceptions -fstack-protector -g -fsanitize=address"
CC="clang"
#TODO add: -nodefaultlibs -nostdlib -fno-builtin (SDL3 needs libc currently...)


#TODO: Make the GAME_DIR configurable e.g ./build.sh gd="../my_game_dir" 
GAME_DIR=./demo

#TODO: Make the OUTPUT_DIR configurable e.g ./build.sh gd="../my_game_dir" 
OUTPUT_DIR=build


rm -rf $OUTPUT_DIR
mkdir -p $OUTPUT_DIR


export ASAN_OPTIONS=detect_stack_use_after_return=1
export LSAN_OPTIONS=suppressions=lsan_ignore.txt

# Build the game dynamic library
$CC $CFLAGS -O0 -std=gnu23 -Iext -Isrc -fPIC -shared \
src/core/input.c src/$GAME_DIR/*.c src/gui/*.c -o $OUTPUT_DIR/game.so

if [ $? -eq 0 ]; then
    echo "✅ Game Build succeeded."
else
    echo "❌ Game Build failed."
fi

# Build the core (engine) executable
$CC $CFLAGS -O0 -std=gnu23 `pkg-config --cflags $PKGS` -L$OUTPUT_DIR \
-Iext -Isrc -Isrc/$GAME_DIR src/base/*.c src/core/*.c -o $OUTPUT_DIR/prototype \
`pkg-config --libs $PKGS`

if [ $? -eq 0 ]; then
    echo "✅ Core Build succeeded."
else
    echo "❌ Core Build failed."
fi
