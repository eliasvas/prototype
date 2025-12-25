#include "game.h"
#include "gui/gui.h"

// For random (maybe we should make our own though..)
#include "stdlib.h"


void rend_push_tile(Game_State *gs, u32 atlas_idx, Tile_Map_Position tile, Tile_Map_Position camera, v2 dim_px) {
  // Make a Tile_Map_Position w/ the needed coordinates relative to 'camera' Tile_Map_Position
  Tile_Map_Position relative_tp = tile;
  relative_tp.abs_tile_coords.x -= camera.abs_tile_coords.x;
  relative_tp.abs_tile_coords.y -= camera.abs_tile_coords.y;
  relative_tp.tile_rel_coords.x -= camera.tile_rel_coords.x;
  relative_tp.tile_rel_coords.y -= camera.tile_rel_coords.y;
  relative_tp = canonicalize_position(gs->world.tm, relative_tp);

  // Calculate on-screen coords
  v2 m2p = v2_div(gs->world.tm->tile_dim_px, gs->world.tm->tile_dim_meters);

  v2 pos_meters = get_tilemap_fpos_in_meters(gs->world.tm, tile);
  v2 camera_meters = get_tilemap_fpos_in_meters(gs->world.tm, camera);
  v2 relative_pos_px = v2_mult(v2_sub(pos_meters, camera_meters), m2p);

#if 0
  v2 tile_dim_px = gs->world.tm->tile_dim_px; 
  s32 chunk_dim = gs->world.tm->chunk_dim; 
  Tile_Chunk_Position chunk_pos = get_chunk_pos(gs->world.tm, relative_tp.abs_tile_coords);
  rect dst = rec(
      chunk_pos.chunk_coords.x * chunk_dim * tile_dim_px.x + chunk_pos.chunk_rel.x * tile_dim_px.x + m2p.x*relative_tp.tile_rel_coords.x, 
      chunk_pos.chunk_coords.y * chunk_dim * tile_dim_px.y + chunk_pos.chunk_rel.y * tile_dim_px.y + m2p.y*relative_tp.tile_rel_coords.y, 
      dim_px.x, dim_px.y
  );
#else
  rect dst = rec(
      relative_pos_px.x, relative_pos_px.y,
      dim_px.x, dim_px.y
  );
#endif

  // Bonus: Modify rectangle so that 'camera' Tile_Map_Position is at the center of the screen
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
      .c = col(1,1,1,1),
      .tex = gs->atlas,
      .rot_deg = 0,
  };

  R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_ADD_QUAD, .q = quad};
  r2d_push_cmd(gs->frame_arena, &gs->cmd_list, cmd, 256);

}


// Entity index zero is always invalid
b32 entity_idx_is_nil(u64 entity_idx) {
  return (entity_idx == 0);
}

void change_entity_residence(Game_State *gs, u32 entity_idx, Entity_Residence residence) {
  Dormant_Entity camera_dormant = gs->dormant_entities[gs->camera_following_entity_index];
  if (residence == RESIDENCE_HIGH) {
    if (gs->entity_residence[entity_idx] != RESIDENCE_HIGH) {
      Dormant_Entity *dormant = &gs->dormant_entities[entity_idx];
      High_Entity *high = &gs->high_entities[entity_idx];
      Tile_Map_Position entity_pos = dormant->p;
      Tile_Map_Position camera_pos = camera_dormant.p;

      v2 camera_relative_pos = v2m(0,0);
      high->p = camera_relative_pos;

      // We find the relative position for the entity and store it in the high_entity array 
      v2 entity_fpos = get_tilemap_fpos_in_meters(gs->world.tm, entity_pos);
      v2 camera_fpos = get_tilemap_fpos_in_meters(gs->world.tm, camera_pos);
      v2 relative_fpos = v2_sub(camera_fpos, entity_fpos);
      high->p = relative_fpos;

    }

  }

  gs->entity_residence[entity_idx] = residence;
}


