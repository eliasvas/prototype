#ifndef WORLD_H__
#define WORLD_H__

//#include "game.h"
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
  iv2 chunk; // which chunk
  v2 offset; // sub-tile position (fractional)
} World_Position;

typedef struct World_Entity_Block World_Entity_Block; 
struct World_Entity_Block {
  u32 low_entity_indices[16];
  u32 count;
  u32 cap;

  World_Entity_Block *next;
  World_Entity_Block *prev;
};

typedef struct World_Chunk World_Chunk;
struct World_Chunk {
  // e.g chunkl(0,1,0)
  s32 chunk_x;
  s32 chunk_y;
  s32 chunk_z;

  // TODO: Make an entity block instead of this
  //u32 *tiles;

  World_Entity_Block *first;
  World_Entity_Block *last;

  World_Chunk *next;
};

// Absolute Tile Position (24 | 8) -> (Chunk | Offset)
// meaning we can have 2^24 chunks and each chunk has 256 tiles
typedef struct {
  f32 tiles_per_chunk; // maybe tiles per block
  v2 tile_dim_meters;
  v2 tile_dim_px;
  v2 chunk_dim_meters;
  
  World_Entity_Block *block_freelist;
  World_Chunk *chunk_slots[1024];
} World;

World_Position canonicalize_position(World *tm, World_Position pos);
v2 get_world_fpos_in_meters(World *tm, World_Position pos);
World_Chunk *get_world_chunk_arena(World *tm, iv2 chunk_coords, Arena *arena);
World_Chunk *get_world_chunk(World *w, iv2 chunk_coords);
World_Position chunk_pos_from_tile_pos(World *w, v2 abs_tile);
b32 are_in_same_chunk(World *w, World_Position a, World_Position b);

World_Position change_entity_location(Arena *arena, World *w, u32 low_entity_idx, World_Position *old, World_Position *new);
World_Position world_pos_nil(void);
b32 world_pos_is_valid(World_Position wp);

#endif
