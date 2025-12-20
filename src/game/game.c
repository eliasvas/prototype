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


u32 get_tile_chunk_value_nocheck(World *world, Tile_Chunk *chunk, iv2 tile_coords) {
  assert(chunk);
  assert((tile_coords.x >= 0) && (tile_coords.x < world->chunk_dim) &&
        (tile_coords.y >= 0) && (tile_coords.y < world->chunk_dim));

  return chunk->tiles[tile_coords.x + world->chunk_dim*tile_coords.y];
}

// Implement get_tile_chunk_value where you pass the chunk and rel coords

b32 is_tile_chunk_tile_empty(World *world, Tile_Chunk *chunk, v2 test_point) {
  b32 empty = false;

  if (chunk) {
    iv2 tile_pos = (iv2){
      .x = (s32)test_point.x,
      .y = (s32)test_point.y,
    };

    if ((tile_pos.x >=0) && (tile_pos.x < world->chunk_dim) && 
        (tile_pos.y >= 0) && (tile_pos.y < world->chunk_dim)) {
      u32 chunk_value = get_tile_chunk_value_nocheck(world, chunk, tile_pos);
      empty = (chunk_value == 0);
    } 
  }

  return empty;
}

Tile_Chunk *get_tile_chunk(World *world, iv2 chunk_coords) {
  Tile_Chunk *chunk= nullptr;
  if ((chunk_coords.x >= 0) && (chunk_coords.x < world->tile_chunk_count.x) &&
        (chunk_coords.y >= 0) && (chunk_coords.x < world->tile_chunk_count.y)) {
    chunk = &world->chunks[chunk_coords.x + world->chunk_dim * chunk_coords.y];
  }
  return chunk;
}

world_pos canonicalize_position(World *world, world_pos can_pos) {
  world_pos p = can_pos;

  // For x-axis
  f32 tile_rel_x = can_pos.tile_rel_coords.x;
  s32 tile_offset_x = round_f32((f32)tile_rel_x / world->tile_dim_meters.x);
  p.abs_tile_coords.x += tile_offset_x;
  p.tile_rel_coords.x = tile_rel_x - world->tile_dim_meters.x * tile_offset_x;

  // For y-axis
  f32 tile_rel_y = can_pos.tile_rel_coords.y;
  s32 tile_offset_y = round_f32((f32)tile_rel_y / world->tile_dim_meters.y);
  p.abs_tile_coords.y += tile_offset_y;
  p.tile_rel_coords.y = tile_rel_y - world->tile_dim_meters.y * tile_offset_y;

  return p;
}

Tile_Chunk_Position get_chunk_pos(World *world, iv2 abs_tile_coords) {
  Tile_Chunk_Position chunk_pos = {
    .chunk_coords = iv2m(abs_tile_coords.x >> world->chunk_shift, abs_tile_coords.y >> world->chunk_shift),
    .chunk_rel = iv2m(abs_tile_coords.x & world->chunk_mask, abs_tile_coords.y & world->chunk_mask),
  };

  return chunk_pos;
}

b32 is_world_point_empty(World *world, world_pos pos) {
  b32 empty = false;

  world_pos cpos = canonicalize_position(world, pos);
  Tile_Chunk_Position chunk_pos = get_chunk_pos(world, iv2m(cpos.abs_tile_coords.x, cpos.abs_tile_coords.y));
  Tile_Chunk *chunk = get_tile_chunk(world, chunk_pos.chunk_coords);
  empty = is_tile_chunk_tile_empty(world, chunk, v2m(cpos.abs_tile_coords.x, cpos.abs_tile_coords.y));

  return empty;
}


void game_init(Game_State *gs) {
  // Initialize the gui
  gui_context_init(gs->frame_arena, &gs->font);

  // Initialize the world
  static u32 MegaTileChunk[256][256] = {
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
    {1,0,0,0,1,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,1,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,0,0,0,1,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1},
    {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1},
  };

  static Tile_Chunk tile_chunks[1][1] = {};

  u32 chunk_shift = 8;
  gs->world = (World) {
    .chunk_dim = 256,
    .chunk_shift = chunk_shift,
    .chunk_mask = ((1 << chunk_shift) - 1),
    .tile_chunk_count = iv2m(1,1),
    .tile_dim_px = v2m(8,8),
    .tile_dim_meters = v2m(1.5,1.5),
    .tile_count = iv2m(8,6),
    .chunks = (Tile_Chunk*)tile_chunks,
  };

  tile_chunks[0][0].tiles = (u32*)MegaTileChunk;


  // Initialize the player
  gs->pp = (world_pos){
    .abs_tile_coords = iv2m(2,2),
    .tile_rel_coords = v2m(0,0),
  };
  gs->player_dim_meters = v2_multf(gs->world.tile_dim_meters, 0.75);
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

  Tile_Chunk *chunk = get_tile_chunk(&gs->world, get_chunk_pos(&gs->world, gs->pp.abs_tile_coords).chunk_coords);

  // For Now at least chunk is always the [0][0] - lets asasert that
  assert(UINT_FROM_PTR(chunk) == UINT_FROM_PTR(gs->world.chunks));

  // TODO: Make it eactly half of screen?
  iv2 screen_offset = iv2m(-4,-3);
  iv2 hero_offset = get_chunk_pos(&gs->world, gs->pp.abs_tile_coords).chunk_rel;
  hero_offset.x += screen_offset.x;
  hero_offset.y += screen_offset.y;

  v2 m2p = v2_div(gs->world.tile_dim_px, gs->world.tile_dim_meters);

  // Draw The backgound
  for (u32 row = 0; row < 6; row+=1) {
    for (u32 col = 0; col < 8; col+=1) {
      if (is_tile_chunk_tile_empty(&gs->world, chunk, v2m(col+hero_offset.x , row+hero_offset.y))) {
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

  world_pos new_player_cpos = gs->pp;
  new_player_cpos.tile_rel_coords = new_player_pos;

  world_pos new_player_cpos_left = new_player_cpos;
  // we subtract pixels here!!
  new_player_cpos_left.tile_rel_coords.x -= gs->player_dim_meters.x/2;

  world_pos new_player_cpos_right = new_player_cpos;
  // we subtract pixels here!!
  new_player_cpos_right.tile_rel_coords.x += gs->player_dim_meters.x/2;


  if (is_world_point_empty(&gs->world, new_player_cpos) && 
      is_world_point_empty(&gs->world, new_player_cpos_left) &&
      is_world_point_empty(&gs->world, new_player_cpos_right)) {
    gs->pp = canonicalize_position(&gs->world, new_player_cpos);
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
