#include "tile.h"

void set_tile_chunk_value_nocheck(Tile_Map *tm, Tile_Chunk *chunk, iv2 tile_coords, u32 tile_value) {
  assert(chunk);
  assert((tile_coords.x >= 0) && (tile_coords.x < tm->chunk_dim) &&
        (tile_coords.y >= 0) && (tile_coords.y < tm->chunk_dim));

  chunk->tiles[tile_coords.x + tm->chunk_dim*tile_coords.y] = tile_value;
}

void set_tile_value(Tile_Map *tm, Tile_Chunk *chunk, v2 test_point, u32 tile_value) {
  if (chunk) {
    iv2 tile_pos = (iv2){
      .x = (s32)test_point.x,
      .y = (s32)test_point.y,
    };

    if ((tile_pos.x >=0) && (tile_pos.x < tm->chunk_dim) && 
        (tile_pos.y >= 0) && (tile_pos.y < tm->chunk_dim)) {
      set_tile_chunk_value_nocheck(tm, chunk, tile_pos, tile_value);
    } 
  }
}


u32 get_tile_chunk_value_nocheck(Tile_Map *tm, Tile_Chunk *chunk, iv2 tile_coords) {
  assert(chunk);
  assert((tile_coords.x >= 0) && (tile_coords.x < tm->chunk_dim) &&
        (tile_coords.y >= 0) && (tile_coords.y < tm->chunk_dim));

  return chunk->tiles[tile_coords.x + tm->chunk_dim*tile_coords.y];
}

b32 get_tile_value(Tile_Map *tm, Tile_Chunk *chunk, v2 test_point) {
  u32 chunk_value = 0;
  if (chunk) {
    iv2 tile_pos = (iv2){
      .x = (s32)test_point.x,
      .y = (s32)test_point.y,
    };

    if ((tile_pos.x >=0) && (tile_pos.x < tm->chunk_dim) && 
        (tile_pos.y >= 0) && (tile_pos.y < tm->chunk_dim)) {
      chunk_value = get_tile_chunk_value_nocheck(tm, chunk, tile_pos);
    } 
  }
  return chunk_value;
}

b32 is_tile_chunk_tile_empty(Tile_Map *tm, Tile_Chunk *chunk, v2 test_point) {
  b32 empty = (get_tile_value(tm, chunk, test_point) == 0);
  return empty;
}

Tile_Chunk *get_tile_chunk(Tile_Map *tm, iv2 chunk_coords) {
  Tile_Chunk *chunk= nullptr;
  if ((chunk_coords.x >= 0) && (chunk_coords.x < tm->tile_chunk_count.x) &&
        (chunk_coords.y >= 0) && (chunk_coords.x < tm->tile_chunk_count.y)) {
    chunk = &tm->chunks[chunk_coords.x + tm->chunk_dim * chunk_coords.y];
  }
  return chunk;
}

Tile_Map_Position canonicalize_position(Tile_Map *tm, Tile_Map_Position can_pos) {
  Tile_Map_Position p = can_pos;

  // For x-axis
  f32 tile_rel_x = can_pos.tile_rel_coords.x;
  s32 tile_offset_x = round_f32((f32)tile_rel_x / tm->tile_dim_meters.x);
  p.abs_tile_coords.x += tile_offset_x;
  p.tile_rel_coords.x = tile_rel_x - tm->tile_dim_meters.x * tile_offset_x;

  // For y-axis
  f32 tile_rel_y = can_pos.tile_rel_coords.y;
  s32 tile_offset_y = round_f32((f32)tile_rel_y / tm->tile_dim_meters.y);
  p.abs_tile_coords.y += tile_offset_y;
  p.tile_rel_coords.y = tile_rel_y - tm->tile_dim_meters.y * tile_offset_y;

  return p;
}

Tile_Chunk_Position get_chunk_pos(Tile_Map *tm, iv2 abs_tile_coords) {
  Tile_Chunk_Position chunk_pos = {
    .chunk_coords = iv2m(abs_tile_coords.x >> tm->chunk_shift, abs_tile_coords.y >> tm->chunk_shift),
    .chunk_rel = iv2m(abs_tile_coords.x & tm->chunk_mask, abs_tile_coords.y & tm->chunk_mask),
  };

  return chunk_pos;
}

b32 is_tile_map_point_empty(Tile_Map *tm, Tile_Map_Position pos) {
  b32 empty = false;

  Tile_Map_Position cpos = canonicalize_position(tm, pos);
  Tile_Chunk_Position chunk_pos = get_chunk_pos(tm, iv2m(cpos.abs_tile_coords.x, cpos.abs_tile_coords.y));
  Tile_Chunk *chunk = get_tile_chunk(tm, chunk_pos.chunk_coords);
  empty = is_tile_chunk_tile_empty(tm, chunk, v2m(cpos.abs_tile_coords.x, cpos.abs_tile_coords.y));

  return empty;
}


