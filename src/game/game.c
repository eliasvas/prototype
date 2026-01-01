#include "game.h"
#include "gui/gui.h"

// For random (maybe we should make our own though..)
#include "stdlib.h"


void rend_push_tile_alpha(Game_State *gs, u32 atlas_idx, World_Position tile, World_Position camera, v2 dim_px, f32 alpha) {
  // Make a World_Position w/ the needed coordinates relative to 'camera' World_Position

  // Calculate on-screen coords
  v2 m2p = v2_div(gs->world.w->tile_dim_px, gs->world.w->tile_dim_meters);

  v2 pos_meters = get_world_fpos_in_meters(gs->world.w, tile);
  v2 camera_meters = get_world_fpos_in_meters(gs->world.w, camera);
  v2 relative_pos_px = v2_mult(v2_sub(pos_meters, camera_meters), m2p);

  rect dst = rec(
      relative_pos_px.x, relative_pos_px.y,
      dim_px.x, dim_px.y
  );

  // Bonus: Modify rectangle so that 'camera' World_Position is at the center of the screen
  v2 screen_offset = v2_divf(gs->screen_dim, 2.0f);
  dst.x += screen_offset.x;
  dst.y += screen_offset.y;

  // Bonus: apply a transform to make it y-up coordinate system (OpenGL-style)
  iv2 llc = iv2m(gs->world.lower_left_corner.x,gs->world.lower_left_corner.y);
  rect yup_rect = dst;
  yup_rect.y = llc.y - dst.h - dst.y;

  // Finally push the rectangle for drawing :)
  u32 xidx = (u32)atlas_idx % gs->atlas_sprites_per_dim.x;
  u32 yidx = (u32)atlas_idx / gs->atlas_sprites_per_dim.x;
  v2 px_per_tile = v2m(gs->atlas.width / gs->atlas_sprites_per_dim.x, gs->atlas.height / gs->atlas_sprites_per_dim.y);
  R2D_Quad quad = (R2D_Quad) {
      .src_rect = rec(xidx*px_per_tile.x, yidx*px_per_tile.y, px_per_tile.x, px_per_tile.y),
      .dst_rect = yup_rect,
      .c = col(alpha,alpha,alpha,1),
      .tex = gs->atlas,
      .rot_deg = 0,
  };

  R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_ADD_QUAD, .q = quad};
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);

}

void rend_push_tile(Game_State *gs, u32 atlas_idx, World_Position tile, World_Position camera, v2 dim_px) {
  rend_push_tile_alpha(gs, atlas_idx, tile, camera, dim_px, 1.0);
}

// Entity index zero is always invalid
b32 entity_idx_is_nil(u64 entity_idx) {
  return (entity_idx == 0);
}

High_Entity *make_entity_high_freq(Game_State *gs, u32 low_entity_idx) {
  High_Entity *high;
  Low_Entity *low = &gs->low_entities[low_entity_idx];
  if (low->high_entity_idx) {
    high = &gs->high_entities[low->high_entity_idx];
  } else {
    assert(gs->high_entity_count < array_count(gs->high_entities));
    u32 high_entity_idx = gs->high_entity_count++;
    high = &gs->high_entities[high_entity_idx];

    World_Position entity_pos = low->p;
    World_Position camera_pos = gs->world.camera_p;

    // We find the relative position for the entity and store it in the high_entity array 
    v2 entity_fpos = get_world_fpos_in_meters(gs->world.w, entity_pos);
    v2 camera_fpos = get_world_fpos_in_meters(gs->world.w, camera_pos);
    v2 relative_fpos = v2_sub(camera_fpos, entity_fpos);
    high->p = relative_fpos;
    high->low_entity_idx = low_entity_idx;

    low->high_entity_idx = high_entity_idx;
  }

  return high;
}
void make_entity_low_freq(Game_State *gs, u32 low_entity_idx) {
  Low_Entity *low = &gs->low_entities[low_entity_idx];
  u32 high_entity_idx = low->high_entity_idx;
  // If it has a high entity, we remove it.
  if (high_entity_idx) {
    u32 last_high_idx = gs->high_entity_count - 1;
    if (high_entity_idx != last_high_idx) {
      u32 last_low_idx = gs->high_entities[last_high_idx].low_entity_idx;
      gs->high_entities[high_entity_idx] = gs->high_entities[last_high_idx];
      gs->low_entities[last_low_idx].high_entity_idx = high_entity_idx;
    }
    gs->high_entity_count -= 1;
    low->high_entity_idx = 0;
  }
}
 