Entity get_entity(Game_State *gs, Entity_Residence residence, u64 entity_idx) {
  Entity e = {};

  if ((entity_idx > 0) && (entity_idx < gs->entity_count)) {
    e.low = &gs->low_entities[entity_idx];
    e.high = &gs->high_entities[entity_idx];
    e.dormant = &gs->dormant_entities[entity_idx];
    e.residence = RESIDENCE_DORMANT;
  }
  gs->entity_residence[entity_idx] = RESIDENCE_DORMANT;
  change_entity_residence(gs, entity_idx, residence);

  // For now..
  assert(!entity_idx_is_nil(entity_idx));

  return e;
}

u64 add_entity(Game_State *gs) {
  u64 entity_idx = 0;
  // Make the nil entity first
  if (gs->entity_count == 0) {
    gs->high_entities[gs->entity_count] = (High_Entity){};
    gs->low_entities[gs->entity_count] = (Low_Entity){};
    gs->dormant_entities[gs->entity_count] = (Dormant_Entity){};
    gs->entity_count += 1;
  }
  // Add the actual entity
  entity_idx = gs->entity_count; 

  gs->high_entities[gs->entity_count] = (High_Entity){};
  gs->low_entities[gs->entity_count] = (Low_Entity){};
  gs->dormant_entities[gs->entity_count] = (Dormant_Entity){};
  gs->entity_count += 1;

  return entity_idx;
}
static Tile_Map_Position map_fpos_to_tile_map_position(Tile_Map *tm, Tile_Map_Position pos, v2 offset_fpos) {
  Tile_Map_Position base = pos;
  base.tile_rel_coords = v2_add(base.tile_rel_coords, offset_fpos);

  base = canonicalize_position(tm, base);
  return base;
}

void initialize_player(Game_State *gs, u64 entity_idx) {
  Entity player_entity = get_entity(gs, RESIDENCE_HIGH, entity_idx);


  // Should this be a nil entity and we check??
  *(player_entity.dormant) = (Dormant_Entity) {
    .exists = true,
    .p = (Tile_Map_Position){
      .abs_tile_coords = iv2m(2,2),
      .tile_rel_coords = v2m(0,0),
    },
    .dim_meters = v2_multf(gs->world.tm->tile_dim_meters, 0.75),
  };
  // TODO: we could also set camerafollowingentityindex ehre
}

