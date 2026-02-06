#include "entity.h"
#include "world.h"

b32 sim_entity_is_flag_set(Sim_Entity *entity, u32 flag) {
  b32 result = (entity->flags & flag);
  return result;
}

void sim_entity_add_flag(Sim_Entity *entity, u32 flag) {
  entity->flags |= flag;
}

void sim_entity_clear_flag(Sim_Entity *entity, u32 flag) {
  entity->flags &= ~flag;
}

void make_sim_entity_non_spatial(Sim_Entity *entity) {
  sim_entity_add_flag(entity, ENTITY_FLAG_NONSPATIAL);
  entity->p = INVALID_P;
}

// TODO: Add dp?
void make_sim_entity_spatial(Sim_Entity *entity, v2 p) {
  sim_entity_clear_flag(entity, ENTITY_FLAG_NONSPATIAL);
  entity->p = p;
}

