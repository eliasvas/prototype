#include "game.h"
#include "gui/gui.h"


// TODO: screen->world and world->screen with camera, so we can spawn at edges of camera entities
#define TILE_W 8
#define TILE_H 8

//-------------------------------------
// draw.h ??
#define ATLAS_SPRITES_X 16
#define ATLAS_SPRITES_Y 10
// Currently y-up right handed coord system,
// Think about this, its probably what we want for world space transforms..
void game_push_atlas_rect(Game_State *gs, u32 atlas_idx, rect r) {
  u32 xidx = (u32)atlas_idx % ATLAS_SPRITES_X;
  u32 yidx = (u32)atlas_idx / ATLAS_SPRITES_X;

  // Calculate the y-up rect
  v2 llc = v2_divf(gs->world.lower_left_corner,gs->zoom);
  rect yup_rect = r;
  yup_rect.y = llc.y - r.h - r.y;

  R2D_Quad quad = (R2D_Quad) {
      .src_rect = rec(xidx*TILE_W,yidx*TILE_H,TILE_W,TILE_H),
      .dst_rect = yup_rect,
      .c = col(1,1,1,1),
      .tex = gs->atlas,
      .rot_deg = 0,
  };
  R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_ADD_QUAD, .q = quad};
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
}
//-----------------------------------------------


void game_init(Game_State *gs) {
  // Initialize the gui
  gui_context_init(gs->frame_arena, &gs->font);

  static Tile_Chunk tile_chunks[1][1] = {};

  u32 chunk_shift = 8;
  // Initialize the Tile_Map
  gs->world.tm = arena_push_struct(gs->persistent_arena, Tile_Map);
  *(gs->world.tm) = (Tile_Map) {
      .chunk_dim = 256,
      .chunk_shift = chunk_shift,
      .chunk_mask = ((1 << chunk_shift) - 1),
      .tile_chunk_count = iv2m(1,1),
      .tile_dim_px = v2m(8,8),
      .tile_dim_meters = v2m(1.5,1.5),
      .chunks = (Tile_Chunk*)tile_chunks,
  };
  tile_chunks[0][0].tiles = arena_push_array(gs->persistent_arena, u32, gs->world.tm->chunk_dim * gs->world.tm->chunk_dim);

  u32 screen_count_x = 8;
  u32 screen_count_y = 6;
  for (u32 screen_x = 0; screen_x < 4; screen_x+=1) {
    for (u32 screen_y = 0; screen_y < 4; screen_y+=1) {
      // Generate a map for screen (x,y)
      for (u32 tile_x = 0; tile_x < 8; tile_x +=1) {
        for (u32 tile_y = 0; tile_y < 6; tile_y +=1) {
          Tile_Chunk *chunk = get_tile_chunk(gs->world.tm, iv2m(0,0));
          set_tile_value(gs->world.tm, chunk, v2m(tile_x + screen_x*screen_count_x, tile_y + screen_y*screen_count_y), ((tile_x == 0 || tile_y == 0) && tile_x != 3 && tile_y != 4));
        }
      }
    }
  }


  // Initialize the player
  gs->pp = (Tile_Map_Position){
    .abs_tile_coords = iv2m(2,2),
    .tile_rel_coords = v2m(0,0),
  };
  gs->player_dim_meters = v2_multf(gs->world.tm->tile_dim_meters, 0.75);
  gs->zoom = 10.0;
}

void game_update(Game_State *gs, float dt) {
  gs->game_viewport = rec(0,0,gs->screen_dim.x, gs->screen_dim.y);
  gs->world.lower_left_corner = v2m(0, gs->screen_dim.y);

  //printf("hero tile coords: %d %d\n", gs->pp.tile_coords.x, gs->pp.tile_coords.y);
  //printf("hero tile rel coords: %f %f\n", gs->pp.tile_rel_coords.x, gs->pp.tile_rel_coords.y);


  // Audio test
  u32 sample_rate = 44100; 
  u32 num_seconds = 4;
  u32 num_channels = 2;

  u32 num_samples = sample_rate * num_channels * num_seconds;
  s32* wdata = arena_push_array(gs->frame_arena, s32, num_samples);

  s32 sample_value1 = 0;
  s32 sample_value2 = 0;
  for (u32 i = 0; i < num_samples; i+=2) {
    sample_value1 += 8000000;
    sample_value2 += 1200000;
    wdata[i] = sample_value1;
    wdata[i+1] = sample_value2;
  }

  assert(write_sample_wav_file("build/hello.wav", wdata, num_samples * sizeof(wdata[0]),
        num_channels, sample_rate, sizeof(wdata[0])*8)); 
  // ------------------

  // Another audio test -- real time (!!)
  // TODO: This is enough backend to make an audio mixer - do it
  u64 audio_samples_req = gs->audio_out.samples_requested;
  if (audio_samples_req > 0) {
    f32 volume_mod = 0.00;
    for (u32 sample_idx = 0; sample_idx < audio_samples_req; sample_idx+=1) {
      f32 freq = 400; // Hz
      f32 s = sin_f32(2 * PI * (freq/gs->audio_out.sample_rate) * gs->audio_out.current_sine_sample);
      gs->audio_out.samples[sample_idx] = s * volume_mod;
      gs->audio_out.current_sine_sample += 1;
    }
  }

}


