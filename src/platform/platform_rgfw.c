#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

// TODO: time util, don't be platform specific mybe just use cstdlib? think about this

// TODO: WHAT about glew, where is that dependency? do we need it? HUH (sad)
//~ $ dnf search glew
//Updating and loading repositories:
//Repositories loaded.
//Matched fields: name (exact)
// glew.x86_64    The OpenGL Extension Wrangler Library

#define BRAND_IMPLEMENTATION
#define PROFILER_IMPLEMENTATION
#include "base/base_inc.h"

#define STB_SPRINTF_IMPLEMENTATION
#include <stb/stb_sprintf.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <GL/glew.h>
#define OGL_IMPLEMENTATION
#include "core/ogl.h"

#define INPUT_IMPLEMENTATION
#include "core/input.h"

#include "game.h"

#define RGFW_IMPLEMENTATION
#include <RGFW/RGFW.h>

u64 platform_read_cpu_timer() {
  //return SDL_GetPerformanceCounter();
  return 0;
}

u64 platform_read_cpu_freq() {
  //return SDL_GetPerformanceFrequency();
  return 0;
}

u64 platform_get_ticks_since_epoch() {
  /*
  SDL_Time ticks = {};
  SDL_GetCurrentTime(&ticks);
  return (u64)ticks;
  */
  return 0;
}

f64 platform_get_time() {
  return 0;
  //return (f64)SDL_GetTicksNS() / 1000000000.0;
}

int main() {
  RGFW_window* win = RGFW_createWindow("a window", 0, 0, 800, 600, RGFW_windowCenter | RGFW_windowNoResize);
  Game_State gs;

  s32 mouseX, mouseY;
  while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
    RGFW_event event;
    while (RGFW_window_checkEvent(win, &event)) {
      RGFW_window_getMouse(win, &mouseX, &mouseY);

      if (event.type == RGFW_mouseButtonPressed && event.button.value == RGFW_mouseLeft) {
        printf("You clicked at x: %d, y: %d\n", mouseX, mouseY);
      } else if (event.key.value == RGFW_keyEscape) {
          RGFW_window_setShouldClose(event.common.win, 1);
      }
    }

    if (RGFW_isMousePressed(RGFW_mouseRight)) {
      printf("The right mouse button was clicked at x: %d, y: %d\n", mouseX, mouseY);
    }
  }

  RGFW_window_close(win);
  return 0;
}

