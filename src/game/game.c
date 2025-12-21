#include "game.h"
#include "gui/gui.h"

// For rand
#include "stdlib.h"

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

  u32 chunk_shift = 4;
  // Initialize the Tile_Map
  gs->world.screen_dim_in_tiles = iv2m(9,7);
  gs->world.tm = arena_push_struct(gs->persistent_arena, Tile_Map);
  *(gs->world.tm) = (Tile_Map) {
      .chunk_dim = chunk_shift*chunk_shift,
      .chunk_shift = chunk_shift,
      .chunk_mask = ((1 << chunk_shift) - 1),
      .tile_chunk_count = iv2m(128,128),
      .tile_dim_px = v2m(8,8),
      .tile_dim_meters = v2m(1.5,1.5),
  };
  gs->world.tm->chunks = arena_push_array(gs->persistent_arena, Tile_Chunk, gs->world.tm->tile_chunk_count.x* gs->world.tm->tile_chunk_count.y);

  
  // Map generation (!!)
  b32 door_right = false;
  b32 door_left = false;
  b32 door_up = false;
  b32 door_down = false;
 
  iv2 screen_offset = iv2m(0,0);
  for (u32 screen_idx = 0; screen_idx < 40; screen_idx+=1) {
    // Generate a map for screen (x,y)
    u32 random_value = rand() % 2; 
    if ( random_value == 0) {
      door_right = true;
    } else {
      door_up = true;
    }

    for (s32 tile_y = 0; tile_y < gs->world.screen_dim_in_tiles.y; tile_y +=1) {
      for (s32 tile_x = 0; tile_x < gs->world.screen_dim_in_tiles.x; tile_x +=1) {
        Tile_Map_Position tile_pos = (Tile_Map_Position){
          .abs_tile_coords = iv2m(tile_x + screen_offset.x*gs->world.screen_dim_in_tiles.x, tile_y + screen_offset.y*gs->world.screen_dim_in_tiles.y),
          .tile_rel_coords = v2m(0,0),
        };
        tile_pos = canonicalize_position(gs->world.tm, tile_pos);

        Tile_Chunk *chunk = get_tile_chunk(gs->world.tm, get_chunk_pos(gs->world.tm, tile_pos.abs_tile_coords).chunk_coords);
        Tile_Value tval = TILE_EMPTY;

        if ((tile_x == 0) && !(door_left && (tile_y == gs->world.screen_dim_in_tiles.y/2))) {
            tval = TILE_WALL;
        }
        if ((tile_y == 0) && !(door_down && (tile_x == gs->world.screen_dim_in_tiles.x/2))) {
            tval = TILE_WALL;
        }

        if ((tile_x == gs->world.screen_dim_in_tiles.x-1) && !(door_right && (tile_y == gs->world.screen_dim_in_tiles.y/2))) {
            tval = TILE_WALL;
        }
        if ((tile_y == gs->world.screen_dim_in_tiles.y-1) && !(door_up && (tile_x == gs->world.screen_dim_in_tiles.x/2))) {
            tval = TILE_WALL;
        }

        //Tile_Chunk_Position cp = get_chunk_pos(gs->world.tm, tile_pos.abs_tile_coords);
        set_or_alloc_tile_value(gs->persistent_arena, gs->world.tm, chunk, get_chunk_pos(gs->world.tm, tile_pos.abs_tile_coords).chunk_rel, tval);
      }
    }
    door_left = door_right; // carry-over from prev room
    door_down = door_up; // carry-over from prev room
    door_right = false;
    door_up = false;

    if ( random_value == 0) {
      screen_offset.x+=1;
    } else {
      screen_offset.y+=1;
    }
  }


  // Initialize the player
  gs->pp = (Tile_Map_Position){
    .abs_tile_coords = iv2m(2,2),
    .tile_rel_coords = v2m(0,0),
  };
  gs->player_dim_meters = v2_multf(gs->world.tm->tile_dim_meters, 0.75);
  gs->zoom = 2.0;
}