void move_player(Game_State *gs, u64 player_entity_idx, f32 dt) {
  Entity entity = get_entity(gs, RESIDENCE_HIGH, player_entity_idx);
  Entity camera = get_entity(gs, RESIDENCE_HIGH, gs->camera_following_entity_index);

  if (entity.residence != RESIDENCE_NONE) {
    // Move + Draw the hero
    f32 hero_speed = 15.0;
    Input *input = &gs->input;
    v2 player_dp = v2m(0,0);
    if (input_key_down(input, KEY_SCANCODE_UP))player_dp.y+=hero_speed*dt;
    if (input_key_down(input, KEY_SCANCODE_DOWN))player_dp.y-=hero_speed*dt;
    if (input_key_down(input, KEY_SCANCODE_LEFT))player_dp.x-=hero_speed*dt;
    if (input_key_down(input, KEY_SCANCODE_RIGHT))player_dp.x+=hero_speed*dt;

#if 0
    v2 new_player_pos = v2m(entity.dormant->p.tile_rel_coords.x, entity.dormant->p.tile_rel_coords.y);
    // TODO: entity collision code
    Tile_Map_Position new_player_cpos = entity.dormant->p;
    new_player_cpos.tile_rel_coords = new_player_pos;

    Tile_Map_Position new_player_cpos_left = new_player_cpos;
    // we subtract pixels here!!
    new_player_cpos_left.tile_rel_coords.x -= entity.dormant->dim_meters.x/2;

    Tile_Map_Position new_player_cpos_right = new_player_cpos;
    // we subtract pixels here!!
    new_player_cpos_right.tile_rel_coords.x += entity.dormant->dim_meters.x/2;
#endif



#if 0
    if (is_tile_map_point_empty(gs->world.tm, new_player_cpos) && 
        is_tile_map_point_empty(gs->world.tm, new_player_cpos_left) &&
        is_tile_map_point_empty(gs->world.tm, new_player_cpos_right)) {
#else
      if (true) {
#endif
      entity.dormant->p = map_fpos_to_tile_map_position(gs->world.tm, camera.dormant->p, player_dp);

      //Tile_Chunk_Position cp = get_chunk_pos(gs->world.tm, canonicalize_position(gs->world.tm, new_player_cpos_right).abs_tile_coords);
      //printf("chunk %d %d\n", cp.chunk_coords.x, cp.chunk_coords.y);
    }
  }

}

void game_init(Game_State *gs) {
  // Initialize the gui
  gui_context_init(gs->frame_arena, &gs->font);

  u32 chunk_shift = 4;
  // Initialize the Tile_Map
  gs->world.screen_dim_in_tiles = iv2m(9,7);
  gs->world.tm = arena_push_struct(gs->persistent_arena, Tile_Map);
  *(gs->world.tm) = (Tile_Map) {
      .chunk_dim = chunk_shift*chunk_shift,
      .chunk_shift = chunk_shift,
      .chunk_mask = ((1 << chunk_shift) - 1),
      .tile_chunk_count = iv2m(128,128),
      .tile_dim_px = v2m(32,32),
      .tile_dim_meters = v2m(1.5,1.5),
  };
  gs->world.tm->chunks = arena_push_array(gs->persistent_arena, Tile_Chunk, gs->world.tm->tile_chunk_count.x* gs->world.tm->tile_chunk_count.y);

  
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
        Tile_Map_Position tile_pos = (Tile_Map_Position){
          .abs_tile_coords = iv2m(tile_x + screen_offset.x*gs->world.screen_dim_in_tiles.x, tile_y + screen_offset.y*gs->world.screen_dim_in_tiles.y),
          .tile_rel_coords = v2m(0,0),
        };
        tile_pos = canonicalize_position(gs->world.tm, tile_pos);

        Tile_Chunk *chunk = get_tile_chunk(gs->world.tm, get_chunk_pos(gs->world.tm, tile_pos.abs_tile_coords).chunk_coords);
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

        //Tile_Chunk_Position cp = get_chunk_pos(gs->world.tm, tile_pos.abs_tile_coords);
        set_or_alloc_tile_value(gs->persistent_arena, gs->world.tm, chunk, get_chunk_pos(gs->world.tm, tile_pos.abs_tile_coords).chunk_rel, tval);
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

  gs->player_entity_idx = add_entity(gs);
  initialize_player(gs, gs->player_entity_idx);
  gs->camera_following_entity_index = gs->player_entity_idx;
}

void game_update(Game_State *gs, float dt) {
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



  // A bit hacky.. you have to zoom in/ou with '-' and '='
  s32 screens_to_draw = 2;
  iv2 screen_offset = iv2m(gs->world.screen_dim_in_tiles.x*screens_to_draw/2, gs->world.screen_dim_in_tiles.y*screens_to_draw/2);

  if (!entity_idx_is_nil(gs->player_entity_idx)) {

    move_player(gs, gs->player_entity_idx, dt);

    Entity camera = get_entity(gs, RESIDENCE_HIGH, gs->camera_following_entity_index);

    // Draw The backgound
    for (s32 row = -screen_offset.y; row < screen_offset.y; row+=1) {
      for (s32 col = -screen_offset.x; col < screen_offset.x; col+=1) {
        Tile_Map_Position tile_pos = (Tile_Map_Position){
          .abs_tile_coords = iv2m(col+camera.dormant->p.abs_tile_coords.x , row+camera.dormant->p.abs_tile_coords.y),
          .tile_rel_coords = v2m(0,0),
        };
        tile_pos = canonicalize_position(gs->world.tm, tile_pos);

        Tile_Chunk_Position chunk_tile_pos = get_chunk_pos(gs->world.tm, tile_pos.abs_tile_coords);
        Tile_Chunk *chunk = get_tile_chunk(gs->world.tm, chunk_tile_pos.chunk_coords);
        Tile_Value value = get_tile_value(gs->world.tm, chunk, chunk_tile_pos.chunk_rel);

        if (value == TILE_UNINITIALIZED) { // uninitialized (nothing)
          rend_push_tile(gs, 68, tile_pos, camera.dormant->p, gs->world.tm->tile_dim_px);
        } else if (value == TILE_EMPTY) { // empty (grass)
          rend_push_tile(gs, 69, tile_pos, camera.dormant->p, gs->world.tm->tile_dim_px);
        } else { // blocker (wall)
          rend_push_tile(gs, 1, tile_pos, camera.dormant->p, gs->world.tm->tile_dim_px);
        }
      }
    }

    /*
    Entity player = get_entity(gs, RESIDENCE_HIGH, gs->player_entity_idx);
    // Render 'player tile visualization thing'
    rend_push_tile(gs, 98, (Tile_Map_Position){.abs_tile_coords = player.dormant->p.abs_tile_coords}, player.dormant->p, gs->world.tm->tile_dim_px);

    // Render the enitites (entity position is actually the halfway-x bottom-y point, so we fixup a bit)
    Tile_Map_Position pp_fixup = player.dormant->p;
    pp_fixup.tile_rel_coords.x += (gs->world.tm->tile_dim_meters.x/2 - player.dormant->dim_meters.x/2);
    pp_fixup.tile_rel_coords.y += gs->world.tm->tile_dim_meters.y/2;
    rend_push_tile(gs, 9, pp_fixup, player.dormant->p, v2_mult(v2_div(gs->world.tm->tile_dim_px, gs->world.tm->tile_dim_meters), player.dormant->dim_meters));
    */
  }

  // Render all the entities
  for (u32 entity_idx = 1; entity_idx < gs->entity_count; ++entity_idx) {
    Entity entity = get_entity(gs, RESIDENCE_HIGH, entity_idx);
    if (gs->entity_residence[entity_idx] != RESIDENCE_NONE) {
      Entity camera = get_entity(gs, RESIDENCE_HIGH, gs->camera_following_entity_index);

      // If its the player also render the tile vizualization thingy
      if (entity_idx ==  gs->player_entity_idx) {
        rend_push_tile(gs, 98, (Tile_Map_Position){.abs_tile_coords = entity.dormant->p.abs_tile_coords}, camera.dormant->p, gs->world.tm->tile_dim_px);
      }

      // Render the enitites (entity position is actually the halfway-x bottom-y point, so we fixup a bit)
      Tile_Map_Position pp_fixup = entity.dormant->p;
      pp_fixup.tile_rel_coords.x += (gs->world.tm->tile_dim_meters.x/2 - entity.dormant->dim_meters.x/2);
      pp_fixup.tile_rel_coords.y += gs->world.tm->tile_dim_meters.y/2;
      rend_push_tile(gs, 9, pp_fixup, camera.dormant->p, v2_mult(v2_div(gs->world.tm->tile_dim_px, gs->world.tm->tile_dim_meters), entity.dormant->dim_meters));
    }
  }

  // ..
  // ..
  // In the end, perform a UI pass (TBA)
  gui_frame_begin(gs->screen_dim, &gs->input, &gs->cmd_list, dt);
  gui_set_next_bg_color(col(0.1, 0.2, 0.4, 0.5));
  Gui_Signal bs = gui_button(MAKE_STR("Exit"));
  if (bs.flags & GUI_SIGNAL_FLAG_LMB_PRESSED)printf("Exit pressed!\n");
  gui_frame_end();
}

void game_shutdown(Game_State *gs) { }
