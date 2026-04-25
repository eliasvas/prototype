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
  return get_time_ns();
}

u64 platform_read_cpu_freq() {
  return get_nano_freq();
}

f64 platform_get_time() {
  return (f64)get_time_ns() / 1000000000.0;
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

Game_Api game_api = {};
RGFW_window* win;
f64 dt = 1.0/60.0;
u64 frame_start;
Ogl_Tex g_backbuffer = {};
Game_State gs = {};

void loop() {
    arena_clear(gs.frame_arena);
    RGFW_event event;
    while (RGFW_window_checkEvent(win, &event)) {
      Input_Event_Node input_event = {};
      switch(event.type) {
        case RGFW_windowResized:
          gs.screen_dim = v2m(event.update.w, event.update.h);
          input_event.evt = (Input_Event){
            .kind = INPUT_EVENT_KIND_RESIZE,
          };
          break;
        case RGFW_mousePosChanged:
          v2 new_mp = v2m(event.mouse.x, event.mouse.y);
          input_event.evt = (Input_Event){
            .data.mme = (Input_MouseMotion_Event) { .mouse_pos = new_mp },
            .kind = INPUT_EVENT_KIND_MOUSEMOTION,
          };
          break;
        case RGFW_keyPressed:
        case RGFW_keyReleased:
          if (event.key.repeat == 1) continue;
          //b32 pressed = event.key.state;
          s32 value = event.key.value;
          s32 scancode = 0;
          // @TODO: More keys mapped needed here please
          if (value >= 'A' && value <= 'Z') scancode = KEY_SCANCODE_A + (value-'A');
          else if (value >= '0' && value < '9') scancode = (value == '0') ? KEY_SCANCODE_0 : KEY_SCANCODE_1 + (value - '1');
          input_event.evt = (Input_Event){
            .data.ke = (Input_Keeb_Event) {
              .scancode = (Key_Scancode)scancode,
              .is_down = (event.type == RGFW_keyPressed),
            },
            .kind = INPUT_EVENT_KIND_KEEB,
          };
          break;
        case RGFW_mouseButtonPressed:
        case RGFW_mouseButtonReleased:
          b32 button_idx = event.button.value;
          if (button_idx >= INPUT_MOUSE_COUNT) continue; // no handling
          input_event.evt = (Input_Event){
            .data.me = (Input_Mouse_Event) {
              .button = (Input_Mouse_Button)(button_idx),
              .is_down = (event.type == RGFW_mouseButtonPressed),
            },
            .kind = INPUT_EVENT_KIND_MOUSE,
          };
          break;
        default:
          continue;
          break;
      }
      input_push_event(&gs.input, gs.frame_arena, &input_event.evt);
    }

    input_process_events(&gs.input);
    arena_clear(gs.frame_arena);


#if 0
    if (input_mkey_pressed(&gs.input, INPUT_MOUSE_MMB)) {
      printf("reload!!\n");
      dlclose(game_lib);
      game_lib = dlopen("./build/libgame.so", RTLD_LAZY | RTLD_DEEPBIND);
      assert(game_lib);
      game_api.init = (game_init_fn)dlsym(game_lib, "game_init");
      game_api.update = (game_update_fn)dlsym(game_lib, "game_update");
      game_api.render = (game_render_fn)dlsym(game_lib, "game_render");
      game_api.shutdown = (game_shutdown_fn)dlsym(game_lib, "game_shutdown");
    }
#endif

    gs.pixels = arena_push(gs.frame_arena, sizeof(u32)*gs.screen_dim.x*gs.screen_dim.y);
    game_api.update(&gs, dt);
    game_api.render(&gs, dt);

#if 0
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
#endif

    r2d_render_cmds(gs.frame_arena, &gs.cmd_list);

    RGFW_window_swapBuffers_OpenGL(win);

    // @TODO: cleanup a bit of funky logic also stop vsyncing always
    // Perform timings (Should this happen before swap window maybe?)
    u64 frame_end = platform_read_cpu_timer();
    dt = (frame_end - frame_start) / (f64)get_nano_freq();
    //printf("fps=%f begin=%f end=%f\n", 1.0/dt, (f32)frame_start, (f32)frame_end);
    frame_start = platform_read_cpu_timer();
    gs.time_sec = platform_get_time();

    input_end_frame(&gs.input);
}

int main(void) {

  profiler_begin();
  BRAND_SEED(1231231);

#if 1
  // Dummy Game_Api initialization
  // @TODO: Try to do the game reloading thingy
  game_api.init = game_init;
  game_api.update = game_update;
  game_api.render = game_render;
  game_api.shutdown = game_shutdown;
  //-------------------------------------------
#else
  void *game_lib = dlopen("./build/libgame.so", RTLD_LAZY | RTLD_DEEPBIND);
  assert(game_lib);
  game_api.init = (game_init_fn)dlsym(game_lib, "game_init");
  game_api.update = (game_update_fn)dlsym(game_lib, "game_update");
  game_api.render = (game_render_fn)dlsym(game_lib, "game_render");
  game_api.shutdown = (game_shutdown_fn)dlsym(game_lib, "game_shutdown");
#endif

  RGFW_glHints* hints = RGFW_getGlobalHints_OpenGL();

#if (WASM64 || WASM32)
  hints->major = 3;
  hints->minor = 0;
  hints->profile = RGFW_glES;
#else
  hints->major = 4;
  hints->minor = 3;
#endif
  RGFW_setGlobalHints_OpenGL(hints);

  win = RGFW_createWindow("window", 0, 0, 800, 600, RGFW_windowCenter | RGFW_windowNoResize | RGFW_windowHide);
  RGFW_window_createContext_OpenGL(win, hints);
  RGFW_window_show(win);
  RGFW_window_setExitKey(win, RGFW_keyEscape);
  RGFW_window_swapInterval_OpenGL(win, 1);
  const GLubyte *version = glGetString(GL_VERSION);
  printf("OpenGL Version: %s\n", version);

  // 0. Initialization
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

  dt = 1.0/60.0;
  frame_start = platform_get_time();

#if (WASM64 || WASM32)
  emscripten_set_main_loop(loop, 0, 1);
#else
  while (RGFW_window_shouldClose(win) == RGFW_FALSE) { loop(); }
#endif
  //profiler_end_and_print();
  //RGFW_window_close(win);
  return 0;
}
