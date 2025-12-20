#ifndef _GAME_H__
#define _GAME_H__

#include "base/base_inc.h"
#include "core/core_inc.h"

typedef union iv2 {
    struct { s32 x,y; };
    struct { s32 u,v; };
    struct { s32 r,g; };
    s32 raw[2];
}iv2;
static iv2 iv2m(s32 x, s32 y)    { return (iv2){{x, y}}; }

// TODO: call tile_rel_coords offset?
typedef struct {
  iv2 abs_tile_coords; // which tile
  v2 tile_rel_coords; // sub-tile position (fractional)
} world_pos;

typedef struct {
  iv2 chunk_coords; // which 256x256 chunk (or rather which tile right?)
  iv2 chunk_rel; // which tile inside the chunk
} Tile_Chunk_Position;

typedef struct {
  u32 *tiles;
} Tile_Chunk;

// Absolute Tile Position (24 | 8) -> (Chunk | Offset)
// meaning we can have 2^24 chunks and each chunk has 256 tiles

typedef struct {
  s32 chunk_dim; // How many tiles a chunk has - typically 256
  iv2 tile_chunk_count;
  u32 chunk_shift;
  u32 chunk_mask;


  v2 tile_dim_meters; // How big each tile in the map is in meters 
  v2 tile_dim_px; // How big each tile in the map is in px
  iv2 tile_count; // how many tiles in each axis
  Tile_Chunk *chunks;

  v2 lower_left_corner;
} World;

typedef struct {
  s32 current_sine_sample; // not needed
  s32 sample_rate;
  s32 channel_count;

  // Game must fill these every frame (!!)
  f32 *samples;
  u64 samples_requested;
} Game_Audio_Output_Buffer;

typedef struct {
  Arena *persistent_arena; // For persistent allocations
  Arena *frame_arena; // For per-frame allocations
  rect game_viewport;
  
  // Interface between platform <-> game
  f32 time_sec;
  v2 screen_dim;
  Input input;
  R2D_Cmd_Chunk_List cmd_list;
  Game_Audio_Output_Buffer audio_out;

  // Game specific stuff
  f32 zoom;
  World world;
  v2 player_dim_meters; // player dimensions (in pixels or???)
  world_pos pp;
  
  // Loaded Asset resources (TODO: Asset system)
  Ogl_Tex atlas;
  Ogl_Tex red;
  Font_Info font;
  Effect fill_effect;
  Effect vortex_effect;

} Game_State;

void game_init(Game_State *gs);
void game_update(Game_State *gs, f32 dt);
void game_render(Game_State *gs, f32 dt);
void game_shutdown(Game_State *gs);

typedef void (*game_init_fn) (Game_State *gs);
typedef void (*game_update_fn) (Game_State *gs, f32 dt);
typedef void (*game_render_fn) (Game_State *gs, f32 dt);
typedef void (*game_shutdown_fn) (Game_State *gs);

typedef struct {
  game_init_fn init;
  game_update_fn update;
  game_render_fn render;
  game_shutdown_fn shutdown;

  void *lib;
  s64 last_modified;

  u64 api_version;
} Game_Api;

#endif
