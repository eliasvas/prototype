#ifndef _FONT_UTIL_H__
#define _FONT_UTIL_H__
#include "base/base_inc.h"
#include "core/ogl.h" // TODO: This should GO
#include "r2d.h"

// TODO: LOD stuff and our own lookup data structure (Glyph_Cache?)
// TODO: SDF font support
// TODO: Maybe put the implementation in the C file?!?!
//
typedef struct {
  rect r;
  rect tc;
  v2 off;
  f32 xadvance;
}Glyph_Info;

// TODO: Maybe we can use a stack allocator for the permanent arena, so the Font_Info's glyphs array can be allocated there (in the end)
typedef struct {
  Glyph_Info glyphs[200];
  u32 first_codepoint;
  u32 last_codepoint;
  u32 glyph_count;

  Ogl_Tex atlas;
}Font_Info;

Font_Info font_util_load_default_atlas(Arena *arena, u32 glyph_height_in_px, u32 atlas_width, u32 atlas_height);

static void font_util_flip_bitmap(u8 *bitmap, u32 width, u32 height) {
  for (u32 y = 0; y < height/2; ++y) {
    for (u32 x = 0; x < width; ++x) {
      u8 temp = bitmap[x + y * width];
      bitmap[x+y*width] = bitmap[x+(height-y)*width];
      bitmap[x+(height-y)*width] = temp;
    }
  }
}

static rect font_util_calc_text_rect(Font_Info *font_info, buf text, v2 baseline_pos, f32 scale) {
  u32 glyph_count = text.count;
  if (glyph_count == 0) return (rect){};

  Glyph_Info first_glyph = font_info->glyphs[text.data[0] - font_info->first_codepoint];
  rect r = (rect) {
    .x = first_glyph.off.x*scale +baseline_pos.x,
    .y = first_glyph.off.y*scale +baseline_pos.y,
    .w = first_glyph.r.w*scale,
    .h = first_glyph.r.h*scale,
  };

  for (u32 glyph_idx = 0; glyph_idx < glyph_count; ++glyph_idx) {
    Glyph_Info glyph = font_info->glyphs[text.data[glyph_idx] - font_info->first_codepoint];
    rect r1 = (rect) {
      .x = glyph.off.x*scale + baseline_pos.x,
      .y = glyph.off.y*scale + baseline_pos.y,
      .w = glyph.r.w*scale,
      .h = glyph.r.h*scale,
    };
    baseline_pos.x += glyph.xadvance*scale;
    r = rect_calc_bounding_rect(r, r1);
  }

  return r;
}

static f32 font_util_measure_text_width(Font_Info *font_info, buf text, f32 scale) {
  return font_util_calc_text_rect(font_info, text, v2m(0,0), scale).w;
}

static f32 font_util_measure_text_height(Font_Info *font_info, buf text, f32 scale) {
  return font_util_calc_text_rect(font_info, text, v2m(0,0), scale).h;
}

static s64 font_util_count_glyphs_until_width(Font_Info *font_info, buf text, f32 scale, f32 target_width) {
  s64 glyph_count = 0;
  while (glyph_count < text.count) {
    f32 text_w = font_util_measure_text_width(font_info, buf_make(text.data, glyph_count), scale);
    if (text_w >= target_width) {
      if (glyph_count > 0) glyph_count -= 1;
      break;
    } else {
      glyph_count+=1;
    }
  }
  return glyph_count;
}

static void font_util_debug_draw_text(Font_Info *font_info, Arena *arena, R2D_Cmd_Chunk_List *cmd_list, rect viewport, rect clip_rect, buf text, v2 baseline_pos, f32 scale, color col, bool draw_box) {

  // set viewport 
  R2D_Cmd cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_VIEWPORT, .r = viewport };
  r2d_push_cmd(arena, cmd_list, cmd, 256);
  // set scissor
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_SCISSOR, .r = clip_rect};
  r2d_push_cmd(arena, cmd_list, cmd, 256);
  // set camera
  R2D_Cam cam = (R2D_Cam){ .offset = v2m(0,0), .origin = v2m(0,0), .zoom = 1.0, .rot_deg = 0.0, };
  cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_SET_CAMERA, .c = cam };
  r2d_push_cmd(arena, cmd_list, cmd, 256);
  // push quad


  //R2D* text_rend = r2d_begin(arena, &(R2D_Cam){ .offset = v2m(0, 0), .origin = v2m(0,0), .zoom = 1.0, .rot_deg = 0.0, }, viewport, clip_rect);

  rect tr = font_util_calc_text_rect(font_info, text, baseline_pos, scale);
  if (draw_box) {
    //r2d_push_quad(text_rend, (R2D_Quad) { .dst_rect = tr, .c = col(0.9,0.4,0.4,1), });
    R2D_Quad quad = (R2D_Quad) {
        .dst_rect = tr,
        .c = col(0.9,0.4,0.4,1),
    };
    cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_ADD_QUAD, .q = quad};
    r2d_push_cmd(arena, cmd_list, cmd, 256);
  }

  for (u32 i = 0; i < text.count; ++i) {
    u8 c = text.data[i];
    Glyph_Info metrics = font_info->glyphs[c - font_info->first_codepoint];
    //r2d_push_quad(text_rend, (R2D_Quad) { .dst_rect = rec(baseline_pos.x+metrics.off.x*scale, baseline_pos.y+metrics.off.y*scale, metrics.r.w*scale, metrics.r.h*scale), .src_rect = rec(metrics.r.x, metrics.r.y, metrics.r.w, metrics.r.h), .c = col, .tex = font_info->atlas, });
    f32 atlas_height = font_info->atlas.height;
    R2D_Quad quad = (R2D_Quad) {
        .dst_rect = rec(baseline_pos.x+metrics.off.x*scale, baseline_pos.y+metrics.off.y*scale, metrics.r.w*scale, metrics.r.h*scale),
        .src_rect = rec(metrics.r.x, atlas_height - metrics.r.y - metrics.r.h, metrics.r.w, metrics.r.h),
        .c = col,
        .tex = font_info->atlas,
    };
    cmd = (R2D_Cmd){ .kind = R2D_CMD_KIND_ADD_QUAD, .q = quad};
    r2d_push_cmd(arena, cmd_list, cmd, 256);

    baseline_pos.x += metrics.xadvance*scale;
  }

  //r2d_end(text_rend);
}


#endif
