#include <stdio.h>
#include <stdbool.h>
#include <assert.h>

// TODO: time util, don't be platform specific mybe just use cstdlib? think about this
// TODO: glew still used, write our own util ok?

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

#define RGFW_DEBUG
#define RGFW_IMPLEMENTATION
#define RGFW_OPENGL
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

int main2() {
  RGFW_window* win = RGFW_createWindow("a window", 0, 0, 800, 600, RGFW_windowCenter | RGFW_windowNoResize | RGFW_windowOpenGL);
  RGFW_window_swapInterval_OpenGL(win, 1);
  s32 mouseX, mouseY;
  while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
    RGFW_event event;
    while (RGFW_window_checkEvent(win, &event)) {

      if (event.type == RGFW_mouseButtonPressed && event.button.value == RGFW_mouseLeft) {
        RGFW_window_getMouse(win, &mouseX, &mouseY);
        printf("You clicked at x: %d, y: %d\n", mouseX, mouseY);
      } else if (event.key.value == RGFW_keyEscape) {
          printf("EsFuckingCape?!\n");
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


GLuint load_shader(const char *shaderSource, GLenum type) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &shaderSource, NULL);
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, NULL, infoLog);
    printf("Shader Compile Error: %s\n", infoLog);
  }

  return shader;
}

typedef struct {
  u8 *data;
  u64 width;
  u64 height;
} Platform_Image_Data;
Platform_Image_Data platform_load_image_bytes_as_rgba(const char *filepath);


Platform_Image_Data platform_load_image_bytes_as_rgba(const char *filepath) {
  Platform_Image_Data img_data = {};

  stbi_set_flip_vertically_on_load(true);
  int width, height, nrChannels;
  u8 *px_data = stbi_load(filepath, &width, &height, &nrChannels, STBI_rgb_alpha);

  img_data.width = width;
  img_data.height = height;
  img_data.data = px_data;

  return img_data;
}

int main(void) {
  Game_Api game_api = {};
  // Dummy Game_Api initialization
  // @TODO: Try to do the game reloading thingy
  game_api.init = game_init;
  game_api.update = game_update;
  game_api.render = game_render;
  game_api.shutdown = game_shutdown;
  //-------------------------------------------

  RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();
  hints->major = 4;
  hints->minor = 6;
  RGFW_setGlobalHints_OpenGL(hints);

  RGFW_window* win = RGFW_createWindow("window", 0, 0, 800, 600, RGFW_windowCenter | RGFW_windowNoResize | RGFW_windowHide);
  RGFW_window_createContext_OpenGL(win, hints);
  RGFW_window_show(win);
  RGFW_window_setExitKey(win, RGFW_keyEscape);
  const GLubyte *version = glGetString(GL_VERSION);
  printf("OpenGL Version: %s\n", version);

  // 0. Initialization
  Ogl_Tex g_backbuffer = {};
  Game_State gs = {};
  gs.persistent_arena = arena_make(GB(1));
  gs.frame_arena = arena_make(MB(256));
  gs.screen_dim = v2m(800, 600);

  glewInit();
  ogl_init(); // To create the bullshit empty VAO opengl side, nothing else
  gs.red = ogl_tex_make((u8[]){250,90,72,255}, 1,1, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT});
  Platform_Image_Data image = platform_load_image_bytes_as_rgba("data/microgue.png");
  assert(image.width > 0);
  assert(image.height > 0);
  assert(image.data != nullptr);
  gs.atlas = ogl_tex_make(image.data, image.width, image.height, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT, .wrap_t = OGL_TEX_WRAP_MODE_REPEAT});
  gs.atlas_sprites_per_dim = v2m(16,10);
  gs.font = font_util_load_default_atlas(gs.persistent_arena, 64, 1024, 1024);
  stbi_image_free(image.data);
  g_backbuffer = ogl_tex_make(nullptr,0,0,OGL_TEX_FORMAT_RGBA8U,(Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT});

  game_api.init(&gs);

  float dt = 1.0f/60.0f;


  while (RGFW_window_shouldClose(win) == RGFW_FALSE) {
    arena_clear(gs.frame_arena);
    RGFW_event event;
    while (RGFW_window_checkEvent(win, &event)) {
      // TBD
    }

    // HACK
    gs.time_sec+=dt/10.0;
    // -----

    input_process_events(&gs.input);
    arena_clear(gs.frame_arena);

    gs.pixels = arena_push(gs.frame_arena, sizeof(u32)*gs.screen_dim.x*gs.screen_dim.y);
    game_api.update(&gs, dt);
    game_api.render(&gs, dt);

    ogl_tex_update(&g_backbuffer, (u8*)gs.pixels, gs.screen_dim.x, gs.screen_dim.y, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT});
    R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_VIEWPORT, .r = gs.game_viewport };
    r2d_push_cmd(gs.frame_arena, &gs.cmd_list, cmd, 256);
    cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_SCISSOR, .r = gs.game_viewport };
    r2d_push_cmd(gs.frame_arena, &gs.cmd_list, cmd, 256);
    //cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(gs.game_viewport.w/2.0, gs.game_viewport.h/2.0), .origin = v2m(0,0), .zoom = gs.zoom, .rot_deg = 0} };
    cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(0,0), .origin = v2m(0,0), .zoom = 1.0, .rot_deg = 0} };
    r2d_push_cmd(gs.frame_arena, &gs.cmd_list, cmd, 256);
    R2D_Quad quad = (R2D_Quad) {
        .src_rect = rec(0,0,gs.screen_dim.x,gs.screen_dim.y),
        .dst_rect = rec(0,0,gs.screen_dim.x,gs.screen_dim.y),
        .c = col(1,1,1,0.5),
        .tex = g_backbuffer,
        .rot_deg = 0,
    };
    cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_ADD_QUAD, .q = quad};
    r2d_push_cmd(gs.frame_arena, &gs.cmd_list, cmd, 256);

    r2d_render_cmds(gs.frame_arena, &gs.cmd_list);

    RGFW_window_swapBuffers_OpenGL(win);
  }

  RGFW_window_close(win);
  return 0;
}
