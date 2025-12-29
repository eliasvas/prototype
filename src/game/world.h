#ifndef WORLD_H__
#define WORLD_H__

#include "base/base_inc.h"

typedef enum {
  TILE_UNINITIALIZED = 0,
  TILE_EMPTY = 1,
  TILE_WALL = 2,
}Tile_Value;

typedef union iv2 {
    struct { s32 x,y; };
    struct { s32 u,v; };
    struct { s32 r,g; };
    s32 raw[2];
}iv2;
static iv2 iv2m(s32 x, s32 y)    { return (iv2){{x, y}}; }

typedef struct {
  iv2 abs_coords; // which tile
  v2 offset; // sub-tile position (fractional)
} World_Position;

typedef struct {
  iv2 chunk_coords; // which 256x256 chunk
  iv2 chunk_rel; // which tile inside the chunk e.g T(255,125)
} World_Chunk_Position;

typedef struct World_Chunk World_Chunk;
struct World_Chunk {
  // e.g chunkl(0,1,0)
  s32 chunk_x;
  s32 chunk_y;
  s32 chunk_z;

  // TODO: Make an entity block instead of this
  u32 *tiles;

  World_Chunk *next;
};

// Absolute Tile Position (24 | 8) -> (Chunk | Offset)
// meaning we can have 2^24 chunks and each chunk has 256 tiles
typedef struct {
  u32 chunk_shift;
  u32 chunk_mask;
  s32 chunk_dim; // How many tiles a chunk has - typically 256

  v2 tile_dim_meters; // How big each tile in the map is in meters 
  v2 tile_dim_px; // How big each tile in the map is in px
  
  World_Chunk *chunk_slots[1024];
} World;

World_Position canonicalize_position(World *tm, World_Position pos);
v2 get_world_fpos_in_meters(World *tm, World_Position pos);
World_Chunk *get_world_chunk_arena(World *tm, iv2 chunk_coords, Arena *arena);
World_Chunk_Position get_chunk_pos(World *tm, iv2 abs_coords);

#endif
