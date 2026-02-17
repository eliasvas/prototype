#ifndef ENTITY_H__
#define ENTITY_H__

#include "base/base_inc.h"
#include "core/core_inc.h"
#include "world.h"

#define INVALID_P (v2m(12345, 12345))

typedef enum {
  ENTITY_FLAG_COLLIDES = (1 << 0),
  ENTITY_FLAG_NONSPATIAL = (1 << 1),
} Entity_Flags;

typedef enum {
  ENTITY_KIND_NIL = 0,
  ENTITY_KIND_HERO = 1,
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

  Entity_Kind kind;
  Entity_Flags flags;
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
  b32 updatable;
};

b32 sim_entity_is_flag_set(Sim_Entity *entity, u32 flag);
void sim_entity_add_flag(Sim_Entity *entity, u32 flag);
void sim_entity_clear_flag(Sim_Entity *entity, u32 flag);
void make_sim_entity_non_spatial(Sim_Entity *entity);
void make_sim_entity_spatial(Sim_Entity *entity, v2 p);

rect sim_entity_get_collider_rect(Sim_Entity *entity, v2 p);
#endif
