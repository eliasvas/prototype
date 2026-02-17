#include "world.h"

// TODO: This should be called just INVALID_CHUNK mayb?
#define WORLD_POS_INVALID_CHUNK (-S32_MAX)

World_Position world_pos_nil(void) {
  World_Position nilp = (World_Position) {
    .chunk = iv2m(WORLD_POS_INVALID_CHUNK, WORLD_POS_INVALID_CHUNK),
    .offset = v2m(0,0),
  };
  return nilp;
}

b32 world_pos_is_valid(World_Position wp) {
  World_Position nilp = world_pos_nil();
  b32 valid = (wp.chunk.x == nilp.chunk.x) && (wp.chunk.y == nilp.chunk.y);
  return valid;
}

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
    s32 extra_chunks = floor_f32((f32)chunk_offset / w->chunk_dim_meters.x); 
    p.chunk.x += extra_chunks; // add to chunk_pos the extra offset
    p.offset.x = chunk_offset - w->tile_dim_meters.x * extra_chunks * w->tiles_per_chunk;
  }

   {
    // For y-axis
    f32 chunk_offset = pos.offset.y; 
    s32 extra_chunks = floor_f32((f32)chunk_offset / w->chunk_dim_meters.y); 
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
#if 0
  assert(p.offset.x >= -0.5 * w->chunk_dim_meters.x);
  assert(p.offset.x <= 0.5 * w->chunk_dim_meters.x);
  assert(p.offset.y >= -0.5 * w->chunk_dim_meters.y);
  assert(p.offset.y <= 0.5 * w->chunk_dim_meters.y);
#endif
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

World_Position chunk_pos_from_tile_pos(World *w, v2 tile_pos) {
  World_Position pos = {};

  pos.chunk.x = tile_pos.x / w->tiles_per_chunk;
  pos.chunk.y = tile_pos.y / w->tiles_per_chunk;

  pos.offset.x = (f32)(tile_pos.x - (pos.chunk.x*w->tiles_per_chunk)) * w->tile_dim_meters.x;
  pos.offset.y = (f32)(tile_pos.y - (pos.chunk.y*w->tiles_per_chunk)) * w->tile_dim_meters.y;

  return (pos);
}

// This will update the block cache moving the entity along the different chunks
World_Position change_entity_location(Arena *arena, World *w, u32 low_entity_idx, World_Position *old, World_Position *new) {
  // 0. If there was a block and its on different chunk, remove it
  bool should_insert = true;
  if (old) {
    if (!are_in_same_chunk(w, *old, *new)) {
      World_Chunk *chunk = get_world_chunk(w, old->chunk);
      if (chunk) {
        b32 found = false;
        // 1. Search to find the specific low_entity_idx 
        for (World_Entity_Block *block = chunk->first; block != nullptr && !found; block = block->next) {
          for (u32 entity_itr = 0; entity_itr < block->count && !found; entity_itr +=1) {
            // 2. if found remove it!
            if (block->low_entity_indices[entity_itr] == low_entity_idx) {
              block->low_entity_indices[entity_itr] = block->low_entity_indices[--block->count];

              // 3. if the block is empty, we will push it to the freelist
              if (block->count == 0) {
                  World_Entity_Block *free_block = block;
                  dll_remove(chunk->first, chunk->last, block);
                  M_ZERO_STRUCT(free_block);
                  free_block->next = w->block_freelist;
                  w->block_freelist = free_block;
              }
              found = true;
            }
          }
        }
      }
    } else {
      should_insert = false;
    }
  }

  // 4. Map the new block to the correct chunk now
  if (new && should_insert) {
    World_Chunk *chunk = get_world_chunk_arena(w, new->chunk, arena);
    World_Entity_Block *block = chunk->last;
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
      block = new_block;
    }
    assert(block);
    assert(low_entity_idx);
    block->low_entity_indices[block->count++] = low_entity_idx;

  }

  // 5. return the final entity low position
  return (new) ? *(new) : world_pos_nil();
}

