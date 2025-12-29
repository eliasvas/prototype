#ifndef TILE_H__
#define TILE_H__

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

// TODO: call tile_rel_coords offset?
typedef struct {
  iv2 abs_tile_coords; // which tile
  v2 tile_rel_coords; // sub-tile position (fractional)
} Tile_Map_Position;

typedef struct {
  iv2 chunk_coords; // which 256x256 chunk
  iv2 chunk_rel; // which tile inside the chunk e.g T(255,125)
} Tile_Chunk_Position;

typedef struct Tile_Chunk Tile_Chunk;
struct Tile_Chunk {
  // e.g tile chunkl(0,1,0)
  s32 tile_chunk_x;
  s32 tile_chunk_y;
  s32 tile_chunk_z;

  u32 *tiles;
  Tile_Chunk *next;
};

// Absolute Tile Position (24 | 8) -> (Chunk | Offset)
// meaning we can have 2^24 chunks and each chunk has 256 tiles
typedef struct {
  s32 chunk_dim; // How many tiles a chunk has - typically 256
  iv2 tile_chunk_count;
  u32 chunk_shift;
  u32 chunk_mask;

  v2 tile_dim_meters; // How big each tile in the map is in meters 
  v2 tile_dim_px; // How big each tile in the map is in px
  
  Tile_Chunk *chunk_slots[1024];
} Tile_Map;


u32 get_tile_chunk_value_nocheck(Tile_Map *tm, Tile_Chunk *chunk, iv2 tile_coords);
b32 get_tile_value(Tile_Map *tm, Tile_Chunk *chunk, iv2 test_point);
void set_tile_chunk_value_nocheck(Tile_Map *tm, Tile_Chunk *chunk, iv2 tile_coords, u32 tile_value);
void set_tile_value(Tile_Map *tm, Tile_Chunk *chunk, iv2 test_point, u32 tile_value);
b32 is_tile_chunk_tile_empty(Tile_Map *tm, Tile_Chunk *chunk, iv2 test_point);
Tile_Chunk *get_tile_chunk(Tile_Map *tm, iv2 chunk_coords);
Tile_Map_Position canonicalize_position(Tile_Map *tm, Tile_Map_Position can_pos);
Tile_Chunk_Position get_chunk_pos(Tile_Map *tm, iv2 abs_tile_coords);
b32 is_tile_map_point_empty(Tile_Map *tm, Tile_Map_Position pos);
v2 get_tilemap_fpos_in_meters(Tile_Map *tm, Tile_Map_Position pos);
Tile_Chunk *get_tile_chunk_arena(Tile_Map *tm, iv2 chunk_coords, Arena *arena);

#endif