Entity entity_from_high_entity(Game_State *gs, u64 high_entity_idx) {
  Entity entity = {};

  if (high_entity_idx) {
    entity.high = &gs->high_entities[high_entity_idx];
    entity.low_entity_idx = entity.high->low_entity_idx;
    entity.low = &gs->low_entities[entity.low_entity_idx];
  }

  return entity;
}

Low_Entity *get_low_entity(Game_State *gs, u64 low_entity_idx) {
  Low_Entity *low = 0;

  if ((low_entity_idx > 0) && (low_entity_idx < gs->low_entity_count)) {
    low = &gs->low_entities[low_entity_idx];
  }

  // For now..
  assert(!entity_idx_is_nil(low_entity_idx));

  return low;
}


Entity get_high_entity(Game_State *gs, u64 low_entity_idx) {
  Entity entity = {};

  if ((low_entity_idx > 0) && (low_entity_idx < gs->low_entity_count)) {
    entity.low_entity_idx = low_entity_idx,
    entity.low = &gs->low_entities[low_entity_idx];
    entity.high = make_entity_high_freq(gs, low_entity_idx);
  }

  // For now..
  assert(!entity_idx_is_nil(low_entity_idx));

  return entity;
}

u64 add_entity(Game_State *gs, Entity_Kind kind) {
  u64 low_entity_idx = 0;
  // Make the nil entity first
  if (gs->low_entity_count == 0) {
    gs->high_entities[gs->high_entity_count] = (High_Entity){};
    gs->low_entities[gs->low_entity_count] = (Low_Entity){};

    gs->low_entity_count += 1;
    gs->high_entity_count += 1;
  }
  // Add the actual entity
  low_entity_idx = gs->low_entity_count; 

  gs->low_entities[low_entity_idx] = (Low_Entity){
    .kind = kind,
  };
  gs->low_entity_count += 1;
  assert(gs->low_entity_count < array_count(gs->low_entities));

  return low_entity_idx;
}

static World_Position map_fpos_to_tile_map_position(World *w, World_Position pos, v2 offset_fpos) {
  World_Position base = pos;
  base.offset = v2_add(base.offset, offset_fpos);

  base = canonicalize_position(w, base);
  return base;
}

Low_Entity_Result add_low_entity(Game_State *gs, Entity_Kind kind, World_Position p) {
  Low_Entity_Result res = {};
  u32 low_entity_idx = add_entity(gs, kind);
  Low_Entity *low = get_low_entity(gs, low_entity_idx);
  low->kind = kind;
  low->p = p;
  low->collides = true;


  // TODO: Add an arena just for the game (gs->game_arena) 
  change_entity_location(gs->persistent_arena, gs->world.w, low_entity_idx, nullptr, &low->p);

  res.entity_idx = low_entity_idx;
  res.low = low;

  return res;
}

Low_Entity_Result add_player(Game_State *gs) {
  Low_Entity_Result low = add_low_entity(gs, ENTITY_KIND_PLAYER, chunk_pos_from_tile_pos(gs->world.w, iv2m(2,2)));
  Entity player_entity = get_high_entity(gs, low.entity_idx);
  player_entity.low->dim_meters = v2_multf(gs->world.w->tile_dim_meters, 0.75);

  return low;
}

Low_Entity_Result add_wall(Game_State *gs, iv2 tile_pos) {
  Low_Entity_Result low = add_low_entity(gs, ENTITY_KIND_WALL, chunk_pos_from_tile_pos(gs->world.w, tile_pos));
  low.low->dim_meters = v2_multf(gs->world.w->tile_dim_meters, 1.0);

  return low;
}

Low_Entity_Result add_familiar(Game_State *gs, iv2 tile_pos) {
  Low_Entity_Result low = add_low_entity(gs, ENTITY_KIND_FAMILIAR, chunk_pos_from_tile_pos(gs->world.w, tile_pos));
  low.low->dim_meters = v2_multf(gs->world.w->tile_dim_meters, 0.5);
  low.low->collides = false;

  return low;
}

Low_Entity_Result add_monster(Game_State *gs, iv2 tile_pos) {
  Low_Entity_Result low = add_low_entity(gs, ENTITY_KIND_MONSTER, chunk_pos_from_tile_pos(gs->world.w, tile_pos));
  low.low->dim_meters = v2_multf(gs->world.w->tile_dim_meters, 1.0);

  return low;
}

