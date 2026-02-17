#include "game.h"
#include "gui/gui.h"

// TODO: Move to a better 2D renderer that can do arbitrary polygons not just AABBS rotated?
// TODO: lookup a good fzf pipeline to be able to search
// TODO: Maybe this should be just a test-bed and have repos reference this?? idk, I dont want many assets inside this repo maybe
// TODO: make-prg for building via build.sh and make an argument to only build, also run, and also just export the object files

void game_init(Game_State *gs) { }

void game_update(Game_State *gs, float dt) {
  static bool gui_initialized = false;
  if (!gui_initialized) {
    gui_context_init(gs->frame_arena, &gs->font);
    gui_initialized = true;
  }
  gs->game_viewport = rec(0,0,gs->screen_dim.x, gs->screen_dim.y);
}


void game_render(Game_State *gs, float dt) {
  // Push viewport, scissor and camera (we will not change these the whole frame except in UI pass)
  R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_VIEWPORT, .r = gs->game_viewport };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_SCISSOR, .r = gs->game_viewport };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  //cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(gs->game_viewport.w/2.0, gs->game_viewport.h/2.0), .origin = v2m(0,0), .zoom = gs->zoom, .rot_deg = 0} };
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(0,0), .origin = v2m(0,0), .zoom = 1.0, .rot_deg = 0} };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);


  f32 scale_factor = mod_f32(gs->time_sec, 1.0);
  scale_factor = ease_in_quad(scale_factor);

  v2 screen_mp = v2_multf(gs->screen_dim, 0.5);
  f32 hero_w = 300;
  hero_w = hero_w * scale_factor;


  R2D_Quad quad = (R2D_Quad) {
      .src_rect = rec(16*4,0,8,8),
      .dst_rect = rec(screen_mp.x - hero_w*0.5, screen_mp.y - hero_w*0.5, hero_w, hero_w),
      .c = col(1,1,1,1),
      .tex = gs->atlas,
      .rot_deg = 0,
  };
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_ADD_QUAD, .q = quad};
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);


  // ..
  // ..
  // In the end, perform a UI pass (TBA)
  // Right now: Print debug hero info stuff
  static u32 squish_count = 0;
  if (input_mkey_pressed(&gs->input, INPUT_MOUSE_RMB)) {
    squish_count+=1;
  }
  gui_frame_begin(gs->screen_dim, &gs->input, &gs->cmd_list, dt);

	gui_set_next_child_layout_axis(GUI_AXIS_X);
  gui_set_next_pref_height((Gui_Size){GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  gui_set_next_pref_width((Gui_Size){GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  Gui_Signal debug_pane = gui_pane(MAKE_STR("Main_Pane"));
  gui_push_parent(debug_pane.box);
  {
    gui_set_next_bg_color(col(0.1, 0.2, 0.4, 0.5));
    gui_set_next_text_alignment(GUI_TEXT_ALIGNMENT_CENTER);
    gui_set_next_pref_width((Gui_Size){GUI_SIZE_KIND_TEXT_CONTENT, 10.0, 1.0});
    gui_set_next_text_color(col(0.7, 0.8, 0.1, 0.9));
    buf hero_info = arena_sprintf(gs->frame_arena, "Press RMB for a nice squish");
    gui_label(hero_info);
  }
  gui_pop_parent();

	gui_set_next_child_layout_axis(GUI_AXIS_X);
  gui_set_next_pref_height((Gui_Size){GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  gui_set_next_pref_width((Gui_Size){GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  Gui_Signal debug_pane2 = gui_pane(MAKE_STR("Main_Pane2"));
  {
    gui_push_parent(debug_pane2.box);
    gui_set_next_bg_color(col(0.3, 0.1, 0.7, 0.5));
    gui_set_next_text_color(col(0.7, 0.8, 0.1, 0.9));
    gui_set_next_pref_width((Gui_Size){GUI_SIZE_KIND_TEXT_CONTENT, 10.0, 1.0});
    buf high_entity_info = arena_sprintf(gs->frame_arena, "squish count: %d", squish_count);
    gui_label(high_entity_info);

    /*
    buf choices[4] = {
      [0] = arena_sprintf(gs->frame_arena, "ease_in_quad"),
      [1] = arena_sprintf(gs->frame_arena, "ease_out_quad"),
      [2] = arena_sprintf(gs->frame_arena, "ease_in_qubic"),
      [3] = arena_sprintf(gs->frame_arena, "ease_out_qubic"),
    };
    gui_choice_box(buf_make("ease func", 6), choices, 4);
    */
  }
  gui_pop_parent();
  //--------------
  gui_frame_end();
}

void game_shutdown(Game_State *gs) {
  // This COULD be used for the persistent
  // GUI stuff outlined in game_update(!!)
}

