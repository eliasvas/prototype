#ifndef SIM_REGION_H__
#define SIM_REGION_H__

#include "entity.h"
#include "world.h"
#include "game.h"

typedef struct Sim_Entity_Hash_Node Sim_Entity_Hash_Node;

struct Sim_Entity_Hash_Node {
  Sim_Entity *ptr;
  u32 storage_idx;

  Sim_Entity_Hash_Node *next;
  Sim_Entity_Hash_Node *prev;
};

typedef struct Sim_Entity_Hash_Slot Sim_Entity_Hash_Slot;
struct Sim_Entity_Hash_Slot {
  Sim_Entity_Hash_Node *first;
  Sim_Entity_Hash_Node *last;
};

typedef struct {
  World *world_ref;
  Arena *arena_ref;

  World_Position origin;
  rect updatable_bounds;
  rect bounds;

  u32 entity_cap;
  u32 entity_count;
  Sim_Entity *entities;

  Sim_Entity_Hash_Slot hash_slots[1024];
  u32 hash_slot_count;
  Sim_Entity_Hash_Node *free_hash_nodes;
} Sim_Region;


Sim_Region *begin_sim(Arena *sim_arena, Game_State *gs, World *w, World_Position origin, rect bounds);
void end_sim(Sim_Region *region, Game_State *gs);



#endif
