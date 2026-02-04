
#include "sim_region.h"

//  region->hash_slot_count = 1024;
//  M_ZERO(region->hash_slots, sizeof(Sim_Entity_Hash_Slot)*region->hash_slot_count);

Sim_Entity_Hash_Node *get_sim_entity_hash_from_storage_idx(Arena *sim_arena, Sim_Region *region, u32 storage_idx) {
  u32 slot_hash = storage_idx % region->hash_slot_count; // TODO: better hashing
  Sim_Entity_Hash_Slot *slot = &region->hash_slots[slot_hash];
  assert(slot);

  // Try to find the storage index in the hash slots
  for (Sim_Entity_Hash_Node *node = slot->first; node != nullptr; node=node->next) {
    if (node->storage_idx == storage_idx) {
      return node;
    }
  }
  // If not found create a new node
  Sim_Entity_Hash_Node *new_node = region->free_hash_nodes;
  if (new_node) {
    sll_stack_pop(region->free_hash_nodes);
  } else {
    new_node = arena_push_struct(sim_arena, Sim_Entity_Hash_Node);
  }
  dll_push_back(slot->first, slot->last, new_node);

  return new_node;
}

void map_storage_idx_to_sim_entity(Arena *sim_arena, Sim_Region *region, u32 storage_idx, Sim_Entity *entity) {
  Sim_Entity_Hash_Node *hash = get_sim_entity_hash_from_storage_idx(sim_arena, region, storage_idx);
  assert((hash->storage_idx == 0)||(hash->storage_idx == storage_idx));
  hash->ptr = entity;
  hash->storage_idx = storage_idx;
}

Sim_Entity *get_sim_entity_from_storage_idx(Arena *sim_arena, Sim_Region *region, u32 storage_idx) {
  Sim_Entity_Hash_Node *hash = get_sim_entity_hash_from_storage_idx(sim_arena, region, storage_idx);
  return hash->ptr;
}

v2 get_sim_space_pos(Sim_Region *sim_region, Low_Entity *low) {
  v2 entity_pos_mt = get_world_fpos_in_meters(sim_region->world_ref, low->p); 
  v2 sim_center_pos_mt = get_world_fpos_in_meters(sim_region->world_ref, sim_region->origin); 
  // TODO: check -- should we reverse this?
  v2 diff = v2_sub(entity_pos_mt, sim_center_pos_mt);
  //v2 diff = v2_sub(sim_center_pos_mt, entity_pos_mt);
  return diff;
}


Low_Entity *get_low_entity(Game_State *gs, u64 low_entity_idx);
void load_entity_ref(Game_State *gs, Arena *sim_arena, Sim_Region *region, Entity_Ref *ref);

Sim_Entity* add_sim_entity_to_region(Game_State *gs, Sim_Region *region, u32 storage_idx) {
  Sim_Entity *entity = nullptr;

  if (region->entity_count < region->entity_cap) {
    entity = &region->entities[region->entity_count++];
    M_ZERO_STRUCT(entity);
    Low_Entity *low = get_low_entity(gs, storage_idx);
    assert(low);
    map_storage_idx_to_sim_entity(region->arena_ref, region, storage_idx, entity);
    if (low) {
      *entity = low->sim;

      v2 pos = get_sim_space_pos(region, low);
      entity->p = pos;
      entity->storage_idx = storage_idx;

      load_entity_ref(gs, region->arena_ref, region, &entity->sword_ref);
    }
  } else{
    assert(region->entity_count < region->entity_cap && "No more sim_entities available");
  }

  return entity;
}

void store_entity_ref(Sim_Region *region, Entity_Ref *ref) {
  if (ref->ptr != nullptr) {
    ref->storage_idx = ref->ptr->storage_idx;
  }
}

void load_entity_ref(Game_State *gs, Arena *sim_arena, Sim_Region *region, Entity_Ref *ref) {
  if (ref->storage_idx) {
    Sim_Entity_Hash_Node *hash = get_sim_entity_hash_from_storage_idx(sim_arena, region, ref->storage_idx);
    if (hash->ptr == nullptr) {
      assert(ref->storage_idx);
      hash->storage_idx = ref->storage_idx;
      Sim_Entity* entity = add_sim_entity_to_region(gs, region, hash->storage_idx);
      entity->storage_idx = ref->storage_idx;
      assert(entity);
      hash->ptr = entity;
    }
  }
}

Sim_Entity* add_sim_entity(Game_State *gs, Sim_Region *region, u32 low_entity_idx) {
  Sim_Entity *entity = add_sim_entity_to_region(gs, region, low_entity_idx);
  Low_Entity *low = get_low_entity(gs, low_entity_idx);

  v2 pos = get_sim_space_pos(region, low);
  entity->p = pos;
  entity->storage_idx = low_entity_idx;

  return entity;
}