void set_camera(Game_State *gs, World_Position new_camera_pos) {
  // First, calculate the actual camera delta + set the new camera position
  v2 prev_cam_pos_mt = get_world_fpos_in_meters(gs->world.w, gs->world.camera_p);
  v2 new_cam_pos_mt = get_world_fpos_in_meters(gs->world.w, new_camera_pos);
  v2 cam_diff_mt = v2_sub(prev_cam_pos_mt, new_cam_pos_mt);
  gs->world.camera_p = new_camera_pos;

  // Then, we fixup current High entities for the new camera delta
  for (u32 high_entity_idx = 1; high_entity_idx < gs->high_entity_count; ++high_entity_idx) {
    High_Entity *high = &gs->high_entities[high_entity_idx];
    high->p = v2_add(high->p, cam_diff_mt);
  }


  // Then, cull all entities outside camera bounding box - make them low!
  // Or add them to high entities if they ARE inside the bounding box but low
  s32 screens_to_include = 1;
  v2 camera_p_in_ws = get_world_fpos_in_meters(gs->world.w, gs->world.camera_p);
  rect camera_bounding_box = rec_centered(camera_p_in_ws, v2_multf(v2m(gs->world.screen_dim_in_tiles.x, gs->world.screen_dim_in_tiles.y), screens_to_include));

#if 1
  World_Position camera_bottom = gs->world.camera_p;
  camera_bottom.offset = v2_sub(camera_bottom.offset, v2_multf(v2m(gs->world.screen_dim_in_tiles.x, gs->world.screen_dim_in_tiles.y), screens_to_include * gs->world.w->tile_dim_meters.x));
  camera_bottom = canonicalize_position(gs->world.w, camera_bottom);

  World_Position camera_top = gs->world.camera_p;
  camera_top.offset = v2_add(camera_top.offset, v2_multf(v2m(gs->world.screen_dim_in_tiles.x, gs->world.screen_dim_in_tiles.y), screens_to_include * gs->world.w->tile_dim_meters.x));
  camera_top = canonicalize_position(gs->world.w, camera_top);

  assert(camera_bottom.chunk.x <= camera_top.chunk.x);
  assert(camera_bottom.chunk.y <= camera_top.chunk.y);
  for (s32 chunk_x = camera_bottom.chunk.x; chunk_x <= camera_top.chunk.x; chunk_x += 1) {
    for (s32 chunk_y = camera_bottom.chunk.y; chunk_y <= camera_top.chunk.y; chunk_y += 1) {
      World_Chunk *chunk = get_world_chunk_arena(gs->world.w, iv2m(chunk_x, chunk_y), gs->persistent_arena);
      for (World_Entity_Block *block = chunk->first; block != nullptr; block = block->next) {
        for (u32 block_entity_idx = 0; block_entity_idx < block->count; block_entity_idx+=1) {
          u32 low_entity_idx = block->low_entity_indices[block_entity_idx];
          Low_Entity *low_entity = get_low_entity(gs, low_entity_idx);

          v2 entity_fpos = get_world_fpos_in_meters(gs->world.w, low_entity->p);
          rect entity_bounding_box = rec_centered(entity_fpos, v2_multf(low_entity->dim_meters, 0.5));

          if (!rect_isect_rect(camera_bounding_box, entity_bounding_box)) {
            if (low_entity->high_entity_idx) {
              make_entity_low_freq(gs, low_entity_idx);
            }
          } else {
            if (low_entity->high_entity_idx == 0) {
              make_entity_high_freq(gs, low_entity_idx);
            }
          }
        }
      }
    }
  }
#else 
  // TODO: only use the chunks intersecting the camera
  for (u32 low_entity_idx = 1; low_entity_idx < gs->low_entity_count; ++low_entity_idx) {
    Low_Entity *low_entity = get_low_entity(gs, low_entity_idx);

    v2 entity_fpos = get_world_fpos_in_meters(gs->world.w, low_entity->p);
    rect entity_bounding_box = rec_centered(entity_fpos, v2_multf(low_entity->dim_meters, 0.5));

    if (!rect_isect_rect(camera_bounding_box, entity_bounding_box)) {
      if (low_entity->high_entity_idx) {
        make_entity_low_freq(gs, low_entity_idx);
      }
    } else {
      if (low_entity->high_entity_idx == 0) {
        make_entity_high_freq(gs, low_entity_idx);
      }
    }
  }
#endif

}


