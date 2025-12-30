#include "world.h"

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
    //new_chunk->tiles = arena_push_array(arena, u32, w->chunk_dim * w->chunk_dim);

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

   {
    // For x-axis
    f32 chunk_offset = pos.offset.x; 
    s32 extra_tiles = round_f32((f32)chunk_offset / w->tile_dim_meters.x); 
    s32 extra_chunks = extra_tiles / w->tiles_per_chunk;
    p.chunk.x += extra_chunks; // add to chunk_pos the extra offset
    p.offset.x = chunk_offset - w->tile_dim_meters.x * extra_chunks * w->tiles_per_chunk;
  }

  {
    // For y-axis
    f32 chunk_offset = pos.offset.y; 
    s32 extra_tiles = round_f32((f32)chunk_offset / w->tile_dim_meters.y); 
    s32 extra_chunks = extra_tiles / w->tiles_per_chunk;
    p.chunk.y += extra_chunks; // add to chunk_pos the extra offset
    p.offset.y = chunk_offset - w->tile_dim_meters.y * extra_chunks * w->tiles_per_chunk;
  }

  return p;
}

// FIXME: This will lead to rounding errors, it would be
// best if we could subtract positions tbh..
// (instead of working with absolutes)
v2 get_world_fpos_in_meters(World *w, World_Position pos) {
  // FIXME: is this needed??
  pos = canonicalize_position(w, pos);

  return v2m(
    pos.chunk.x * w->chunk_dim_meters.x + pos.offset.x,
    pos.chunk.y * w->chunk_dim_meters.y + pos.offset.y
  );
}


//TODO: Chunk dim is symmetrical, no reason to
// use a v2 here, just make it f32
b32 is_world_pos_canonical(World *w, World_Position p) {
  assert(p.chunk.x >= -0.5 * w->chunk_dim_meters.x);
  assert(p.chunk.x <= 0.5 * w->chunk_dim_meters.x);
  assert(p.chunk.y >= -0.5 * w->chunk_dim_meters.y);
  assert(p.chunk.y <= 0.5 * w->chunk_dim_meters.y);
  return true;
}

b32 are_in_same_chunk(World *w, World_Position a, World_Position b) {
  assert(is_world_pos_canonical(w, a));
  assert(is_world_pos_canonical(w, b));
  b32 result = true;
  result &= (a.chunk.x == b.chunk.x);
  result &= (a.chunk.y == b.chunk.y);
  //result &= (a.chunk.z == b.chunk_z);

  return(result);
}

World_Position chunk_pos_from_tile_pos(World *w, iv2 abs_tile) {
  World_Position pos = {};

  pos.chunk.x = abs_tile.x / w->tiles_per_chunk;
  pos.chunk.y = abs_tile.y / w->tiles_per_chunk;

  pos.offset.x = (f32)(abs_tile.x - (pos.chunk.x*w->tiles_per_chunk)) * w->tile_dim_meters.x;
  pos.offset.y = (f32)(abs_tile.y - (pos.chunk.y*w->tiles_per_chunk)) * w->tile_dim_meters.y;

  return (pos);
}

void change_entity_location(Arena *arena, World *w, u32 low_entity_idx, World_Position *old, World_Position *new) {
  // 0. If there was a block and its on different chunk, remove it
  if (old) {
    if (!are_in_same_chunk(w, *old, *new)) {

      World_Chunk *chunk = get_world_chunk(w, old->chunk);
      if (chunk) {
        // 1. Search to find the specific low_entity_idx 
        for (World_Entity_Block *block = chunk->first; block != nullptr; block = block->next) {
          for (u32 entity_itr = 0; entity_itr < block->count; entity_itr +=1) {
            // 2. if found remove it!
            if (block->low_entity_indices[entity_itr] == low_entity_idx) {
              block->low_entity_indices[entity_itr] = block->low_entity_indices[--block->count];

              // 3. if the block is empty, we will push it to the freelist
              if (chunk->first->count == 0) {
                  World_Entity_Block *free_block = dll_remove(chunk->first, chunk->last, block);
                  M_ZERO_STRUCT(free_block);
                  free_block->next = w->block_freelist;
                  w->block_freelist = free_block;
              }
              // double break to exit iteration
              block = nullptr;
              break;
            }
          }
        }
      }
    }
  }

  // 4. Map the new block to the correct chunk now
  World_Chunk *chunk = get_world_chunk(w, new->chunk);
  World_Entity_Block *block = chunk->first;
  if (!block || block->count == block->cap) {
    World_Entity_Block *new_block = w->block_freelist;
    if (new_block) {
      w->block_freelist = new_block->next;
    } else {
      new_block = arena_push_struct(arena, World_Entity_Block); 
    }
    M_ZERO_STRUCT(new_block);
    new_block->cap = array_count(new_block->low_entity_indices);
    dll_push_back(chunk->first, chunk->last, new_block);
  }
  block->low_entity_indices[block->count++] = low_entity_idx;

}