Sim_Region *begin_sim(Arena *sim_arena, Game_State *gs, World *w, World_Position origin, rect bounds) {
  // 0. Allocate the sim_region
  Sim_Region *region = arena_push_struct(sim_arena, Sim_Region);
  region->world_ref = w;
  region->arena_ref = sim_arena;
  region->origin = origin;
  region->bounds = bounds;
  region->entity_cap = 1024;
  region->entity_count = 1;
  region->entities = arena_push_array(sim_arena, Sim_Entity, region->entity_cap);

  region->hash_slot_count = 1024;
  M_ZERO(region->hash_slots, sizeof(Sim_Entity_Hash_Slot)*region->hash_slot_count);


  // 1. Find close-enough entities and _add_ them to the region
  s32 screens_to_include = 1;
  World_Position camera_bottom = gs->gworld.camera_p;
  camera_bottom.offset = v2_sub(camera_bottom.offset, v2_multf(v2m(gs->gworld.screen_dim_in_tiles.x, gs->gworld.screen_dim_in_tiles.y), screens_to_include * gs->gworld.w->tile_dim_meters.x));
  camera_bottom = canonicalize_position(gs->gworld.w, camera_bottom);

  World_Position camera_top = gs->gworld.camera_p;
  camera_top.offset = v2_add(camera_top.offset, v2_multf(v2m(gs->gworld.screen_dim_in_tiles.x, gs->gworld.screen_dim_in_tiles.y), screens_to_include * gs->gworld.w->tile_dim_meters.x));
  camera_top = canonicalize_position(gs->gworld.w, camera_top);

  assert(camera_bottom.chunk.x <= camera_top.chunk.x);
  assert(camera_bottom.chunk.y <= camera_top.chunk.y);
  //int add_count = 0;
  for (s32 chunk_x = camera_bottom.chunk.x; chunk_x <= camera_top.chunk.x; chunk_x += 1) {
    for (s32 chunk_y = camera_bottom.chunk.y; chunk_y <= camera_top.chunk.y; chunk_y += 1) {
      World_Chunk *chunk = get_world_chunk_arena(gs->gworld.w, iv2m(chunk_x, chunk_y), gs->persistent_arena);
      for (World_Entity_Block *block = chunk->first; block != nullptr; block = block->next) {
        //printf("block count: %d, next: %lu\n", block->count, UINT_FROM_PTR(block->next));
        for (u32 block_entity_idx = 0; block_entity_idx < block->count; block_entity_idx+=1) {
          u32 storage_idx = block->low_entity_indices[block_entity_idx];

          // TODO: should be renamed to stored_entity?
          Low_Entity *low_entity = &gs->low_entities[storage_idx];
          v2 entity_fpos = get_world_fpos_in_meters(gs->gworld.w, low_entity->p);
          rect entity_bounding_box = rec_centered(entity_fpos, v2_multf(low_entity->sim.dim_meters, 0.5));
          if (rect_isect_rect(region->bounds, entity_bounding_box)) {
            //add_count+=1;
            Sim_Entity *sim_entity = add_sim_entity(gs, region, storage_idx);
            // Fill out the entity
            assert(sim_entity);
            World_Position entity_wpos = low_entity->p;
            // We find the relative position for the entity and store it in the sim_region array
            v2 entity_fpos = get_world_fpos_in_meters(gs->gworld.w, entity_wpos);
            v2 sim_origin_fpos = get_world_fpos_in_meters(gs->gworld.w, region->origin);
            v2 relative_fpos = v2_sub(entity_fpos, sim_origin_fpos);
            sim_entity->p = relative_fpos;
            sim_entity->storage_idx = storage_idx;
          }
        }
      }
    }
  }
  //printf("Entity adds this frame: %d\n", add_count);


  return region;
}


static World_Position map_fpos_to_tile_map_position(World *w, World_Position pos, v2 offset_fpos) {
  World_Position base = pos;
  base.offset = v2_add(base.offset, offset_fpos);

  base = canonicalize_position(w, base);
  return base;
}

void end_sim(Sim_Region *region, Game_State *gs) {
  // 2. Map sim_entities back to stored_entities

  for (u32 entity_idx = 1; entity_idx < region->entity_count; entity_idx+=1) {
    Sim_Entity entity = region->entities[entity_idx];
    //World_Position camera_p = region->origin;

    Low_Entity *low = &gs->low_entities[entity.storage_idx];
    low->sim = entity;
    World_Position new_entity_wpos = map_fpos_to_tile_map_position(gs->gworld.w, region->origin, entity.p);
    low->p = change_entity_location(gs->persistent_arena, gs->gworld.w, entity.storage_idx, &low->p, &new_entity_wpos);
  }

}