void move_entity(Game_State *gs, u64 low_entity_idx, v2 dp, f32 dt) {
  Entity entity = get_high_entity(gs, low_entity_idx);

  if (true) {
    // Check for entity/tile collisions
    b32 collides = false;
    for (u32 high_entity_idx = 1; high_entity_idx < gs->high_entity_count; ++high_entity_idx) {
      High_Entity *high = &gs->high_entities[high_entity_idx];
      if (high->low_entity_idx != low_entity_idx && entity_from_high_entity(gs, high_entity_idx).low->collides) {
        v2 entity_new_pos = v2_add(get_world_fpos_in_meters(gs->world.w, entity.low->p), dp);
        v2 entity_new_pos_left = v2_sub(entity_new_pos, v2m(entity.low->dim_meters.x/2,0));
        v2 entity_new_pos_right = v2_add(entity_new_pos, v2m(entity.low->dim_meters.x/2,0));

        Entity entity = get_high_entity(gs, high->low_entity_idx);
        v2 entity_pos_meters = get_world_fpos_in_meters(gs->world.w, entity.low->p);
        v2 entity_dim_mt = entity.low->dim_meters;
        rect entity_collision_rect = rec(entity_pos_meters.x, entity_pos_meters.y, entity_dim_mt.x, entity_dim_mt.y);
        if (!collides) {
          collides = (rect_isect_point(entity_collision_rect, entity_new_pos) ||
                     rect_isect_point(entity_collision_rect, entity_new_pos_left) ||
                     rect_isect_point(entity_collision_rect, entity_new_pos_right));
          //if (collides) { printf("collision succeded with rect %f %f %f %f and pos %f %f\n", entity_pos_meters.x, entity_pos_meters.y, tile_dim_mt.x, tile_dim_mt.y, dp.x, dp.y); }
        }
      }
    }
 

    // If there is no collision map the new entity position back to the low entity
    if (!collides) {
      entity.high->p = v2_add(entity.high->p, dp);
      World_Position new_entity_wpos = map_fpos_to_tile_map_position(gs->world.w, gs->world.camera_p, dp);
      change_entity_location(gs->persistent_arena, gs->world.w, low_entity_idx, &entity.low->p, &new_entity_wpos);
      entity.low->p = new_entity_wpos;
    }

  }

}

void update_familiar(Game_State *gs, Entity entity, f32 dt) {
  assert(entity.low->kind == ENTITY_KIND_FAMILIAR);
  for (u32 high_entity_idx = 1; high_entity_idx < gs->high_entity_count; ++high_entity_idx) {
    Entity test_entity = entity_from_high_entity(gs, high_entity_idx);

    if (test_entity.low->kind == ENTITY_KIND_PLAYER) {
      v2 distance = v2_sub(entity.high->p, test_entity.high->p);
      if (v2_len(distance) < 100) {
        f32 familiar_speed = 1.0;
        v2 dp = v2_multf(v2_norm(distance), familiar_speed);
        move_entity(gs, entity.low_entity_idx, dp, dt);
      }
    }
  }
}

void update_monster(Game_State *gs, Entity entity, f32 dt) {
  assert(entity.low->kind == ENTITY_KIND_MONSTER);
  // TBA
}

void update_hero(Game_State *gs, Entity entity, f32 dt) {
  assert(entity.low->kind == ENTITY_KIND_PLAYER);
  f32 hero_speed = 15.0;
  Input *input = &gs->input;
  v2 dp = v2m(0,0);
  if (input_key_down(input, KEY_SCANCODE_UP))dp.y+=hero_speed*dt;
  if (input_key_down(input, KEY_SCANCODE_DOWN))dp.y-=hero_speed*dt;
  if (input_key_down(input, KEY_SCANCODE_LEFT))dp.x-=hero_speed*dt;
  if (input_key_down(input, KEY_SCANCODE_RIGHT))dp.x+=hero_speed*dt;

  move_entity(gs, entity.low_entity_idx, dp, dt);
}



