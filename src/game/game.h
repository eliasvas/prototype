#ifndef _GAME_H__
#define _GAME_H__

#include "base/base_inc.h"
#include "core/core_inc.h"
#include "tile.h"

typedef struct {
  b32 exists;
  Tile_Map_Position p;
  v2 delta_p;
  v2 dim_meters; // width,height
} Entity;

typedef struct {
  Tile_Map *tm;
  iv2 screen_dim_in_tiles;
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
  //v2 player_dim_meters;
  //Tile_Map_Position pp;
  u64 entity_count;
  Entity entities[256];

  u64 player_entity_idx;
  u64 camera_following_entity_index;
  
  // Loaded Asset resources (TODO: Asset system)
  Ogl_Tex atlas;
  v2 atlas_tile_dim;
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
