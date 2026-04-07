# Good reference for emscripten building https://github.com/ocornut/imgui/blob/master/examples/example_sdl3_opengl3/Makefile.emscripten
# TODO: MUCH more customization needed, rn its GARBO

source "build_common.sh"

EMCC_DEBUG=1 emcc -v -o build/libgame.so $CFLAGS -Iext -Isrc -Isrc/demo -O1 \
-std=gnu23  "$GAME_DIR"/*.c src/gui/*.c -fPIC -shared \
-s USE_SDL=3 -s DISABLE_EXCEPTION_CATCHING=1 -s WASM=1 \
-s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=0 -s ASSERTIONS=1 \
-s FULL_ES3=1 -s GL_DEBUG=1

EMCC_DEBUG=1 emcc -v -o build/index.html $CFLAGS -Iext -Isrc -Isrc/demo -Lbuild -O1 \
-std=gnu23  src/core/*.c -lgame -Wl,-rpath='build/' \
-s USE_SDL=3 -s DISABLE_EXCEPTION_CATCHING=1 -s WASM=1 \
-s ALLOW_MEMORY_GROWTH=1 -s NO_EXIT_RUNTIME=0 -s ASSERTIONS=1 \
-s FULL_ES3=1 -s GL_DEBUG=1 --preload-file data