void game_init(Game_State *gs) {

  // Initialize the World
  gs->world.screen_dim_in_tiles = iv2m(9,7);
  gs->world.w = arena_push_struct(gs->persistent_arena, World);
  *(gs->world.w) = (World) {
      .tile_dim_px = v2m(32,32),
      .tile_dim_meters = v2m(1.5, 1.5),
      .tiles_per_chunk = 16,
  };
  gs->world.w->chunk_dim_meters = v2_multf(gs->world.w->tile_dim_meters, gs->world.w->tiles_per_chunk);
  M_ZERO_STRUCT(gs->world.w->chunk_slots);

  
  // Map generation (!!)
  b32 door_right = false;
  b32 door_left = false;
  b32 door_up = false;
  b32 door_down = false;
 
  iv2 screen_offset = iv2m(0,0);
  for (u32 screen_idx = 0; screen_idx < 40; screen_idx+=1) {
    // Generate a map for screen (x,y)
    u32 random_value = rand() % 2; 
    if ( random_value == 0) {
      door_right = true;
    } else {
      door_up = true;
    }

    for (s32 tile_y = 0; tile_y < gs->world.screen_dim_in_tiles.y; tile_y +=1) {
      for (s32 tile_x = 0; tile_x < gs->world.screen_dim_in_tiles.x; tile_x +=1) {

        iv2 tile_pos = iv2m(
            tile_x + screen_offset.x*gs->world.screen_dim_in_tiles.x,
            tile_y + screen_offset.y*gs->world.screen_dim_in_tiles.y
      );
        //World_Position wp = chunk_pos_from_tile_pos(gs->world.w, tile_pos);
        //wp = canonicalize_position(gs->world.w, tile_pos);

        Tile_Value tval = TILE_EMPTY;
        if ((tile_x == 0) && !(door_left && (tile_y == gs->world.screen_dim_in_tiles.y/2))) {
            tval = TILE_WALL;
        }
        if ((tile_y == 0) && !(door_down && (tile_x == gs->world.screen_dim_in_tiles.x/2))) {
            tval = TILE_WALL;
        }

        if ((tile_x == gs->world.screen_dim_in_tiles.x-1) && !(door_right && (tile_y == gs->world.screen_dim_in_tiles.y/2))) {
            tval = TILE_WALL;
        }
        if ((tile_y == gs->world.screen_dim_in_tiles.y-1) && !(door_up && (tile_x == gs->world.screen_dim_in_tiles.x/2))) {
            tval = TILE_WALL;
        }

        if (tval == TILE_WALL) {
          add_wall(gs, tile_pos);
        }

      }
    }
    door_left = door_right; // carry-over from prev room
    door_down = door_up; // carry-over from prev room
    door_right = false;
    door_up = false;

    if ( random_value == 0) {
      screen_offset.x+=1;
    } else {
      screen_offset.y+=1;
    }
  }

  gs->player_low_entity_idx = add_player(gs).entity_idx;
  add_monster(gs, iv2m(4,4));
  add_familiar(gs, iv2m(2,4));
}

