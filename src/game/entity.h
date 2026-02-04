#ifndef ENTITY_H__
#define ENTITY_H__

#include "base/base_inc.h"
#include "core/core_inc.h"
#include "world.h"

typedef enum {
  ENTITY_KIND_NIL = 0,
  ENTITY_KIND_PLAYER = 1,
  ENTITY_KIND_WALL = 2,
  ENTITY_KIND_FAMILIAR = 3,
  ENTITY_KIND_MONSTER = 4,
  ENTITY_KIND_SWORD = 5,
} Entity_Kind;

typedef struct {
  u8 flags;
  u8 filled_amount;
} Hit_Point;

typedef struct {
  v2 dir; // unit vector
  f32 speed;
  f32 drag;

} Move_Spec;

typedef struct Sim_Entity Sim_Entity;
typedef struct {
  Sim_Entity *ptr;
  u32 storage_idx;
} Entity_Ref;

// TODO: Move to sim_region!!
struct Sim_Entity {
  v2 p;


  b32 collides;
  Entity_Kind kind;
  v2 delta_p;
  v2 dim_meters;

  u32 hit_point_count;
  Hit_Point hit_points[16];

  // Garbo
  Entity_Ref sword_ref;
  f32 movement_remaining;
  v2 hit_dir;

  // Ref
  u32 storage_idx;
};

#endif
