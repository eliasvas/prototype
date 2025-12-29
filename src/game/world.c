#include "world.h"

#if 0
u32 get_world_chunk_value_nocheck(World *w, World_Chunk *chunk, iv2 tile_coords) {
  assert(chunk);
  assert((tile_coords.x >= 0) && (tile_coords.x < w->chunk_dim) &&
        (tile_coords.y >= 0) && (tile_coords.y < w->chunk_dim));

  return chunk->tiles[tile_coords.x + w->chunk_dim*tile_coords.y];
}

b32 get_tile_value(World *w, World_Chunk *chunk, iv2 tile_pos) {
  u32 chunk_value = 0;
  if (chunk && chunk->tiles) {
    if ((tile_pos.x >=0) && (tile_pos.x < w->chunk_dim) && 
        (tile_pos.y >= 0) && (tile_pos.y < w->chunk_dim)) {
      chunk_value = get_world_chunk_value_nocheck(w, chunk, tile_pos);
    } 
  }
  return chunk_value;
}
#endif


World_Chunk *get_world_chunk_arena(World *w, iv2 chunk_coords, Arena *arena) {
  World_Chunk *chunk= nullptr;

  // TODO: better hash function
  u32 coord_hash = 33*chunk_coords.x + 68*chunk_coords.y;
  u32 hash_slot = coord_hash % array_count(w->chunk_slots);

  b32 found = false;
  for (World_Chunk *c = w->chunk_slots[hash_slot]; c != nullptr; c = c->next) {
    if (c->chunk_x == chunk_coords.x && c->chunk_y == chunk_coords.y) {
      chunk = c;
      found = true;
    }
  }
  if (!found && arena) {
    World_Chunk *new_chunk = arena_push_struct(arena, World_Chunk);
    new_chunk->chunk_x = chunk_coords.x;
    new_chunk->chunk_y = chunk_coords.y;
    new_chunk->tiles = arena_push_array(arena, u32, w->chunk_dim * w->chunk_dim);

    w->chunk_slots[hash_slot] = sll_stack_push(w->chunk_slots[hash_slot], new_chunk);

    chunk = new_chunk;
  }

  return chunk;
}

World_Chunk *get_world_chunk(World *w, iv2 chunk_coords) {
  return get_world_chunk_arena(w, chunk_coords, nullptr);
}

World_Position canonicalize_position(World *w, World_Position pos) {
  World_Position p = pos;

  // For x-axis
  f32 tile_rel_x = pos.offset.x;
  s32 tile_offset_x = round_f32((f32)tile_rel_x / w->tile_dim_meters.x);
  p.abs_coords.x += tile_offset_x;
  p.offset.x = tile_rel_x - w->tile_dim_meters.x * tile_offset_x;

  // For y-axis
  f32 tile_rel_y = pos.offset.y;
  s32 tile_offset_y = round_f32((f32)tile_rel_y / w->tile_dim_meters.y);
  p.abs_coords.y += tile_offset_y;
  p.offset.y = tile_rel_y - w->tile_dim_meters.y * tile_offset_y;

  return p;
}

World_Chunk_Position get_chunk_pos(World *w, iv2 abs_coords) {
  World_Chunk_Position chunk_pos = {
    .chunk_coords = iv2m(abs_coords.x >> w->chunk_shift, abs_coords.y >> w->chunk_shift),
    .chunk_rel = iv2m(abs_coords.x & w->chunk_mask, abs_coords.y & w->chunk_mask),
  };

  return chunk_pos;
}

// FIXME: This will lead to rounding errors, it would be
// best if we could subtract positions tbh..
// (instead of working with absolutes)
v2 get_world_fpos_in_meters(World *w, World_Position pos) {
  pos = canonicalize_position(w, pos);

  v2 tile_dim_px = w->tile_dim_px; 
  s32 chunk_dim = w->chunk_dim; 
  v2 p2m = v2_div(w->tile_dim_meters, w->tile_dim_px);
  World_Chunk_Position chunk_pos = get_chunk_pos(w, pos.abs_coords);

  return v2m(
    p2m.x * chunk_pos.chunk_coords.x * chunk_dim * tile_dim_px.x + p2m.x * chunk_pos.chunk_rel.x * tile_dim_px.x + pos.offset.x,
    p2m.y * chunk_pos.chunk_coords.y * chunk_dim * tile_dim_px.y + p2m.y * chunk_pos.chunk_rel.y * tile_dim_px.y + pos.offset.y
  );

}