void game_update(Game_State *gs, float dt) {
  // TODO: Think about this, especially when we have persistent state (Panels) 
  static bool gui_initialized = false;
  if (!gui_initialized) {
    gui_context_init(gs->frame_arena, &gs->font);
    gui_initialized = true;
  }

  gs->game_viewport = rec(0,0,gs->screen_dim.x, gs->screen_dim.y);
  gs->world.lower_left_corner = v2m(0, gs->screen_dim.y);

  /*
  // Audio test
  u32 sample_rate = 44100; 
  u32 num_seconds = 4;
  u32 num_channels = 2;

  u32 num_samples = sample_rate * num_channels * num_seconds;
  s32* wdata = arena_push_array(gs->frame_arena, s32, num_samples);

  s32 sample_value1 = 0;
  s32 sample_value2 = 0;
  for (u32 i = 0; i < num_samples; i+=2) {
    sample_value1 += 8000000;
    sample_value2 += 1200000;
    wdata[i] = sample_value1;
    wdata[i+1] = sample_value2;
  }

  assert(write_sample_wav_file("build/hello.wav", wdata, num_samples * sizeof(wdata[0]), num_channels, sample_rate, sizeof(wdata[0])*8)); 
  */

  // Another audio test -- real time (!!)
  // TODO: This is enough backend to make an audio mixer - do it
  u64 audio_samples_req = gs->audio_out.samples_requested;
  if (audio_samples_req > 0) {
    f32 volume_mod = 0.00;
    for (u32 sample_idx = 0; sample_idx < audio_samples_req; sample_idx+=1) {
      f32 freq = 400; // Hz
      f32 s = sin_f32(2 * PI * (freq/gs->audio_out.sample_rate) * gs->audio_out.current_sine_sample);
      gs->audio_out.samples[sample_idx] = s * volume_mod;
      gs->audio_out.current_sine_sample += 1;
    }
  }

  // Set the camera position
  Entity camera = get_high_entity(gs, gs->player_low_entity_idx);
  set_camera(gs, camera.low->p);

#if 0
  // Move the player
  if (!entity_idx_is_nil(gs->player_low_entity_idx)) {

    f32 hero_speed = 15.0;
    Input *input = &gs->input;
    v2 dp = v2m(0,0);
    if (input_key_down(input, KEY_SCANCODE_UP))dp.y+=hero_speed*dt;
    if (input_key_down(input, KEY_SCANCODE_DOWN))dp.y-=hero_speed*dt;
    if (input_key_down(input, KEY_SCANCODE_LEFT))dp.x-=hero_speed*dt;
    if (input_key_down(input, KEY_SCANCODE_RIGHT))dp.x+=hero_speed*dt;


    move_entity(gs, gs->player_low_entity_idx, dp, dt);

  }
#endif

  // Update all the entities!!
  for (u32 high_entity_idx = 1; high_entity_idx < gs->high_entity_count; ++high_entity_idx) {
    Entity entity = entity_from_high_entity(gs, high_entity_idx);
    switch (entity.low->kind) {
      case ENTITY_KIND_PLAYER: {
        update_hero(gs, entity, dt);
      }break;
      case ENTITY_KIND_MONSTER: {
        update_monster(gs, entity, dt);
      }break;
      case ENTITY_KIND_FAMILIAR: {
        update_familiar(gs, entity, dt);
      }break;
      case ENTITY_KIND_WALL: {
      }break;
      case ENTITY_KIND_NIL: {
        // Nothing
      }break;
      default: break;
    }
  }
}


void game_render(Game_State *gs, float dt) {
  // Push viewport, scissor and camera (we will not change these the whole frame except in UI pass)
  R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_VIEWPORT, .r = gs->game_viewport };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_SCISSOR, .r = gs->game_viewport };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);
  //cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(gs->game_viewport.w/2.0, gs->game_viewport.h/2.0), .origin = v2m(0,0), .zoom = gs->zoom, .rot_deg = 0} };
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = (R2D_Cam){ .offset = v2m(0,0), .origin = v2m(0,0), .zoom = 1.0, .rot_deg = 0} };
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);



  // Logic to draw background, no entities!
  // Don't remove yet!
#if 0 
  s32 screens_to_draw = 3;
  iv2 screen_offset = iv2m(gs->world.screen_dim_in_tiles.x*screens_to_draw/2, gs->world.screen_dim_in_tiles.y*screens_to_draw/2);
  if (!entity_idx_is_nil(gs->player_low_entity_idx)) {
    // Draw The backgound
    for (s32 row = -screen_offset.y; row < screen_offset.y; row+=1) {
      for (s32 col = -screen_offset.x; col < screen_offset.x; col+=1) {
        World_Position tile_pos = (World_Position){
          .abs_coords = iv2m(col+gs->world.camera_p.abs_coords.x , row+gs->world.camera_p.abs_coords.y),
          .offset = v2m(0,0),
        };
        tile_pos = canonicalize_position(gs->world.w, tile_pos);

        World_Chunk_Position chunk_tile_pos = get_chunk_pos(gs->world.w, tile_pos.abs_coords);
        World_Chunk *chunk = get_world_chunk_arena(gs->world.w, chunk_tile_pos.chunk_coords, nullptr);
        Tile_Value value = get_tile_value(gs->world.w, chunk, chunk_tile_pos.chunk_rel);

        if (value == TILE_UNINITIALIZED) { // uninitialized (nothing)
          rend_push_tile(gs, 68, tile_pos, gs->world.camera_p, gs->world.w->tile_dim_px);
        } else if (value == TILE_EMPTY) { // empty (grass)
          rend_push_tile(gs, 69, tile_pos, gs->world.camera_p, gs->world.w->tile_dim_px);
        } else { // blocker (wall)
          rend_push_tile(gs, 1, tile_pos, gs->world.camera_p, gs->world.w->tile_dim_px);
        }
      }
    }
  }