void game_render(Game_State *gs, float dt) {
  // Push viewport, scissor and camera (we will not change these the whole frame except in UI pass)
  R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_VIEWPORT, .r = gs->game_viewport };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_SCISSOR, .r = gs->game_viewport };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  //cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(gs->game_viewport.w/2.0, gs->game_viewport.h/2.0), .origin = v2m(0,0), .zoom = gs->zoom, .rot_deg = 0} };
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(0,0), .origin = v2m(0,0), .zoom = gs->zoom, .rot_deg = 0} };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);

  Tile_Chunk *chunk = get_tile_chunk(gs->world.tm, get_chunk_pos(gs->world.tm, gs->pp.abs_tile_coords).chunk_coords);

  // For Now at least chunk is always the [0][0] - lets asasert that
  assert(UINT_FROM_PTR(chunk) == UINT_FROM_PTR(gs->world.tm->chunks));

  // TODO: Make it eactly half of screen?
  iv2 screen_offset = iv2m(-4,-3);
  iv2 hero_offset = get_chunk_pos(gs->world.tm, gs->pp.abs_tile_coords).chunk_rel;
  hero_offset.x += screen_offset.x;
  hero_offset.y += screen_offset.y;

  v2 m2p = v2_div(gs->world.tm->tile_dim_px, gs->world.tm->tile_dim_meters);

  // Draw The backgound
  for (u32 row = 0; row < 6; row+=1) {
    for (u32 col = 0; col < 8; col+=1) {
      if (is_tile_chunk_tile_empty(gs->world.tm, chunk, v2m(col+hero_offset.x , row+hero_offset.y))) {
        game_push_atlas_rect(gs, 69, rec(col*TILE_W - m2p.x*gs->pp.tile_rel_coords.x, row*TILE_H - m2p.y*gs->pp.tile_rel_coords.y,TILE_W,TILE_H));
      } else {
        game_push_atlas_rect(gs, 1, rec(col*TILE_W - m2p.x*gs->pp.tile_rel_coords.x, row*TILE_H - m2p.y*gs->pp.tile_rel_coords.y,TILE_W,TILE_H));
      }
    }
  }

  // Move + Draw the hero
  v2 new_player_pos = v2m(gs->pp.tile_rel_coords.x, gs->pp.tile_rel_coords.y);
  f32 hero_speed = 15.0;
  if (input_key_down(&gs->input, KEY_SCANCODE_UP))new_player_pos.y+=hero_speed*dt;
  if (input_key_down(&gs->input, KEY_SCANCODE_DOWN))new_player_pos.y-=hero_speed*dt;
  if (input_key_down(&gs->input, KEY_SCANCODE_LEFT))new_player_pos.x-=hero_speed*dt;
  if (input_key_down(&gs->input, KEY_SCANCODE_RIGHT))new_player_pos.x+=hero_speed*dt;

  Tile_Map_Position new_player_cpos = gs->pp;
  new_player_cpos.tile_rel_coords = new_player_pos;

  Tile_Map_Position new_player_cpos_left = new_player_cpos;
  // we subtract pixels here!!
  new_player_cpos_left.tile_rel_coords.x -= gs->player_dim_meters.x/2;

  Tile_Map_Position new_player_cpos_right = new_player_cpos;
  // we subtract pixels here!!
  new_player_cpos_right.tile_rel_coords.x += gs->player_dim_meters.x/2;

  if (is_tile_map_point_empty(gs->world.tm, new_player_cpos) && 
      is_tile_map_point_empty(gs->world.tm, new_player_cpos_left) &&
      is_tile_map_point_empty(gs->world.tm, new_player_cpos_right)) {
    gs->pp = canonicalize_position(gs->world.tm, new_player_cpos);
  }

  // to also draw the current tile, mainly for debugging
#if 1
  game_push_atlas_rect(gs, 16*6+2, 
      rec( -screen_offset.x*TILE_W  - m2p.x*gs->pp.tile_rel_coords.x,
        -screen_offset.y*TILE_H  - m2p.y*gs->pp.tile_rel_coords.y,
        TILE_W,
        TILE_H
      )
  );
#endif

  game_push_atlas_rect(gs, 9, 
      rec( -screen_offset.x*TILE_W + TILE_W/2 - m2p.x*gs->player_dim_meters.x/2,
        -screen_offset.y*TILE_H + TILE_H/2,
        m2p.x*gs->player_dim_meters.x,
        m2p.y*gs->player_dim_meters.y
      )
  );

  // ..
  // ..
  // In the end, perform a UI pass (TBA)
  gui_frame_begin(gs->screen_dim, &gs->input, &gs->cmd_list, dt);
  gui_set_next_bg_color(col(0.1, 0.2, 0.4, 0.5));
  Gui_Signal bs = gui_button(MAKE_STR("Exit"));
  if (bs.flags & GUI_SIGNAL_FLAG_LMB_PRESSED)printf("Exit pressed!\n");
  gui_frame_end();
}

void game_shutdown(Game_State *gs) { }
