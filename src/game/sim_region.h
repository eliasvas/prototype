#ifndef SIM_REGION_H__
#define SIM_REGION_H__

#include "world.h"
#include "game.h"

typedef struct {
  World *world_ref;
  World_Position origin;
  rect bounds;

  u32 entity_cap;
  u32 entity_count;
  Sim_Entity *entities;
} Sim_Region;

Sim_Region *begin_sim(Arena *sim_arena, Game_State *gs, World *w, World_Position origin, rect bounds);
void end_sim(Sim_Region *region, Game_State *gs);

#endif
