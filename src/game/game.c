#include "game.h"
#include "gui/gui.h"
#include "sim_region.h"

// For random (maybe we should make our own though..)
#include "stdlib.h"


void rend_push_tile_alpha(Game_State *gs, u32 atlas_idx, World_Position tile, World_Position camera, v2 dim_px, f32 alpha) {
  // Make a World_Position w/ the needed coordinates relative to 'camera' World_Position

  // Calculate on-screen coords
  v2 m2p = v2_div(gs->gworld.w->tile_dim_px, gs->gworld.w->tile_dim_meters);

  v2 pos_meters = get_world_fpos_in_meters(gs->gworld.w, tile);
  v2 camera_meters = get_world_fpos_in_meters(gs->gworld.w, camera);
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
  iv2 llc = iv2m(gs->gworld.lower_left_corner.x,gs->gworld.lower_left_corner.y);
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


static World_Position map_fpos_to_tile_map_position(World *w, World_Position pos, v2 offset_fpos) {
  World_Position base = pos;
  base.offset = v2_add(base.offset, offset_fpos);

  base = canonicalize_position(w, base);
  return base;
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


u64 add_entity(Game_State *gs, Entity_Kind kind) {
  u64 low_entity_idx = 0;
  // Make the nil entity first
  if (gs->low_entity_count == 0) {
    gs->low_entities[gs->low_entity_count] = (Low_Entity){};

    gs->low_entity_count += 1;
  }
  // Add the actual entity
  low_entity_idx = gs->low_entity_count; 

  gs->low_entities[low_entity_idx] = (Low_Entity){
    .sim.kind = kind,
  };
  gs->low_entity_count += 1;
  assert(gs->low_entity_count < array_count(gs->low_entities));

  return low_entity_idx;
}


// TODO: Add an arena just for the game (gs->game_arena) 


Low_Entity_Result add_low_entity(Game_State *gs, Entity_Kind kind, World_Position *p) {
  Low_Entity_Result res = {};
  u32 low_entity_idx = add_entity(gs, kind);
  Low_Entity *low = get_low_entity(gs, low_entity_idx);
  low->sim.kind = kind;

  if (p) {
    low->p = change_entity_location(gs->persistent_arena, gs->gworld.w, low_entity_idx, nullptr, p);
  } else { // positionless entity
    low->p = world_pos_nil(); 
  }

  res.entity_idx = low_entity_idx;
  res.low = low;

  return res;
}

Low_Entity_Result add_sword(Game_State *gs) {
  Low_Entity_Result low = add_low_entity(gs, ENTITY_KIND_SWORD, nullptr);
  low.low->sim.dim_meters = v2_multf(gs->gworld.w->tile_dim_meters, 0.5);

  return low;
}

Low_Entity_Result add_player(Game_State *gs) {
  World_Position chunk_pos = chunk_pos_from_tile_pos(gs->gworld.w, v2m(2,2));
  Low_Entity_Result low = add_low_entity(gs, ENTITY_KIND_PLAYER, &chunk_pos);
  low.low->sim.dim_meters = v2_multf(gs->gworld.w->tile_dim_meters, 0.75);
  low.low->sim.hit_point_count = 4;
  sim_entity_add_flag(&low.low->sim, ENTITY_FLAG_COLLIDES);


  // Add sword!
  Low_Entity_Result sword = add_sword(gs);
  low.low->sim.sword_ref.storage_idx = sword.entity_idx;
  make_sim_entity_non_spatial(&sword.low->sim);

  return low;
}

Low_Entity_Result add_wall(Game_State *gs, v2 tile_pos) {
  World_Position chunk_pos = chunk_pos_from_tile_pos(gs->gworld.w, tile_pos);
  Low_Entity_Result low = add_low_entity(gs, ENTITY_KIND_WALL, &chunk_pos);
  low.low->sim.dim_meters = v2_multf(gs->gworld.w->tile_dim_meters, 1.0);
  sim_entity_add_flag(&low.low->sim, ENTITY_FLAG_COLLIDES);

  return low;
}

Low_Entity_Result add_familiar(Game_State *gs, v2 tile_pos) {
  World_Position chunk_pos = chunk_pos_from_tile_pos(gs->gworld.w, tile_pos);
  Low_Entity_Result low = add_low_entity(gs, ENTITY_KIND_FAMILIAR, &chunk_pos);
  low.low->sim.dim_meters = v2_multf(gs->gworld.w->tile_dim_meters, 0.5);
  //sim_entity_add_flag(&low.low->sim, ENTITY_FLAG_COLLIDES);

  return low;
}

Low_Entity_Result add_monster(Game_State *gs, v2 tile_pos) {
  World_Position chunk_pos = chunk_pos_from_tile_pos(gs->gworld.w, tile_pos);
  Low_Entity_Result low = add_low_entity(gs, ENTITY_KIND_MONSTER, &chunk_pos);
  low.low->sim.dim_meters = v2_multf(gs->gworld.w->tile_dim_meters, 1.0);
  sim_entity_add_flag(&low.low->sim, ENTITY_FLAG_COLLIDES);

  // TODO: Fill the rest of hit point stuff :/ (ok?)
  low.low->sim.hit_point_count = 3;

  return low;
}

void move_entity(Game_State *gs, Sim_Region *region, Sim_Entity *entity, Move_Spec move_spec, f32 dt) {
  b32 collides = false;
  v2 move_vec = v2_multf(move_spec.dir, move_spec.speed * dt);
  v2 new_p = v2_add(entity->p, move_vec);
  rect bb = rec(new_p.x, new_p.y, entity->dim_meters.x, entity->dim_meters.y); 

  for (u32 entity_idx = 1; entity_idx < region->entity_count; entity_idx+=1) {
    Sim_Entity *test_entity = &region->entities[entity_idx];
    if (test_entity != entity && 
        sim_entity_is_flag_set(test_entity, ENTITY_FLAG_COLLIDES) && 
        sim_entity_is_flag_set(entity, ENTITY_FLAG_COLLIDES)) {
      v2 test_pos = test_entity->p;
      v2 test_dim = test_entity->dim_meters;
      rect test_bb = rec(test_pos.x, test_pos.y, test_dim.x, test_dim.y); 
      if (rect_isect_rect(bb, test_bb)) {
        collides = true;
        break;
      }
    }
  }

  // If there is no collision map the new entity position back to the low entity
  if (!collides) {
    entity->p = new_p;
  }

}

void update_familiar(Game_State *gs, Sim_Region *region, Sim_Entity *entity, f32 dt) {
  assert(entity->kind == ENTITY_KIND_FAMILIAR);


#if 0
  for (u32 high_entity_idx = 1; high_entity_idx < gs->high_entity_count; high_entity_idx+=1) {
    //Entity test_entity = entity_from_high_entity(gs, high_entity_idx);

    High_Entity *test_high = &gs->high_entities[high_entity_idx];
    Low_Entity *test_low = get_low_entity(gs, test_high->low_entity_idx);

    if (test_low->sim.kind == ENTITY_KIND_PLAYER) {
      //v2 distance = v2_sub(entity.high->p, test_entity.high->p);
      v2 distance = v2_sub(test_high->p, entity->p);
      if (v2_len(distance) < 100) {

        Move_Spec move_spec = (Move_Spec) {
          .dir = v2_norm(distance),
          .speed = 2.0,
          .drag = 0.0,
        };
        move_entity(gs, entity->storage_idx, move_spec, dt);
      }
    }
  }
#else
  if (entity->kind == ENTITY_KIND_FAMILIAR) {
    Move_Spec move_spec = (Move_Spec) {
      .dir = v2m(1,0),
      .speed = 2.0,
      .drag = 0.0,
    };
    move_entity(gs, region, entity, move_spec, dt);
  }
#endif
}

void update_monster(Game_State *gs, Sim_Region *region, Sim_Entity *entity, f32 dt) {
  assert(entity->kind == ENTITY_KIND_MONSTER);
  // TBA
}

void update_hero(Game_State *gs, Sim_Region *region, Sim_Entity *entity, f32 dt) {
  assert(entity->kind == ENTITY_KIND_PLAYER);

  // Move the hero
  Input *input = &gs->input;
  v2 dp = v2m(0,0);
  if (input_key_down(input, KEY_SCANCODE_UP))dp=v2m(0,1);
  if (input_key_down(input, KEY_SCANCODE_DOWN))dp=v2m(0,-1);
  if (input_key_down(input, KEY_SCANCODE_LEFT))dp=v2m(-1,0);
  if (input_key_down(input, KEY_SCANCODE_RIGHT))dp=v2m(1,0);

  Move_Spec move_spec = (Move_Spec) {
    .dir = dp,
    .speed = 15.0,
    .drag = 0.0,
  };
  move_entity(gs, region, entity, move_spec, dt);

  // Spawn sword if need be
  if (input_key_down(input, KEY_SCANCODE_W) ||
      input_key_down(input, KEY_SCANCODE_S) ||
      input_key_down(input, KEY_SCANCODE_A) ||
      input_key_down(input, KEY_SCANCODE_D) ) {
    //Low_Entity *sword_low = get_low_entity(gs, entity->sword_ref.storage_idx);
    Sim_Entity *sword_entity = entity->sword_ref.ptr;
    assert(sword_entity); // The entity will be mapped via the load_entity_ref at begin_sim
    if (sim_entity_is_flag_set(sword_entity, ENTITY_FLAG_NONSPATIAL)) {
      make_sim_entity_spatial(sword_entity, entity->p); 
      sword_entity->movement_remaining = 0.3;

      v2 dir = v2m(0,0);
      if (input_key_down(input, KEY_SCANCODE_W))dir=v2m(0,1);
      if (input_key_down(input, KEY_SCANCODE_S))dir=v2m(0,-1);
      if (input_key_down(input, KEY_SCANCODE_A))dir=v2m(-1,0);
      if (input_key_down(input, KEY_SCANCODE_D))dir=v2m(1,0);
      sword_entity->hit_dir = dir;
    }
  }

}

void update_sword(Game_State *gs, Sim_Region *region, Sim_Entity *entity, f32 dt) {
  assert(entity->kind == ENTITY_KIND_SWORD);

  Move_Spec move_spec = (Move_Spec) {
    .dir = entity->hit_dir,
    .speed = 20.0,
    .drag = 0.0,
  };

  if (entity->movement_remaining > 0) {
    entity->movement_remaining = maximum(entity->movement_remaining - dt, 0);
    move_entity(gs, region, entity, move_spec, dt);
  } else {
    make_sim_entity_non_spatial(entity); 
  }
}

void game_init(Game_State *gs) {

  // Initialize the World
  gs->gworld.screen_dim_in_tiles = iv2m(9,7);
  gs->gworld.w = arena_push_struct(gs->persistent_arena, World);
  *(gs->gworld.w) = (World) {
      .tile_dim_px = v2m(32,32),
      .tile_dim_meters = v2m(1.5, 1.5),
      .tiles_per_chunk = 16,
  };
  gs->gworld.w->chunk_dim_meters = v2_multf(gs->gworld.w->tile_dim_meters, gs->gworld.w->tiles_per_chunk);
  M_ZERO_STRUCT(gs->gworld.w->chunk_slots);

  
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

    for (s32 tile_y = 0; tile_y < gs->gworld.screen_dim_in_tiles.y; tile_y +=1) {
      for (s32 tile_x = 0; tile_x < gs->gworld.screen_dim_in_tiles.x; tile_x +=1) {

        v2 tile_pos = v2m(
            tile_x + screen_offset.x*gs->gworld.screen_dim_in_tiles.x,
            tile_y + screen_offset.y*gs->gworld.screen_dim_in_tiles.y
        );
        //World_Position wp = chunk_pos_from_tile_pos(gs->gworld.w, tile_pos);
        //wp = canonicalize_position(gs->gworld.w, tile_pos);

        Tile_Value tval = TILE_EMPTY;
        if ((tile_x == 0) && !(door_left && (tile_y == gs->gworld.screen_dim_in_tiles.y/2))) {
            tval = TILE_WALL;
        }
        if ((tile_y == 0) && !(door_down && (tile_x == gs->gworld.screen_dim_in_tiles.x/2))) {
            tval = TILE_WALL;
        }

        if ((tile_x == gs->gworld.screen_dim_in_tiles.x-1) && !(door_right && (tile_y == gs->gworld.screen_dim_in_tiles.y/2))) {
            tval = TILE_WALL;
        }
        if ((tile_y == gs->gworld.screen_dim_in_tiles.y-1) && !(door_up && (tile_x == gs->gworld.screen_dim_in_tiles.x/2))) {
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
  add_monster(gs, v2m(4,4));
  add_familiar(gs, v2m(2,4));
}

// DELETEME
static int sim_entity_count = 0;

void game_update(Game_State *gs, float dt) {
  // TODO: Think about this, especially when we have persistent state (Panels) 
  static bool gui_initialized = false;
  if (!gui_initialized) {
    gui_context_init(gs->frame_arena, &gs->font);
    gui_initialized = true;
  }

  gs->game_viewport = rec(0,0,gs->screen_dim.x, gs->screen_dim.y);
  gs->gworld.lower_left_corner = v2m(0, gs->screen_dim.y);

#if 0
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
#endif

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
  //Entity camera = get_low_entity(gs, gs->player_low_entity_idx);

  s32 screens_to_include = 1;
  v2 camera_p_in_ws = get_world_fpos_in_meters(gs->gworld.w, gs->gworld.camera_p);
  rect region_box = rec_centered(camera_p_in_ws, v2_multf(v2m(gs->gworld.screen_dim_in_tiles.x, gs->gworld.screen_dim_in_tiles.y), screens_to_include));

  World_Position player_wpos = get_low_entity(gs, gs->player_low_entity_idx)->p;
  gs->gworld.camera_p = player_wpos;

  Sim_Region *sim_region = begin_sim(gs->frame_arena, gs, gs->gworld.w, gs->gworld.camera_p, region_box);
  sim_entity_count = sim_region->entity_count;

  // Update all the entities!!
  for (u32 entity_idx = 1; entity_idx < sim_region->entity_count; entity_idx+=1) {
    Sim_Entity *entity = &sim_region->entities[entity_idx];

    switch (entity->kind) {
      case ENTITY_KIND_PLAYER: {
        update_hero(gs, sim_region, entity, dt);
      }break;
      case ENTITY_KIND_MONSTER: {
        update_monster(gs, sim_region, entity, dt);
      }break;
      case ENTITY_KIND_FAMILIAR: {
        update_familiar(gs, sim_region, entity, dt);
      }break;
      case ENTITY_KIND_WALL: {
      }break;
      case ENTITY_KIND_SWORD: {
        update_sword(gs, sim_region, entity, dt);
      }break;
      case ENTITY_KIND_NIL: {
        // Nothing
      }break;
      default: break;
    }
  }
  end_sim(sim_region, gs);

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

  // Render all the entities
  for (u32 low_entity_idx = 1; low_entity_idx < gs->low_entity_count; ++low_entity_idx) {
    Low_Entity *low = get_low_entity(gs, low_entity_idx);
    // If its the player also render the tile vizualization thingy
    if (low_entity_idx == gs->player_low_entity_idx) {
      World_Position tile_wp = chunk_pos_from_tile_pos(gs->gworld.w, v2m(
            floor_f32(low->p.chunk.x * gs->gworld.w->tiles_per_chunk + low->p.offset.x / gs->gworld.w->tile_dim_meters.x),
            floor_f32(low->p.chunk.y * gs->gworld.w->tiles_per_chunk + low->p.offset.y / gs->gworld.w->tile_dim_meters.y)
      ));
      rend_push_tile(gs, 98, tile_wp, gs->gworld.camera_p, gs->gworld.w->tile_dim_px);
    }

    // Render the enitites (for players, entity 
    // position is actually the halfway-x bottom-y point,
    // so we fixup a bit)
    v2 m2p = v2_div(gs->gworld.w->tile_dim_px, gs->gworld.w->tile_dim_meters);
    switch (low->sim.kind) {
      case ENTITY_KIND_FAMILIAR:
      case ENTITY_KIND_MONSTER:
      case ENTITY_KIND_PLAYER: {
        World_Position pp_fixup = low->p;
        pp_fixup.offset.x -= low->sim.dim_meters.x/2;
        rend_push_tile_alpha(gs, 3+low->sim.kind, pp_fixup, gs->gworld.camera_p, v2_mult(m2p, low->sim.dim_meters), 0.5 + 0.5 * (low->sim.storage_idx > 0));

        // -- Draw the HP
        for (u32 health_idx = 0; health_idx < low->sim.hit_point_count; health_idx+=1) {
          v2 hp_dim_mt = v2m(0.5, 0.5);
          World_Position hp_pos = low->p;
          hp_pos.offset.y -= 0.5;
          float hp_spacing_x = 0.6;
          assert(hp_dim_mt.x < hp_spacing_x);
          hp_pos.offset.x -= (low->sim.hit_point_count - 1) * hp_spacing_x*0.5;
          hp_pos.offset.x += health_idx * hp_spacing_x;
          hp_pos.offset.x -= hp_dim_mt.x/2; // center the hp to the middle
          hp_pos = canonicalize_position(gs->gworld.w, hp_pos);
          rend_push_tile_alpha(gs, 6+16*6, hp_pos, gs->gworld.camera_p, v2_mult(m2p, hp_dim_mt), 0.5 + 0.5 * (low->sim.storage_idx > 0));
        }
      }break;
      case ENTITY_KIND_SWORD: {
        World_Position pp_fixup = low->p;
        pp_fixup.offset.x -= low->sim.dim_meters.x/2;
        rend_push_tile_alpha(gs, 6+16*4, pp_fixup, gs->gworld.camera_p, v2_mult(m2p, low->sim.dim_meters), 0.5 + 0.5 * (low->sim.storage_idx > 0));
      }break;
      case ENTITY_KIND_WALL: {
        rend_push_tile_alpha(gs, 1, low->p, gs->gworld.camera_p, v2_mult(m2p, low->sim.dim_meters), 0.5 + 0.5 * (low->sim.storage_idx > 0));
      }break;
      case ENTITY_KIND_NIL: {
        // Nothing
      }break;
      default: break;
    }

    // reset storage_idx
    low->sim.storage_idx = 0;
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
    buf high_entity_info = arena_sprintf(gs->frame_arena, "sim entity count: %d", sim_entity_count);
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