void game_update(Game_State *gs, float dt) {
  gs->game_viewport = rec(0,0,gs->screen_dim.x, gs->screen_dim.y);
  gs->world.lower_left_corner = v2m(0, gs->screen_dim.y);

  // Perform the zoom
  if (input_key_pressed(&gs->input, KEY_SCANCODE_EQUALS)){
    gs->zoom*=2;
  }
  if (input_key_pressed(&gs->input, KEY_SCANCODE_MINUS)){
    gs->zoom/=2;
  }


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



  // A bit hacky.. you have to zoom in/ou with '-' and '='
  s32 screens_to_draw = 4;
  iv2 screen_offset = iv2m(gs->world.screen_dim_in_tiles.x*screens_to_draw/2, gs->world.screen_dim_in_tiles.y*screens_to_draw/2);

  v2 m2p = v2_div(gs->world.tm->tile_dim_px, gs->world.tm->tile_dim_meters);
  v2 po = v2m(m2p.y*gs->pp.tile_rel_coords.x, m2p.y*gs->pp.tile_rel_coords.y);

  // Draw The backgound
  for (s32 row = -screen_offset.y; row < screen_offset.y; row+=1) {
    for (s32 col = -screen_offset.x; col < screen_offset.x; col+=1) {
      Tile_Map_Position tile_pos = (Tile_Map_Position){
        .abs_tile_coords = iv2m(col+gs->pp.abs_tile_coords.x , row+gs->pp.abs_tile_coords.y),
        .tile_rel_coords = v2m(0,0),
      };
      tile_pos = canonicalize_position(gs->world.tm, tile_pos);

      Tile_Chunk_Position chunk_tile_pos = get_chunk_pos(gs->world.tm, tile_pos.abs_tile_coords);
 
      Tile_Chunk *chunk = get_tile_chunk(gs->world.tm, chunk_tile_pos.chunk_coords);
      Tile_Value value = get_tile_value(gs->world.tm, chunk, chunk_tile_pos.chunk_rel);

      if (value == TILE_UNINITIALIZED) { // uninitialized (nothing)
        game_push_atlas_rect(gs, 68, rec(screen_offset.x*TILE_W + col*TILE_W-po.x, screen_offset.y*TILE_H + row*TILE_H-po.y,TILE_W,TILE_H));
      } else if (value == TILE_EMPTY) { // empty (grass)
        game_push_atlas_rect(gs, 69, rec(screen_offset.x*TILE_W + col*TILE_W-po.x, screen_offset.y*TILE_H + row*TILE_H-po.y,TILE_W,TILE_H));
      } else { // wall
        game_push_atlas_rect(gs, 1, rec(screen_offset.x*TILE_W + col*TILE_W-po.x, screen_offset.y*TILE_H + row*TILE_H-po.y,TILE_W,TILE_H));
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
      is_tile_map_point_empty(gs->world.tm, new_player_cpos_right)) { gs->pp = canonicalize_position(gs->world.tm, new_player_cpos); Tile_Chunk_Position cp = get_chunk_pos(gs->world.tm, canonicalize_position(gs->world.tm, new_player_cpos_right).abs_tile_coords); printf("chunk %d %d\n", cp.chunk_coords.x, cp.chunk_coords.y);
  }

  // to also draw the current tile, mainly for debugging
#if 1
  game_push_atlas_rect(gs, 16*6+2, 
      rec( screen_offset.x*TILE_W - po.x, screen_offset.y*TILE_H - po.y,
        TILE_W,
        TILE_H
      )
  );
#endif

  game_push_atlas_rect(gs, 9, 
      rec( screen_offset.x*TILE_W + TILE_W/2 - m2p.x*gs->player_dim_meters.x/2,
        screen_offset.y*TILE_H + TILE_H/2,
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