#endif

  // Render all the entities
  for (u32 low_entity_idx = 1; low_entity_idx < gs->low_entity_count; ++low_entity_idx) {
    Low_Entity *low = get_low_entity(gs, low_entity_idx);
    // If its the player also render the tile vizualization thingy
    if (low_entity_idx == gs->player_low_entity_idx) {
      World_Position tile_wp = chunk_pos_from_tile_pos(gs->world.w, iv2m(
            (low->p.chunk.x * gs->world.w->tiles_per_chunk + low->p.offset.x / gs->world.w->tile_dim_meters.x + gs->world.w->tiles_per_chunk/2),
            (low->p.chunk.y * gs->world.w->tiles_per_chunk + low->p.offset.y / gs->world.w->tile_dim_meters.y + gs->world.w->tiles_per_chunk/2)
      ));
      rend_push_tile(gs, 98, tile_wp, gs->world.camera_p, gs->world.w->tile_dim_px);
    }

    // Render the enitites (for players, entity 
    // position is actually the halfway-x bottom-y point,
    // so we fixup a bit)
    switch (low->kind) {
      case ENTITY_KIND_FAMILIAR:
      case ENTITY_KIND_MONSTER:
      case ENTITY_KIND_PLAYER: {
        World_Position pp_fixup = low->p;
        pp_fixup.offset.x -= low->dim_meters.x/2;
        rend_push_tile_alpha(gs, 3+low->kind, pp_fixup, gs->world.camera_p, v2_mult(v2_div(gs->world.w->tile_dim_px, gs->world.w->tile_dim_meters), low->dim_meters), 0.5 + 0.5 * (low->high_entity_idx > 0));
      }break;
      case ENTITY_KIND_WALL: {
        rend_push_tile_alpha(gs, 1, low->p, gs->world.camera_p, v2_mult(v2_div(gs->world.w->tile_dim_px, gs->world.w->tile_dim_meters), low->dim_meters), 0.5 + 0.5 * (low->high_entity_idx > 0));
      }break;
      case ENTITY_KIND_NIL: {
        // Nothing
      }break;
      default: break;
    }
  }

  // ..
  // ..
  // In the end, perform a UI pass (TBA)
  // Right now: Print debug player info stuff
  gui_frame_begin(gs->screen_dim, &gs->input, &gs->cmd_list, dt);

	gui_set_next_child_layout_axis(GUI_AXIS_X);
  gui_set_next_pref_height((Gui_Size){GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  gui_set_next_pref_width((Gui_Size){GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  Gui_Signal debug_pane = gui_pane(MAKE_STR("Main_Pane"));
  gui_push_parent(debug_pane.box);
  {
    gui_set_next_bg_color(col(0.1, 0.2, 0.4, 0.5));
    gui_set_next_text_alignment(GUI_TEXT_ALIGNMENT_CENTER);
    gui_set_next_pref_width((Gui_Size){GUI_SIZE_KIND_TEXT_CONTENT, 10.0, 1.0});
    gui_set_next_text_color(col(0.7, 0.8, 0.1, 0.9));
    Low_Entity *low = get_low_entity(gs, gs->player_low_entity_idx);
    buf player_info = arena_sprintf(gs->frame_arena, "low_p: chunk(%d,%d), offset(%f, %f)", low->p.chunk.x, low->p.chunk.y, low->p.offset.x, low->p.offset.y);
    gui_label(player_info);
  }
  gui_pop_parent();

	gui_set_next_child_layout_axis(GUI_AXIS_X);
  gui_set_next_pref_height((Gui_Size){GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  gui_set_next_pref_width((Gui_Size){GUI_SIZE_KIND_PARENT_PCT, 1.0, 1.0});
  Gui_Signal debug_pane2 = gui_pane(MAKE_STR("Main_Pane2"));
  {
    gui_push_parent(debug_pane2.box);
    gui_set_next_bg_color(col(0.3, 0.1, 0.7, 0.5));
    gui_set_next_text_color(col(0.7, 0.8, 0.1, 0.9));
    gui_set_next_text_alignment(GUI_TEXT_ALIGNMENT_CENTER);
    gui_set_next_pref_width((Gui_Size){GUI_SIZE_KIND_TEXT_CONTENT, 10.0, 1.0});
    buf high_entity_info = arena_sprintf(gs->frame_arena, "high entity count: %d", gs->high_entity_count);
    gui_label(high_entity_info);
  }
  gui_pop_parent();
  //--------------
  gui_frame_end();
}

void game_shutdown(Game_State *gs) {
  // This COULD be used for the persistent
  // GUI stuff outlined in game_update(!!)
}

