#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION
#include <stb/stb_truetype.h>

#include "font_util.h"

#include "base/base_inc.h"
#include "ogl.h"
#include "r2d.h"

// By default we just embed ProggyClean - Ugly AF but for now it'll do!
static const u8 default_font_data[] = {
#embed "../../data/ProggyClean.ttf"
};

Font_Info font_util_load_default_atlas(Arena *arena, u32 glyph_height_in_px, u32 atlas_width, u32 atlas_height) {
  Font_Info font = {};

  font.first_codepoint = 32; // ' ' 
  font.last_codepoint = 127; // '~'
  font.glyph_count = font.last_codepoint - font.first_codepoint + 1; 

  u8 *font_bitmap = (u8*)arena_push_array(arena, u8, sizeof(u8)*atlas_width*atlas_height);

  stbtt_packedchar *packed_chars = arena_push_array(arena, stbtt_packedchar, font.glyph_count);
  stbtt_aligned_quad *aligned_quads = arena_push_array(arena, stbtt_aligned_quad, font.glyph_count);

  // Pack all the needed glyphs to the bitmap and get their metrics (packedchar / aligned_quad)
  stbtt_pack_context pctx = {};
  stbtt_PackBegin(&pctx, font_bitmap, atlas_width, atlas_height, 0, 1, nullptr);
  stbtt_PackFontRange(&pctx, default_font_data, 0, glyph_height_in_px, font.first_codepoint, font.glyph_count, packed_chars);
  stbtt_PackEnd(&pctx);

  for (u32 glyph_idx = 0; glyph_idx < font.glyph_count; ++glyph_idx) {
    f32 trash_x, trash_y;
    stbtt_GetPackedQuad(packed_chars, atlas_width, atlas_height, glyph_idx, &trash_x, &trash_y, &aligned_quads[glyph_idx], 1);
  }

  // Calculate our internal font metrics, which we will use in-engine for font rendering
  for (u32 glyph_idx = 0; glyph_idx < font.glyph_count; ++glyph_idx) {
    stbtt_packedchar pc = packed_chars[glyph_idx];
    stbtt_aligned_quad ac = aligned_quads[glyph_idx];
    font.glyphs[glyph_idx].r = (rect){
      .x = pc.x0,
      .y = pc.y0,
      .w = pc.x1 - pc.x0,
      .h = pc.y1 - pc.y0,
    };

    // w/h = atlas_width atlas_height
    // NOTE: Not sure if this one is needed..
    font.glyphs[glyph_idx].tc = (rect){
      .x = ac.s0,
      .y = ac.t0,
      .w = ac.s1 - ac.s0,
      .h = ac.t1 - ac.t0,
    };
    font.glyphs[glyph_idx].off = v2m(pc.xoff, pc.yoff);
    font.glyphs[glyph_idx].xadvance = pc.xadvance;
  }

  // @HACK, This is because stbtt_Pack API is made to pack glyphs so the SPACE on has
  // no size, which means also no xadvance I think, for that reason we use the Font API to populate its xadvance..
  stbtt_fontinfo font_info;
  stbtt_InitFont(&font_info, default_font_data, stbtt_GetFontOffsetForIndex(default_font_data, 0));
  f32 scale = stbtt_ScaleForPixelHeight(&font_info, glyph_height_in_px);
  int advance, lsb;
  u32 space_glyph_idx = ' ' - font.first_codepoint;
  stbtt_GetCodepointHMetrics(&font_info, font.first_codepoint + space_glyph_idx, &advance, &lsb);
  font.glyphs[space_glyph_idx].xadvance = advance * scale;
 

  // Transform to OpenGL-style texture (mainly by convention, I like the upright view on renderdoc) + make the actual texture
  font_util_flip_bitmap(font_bitmap, atlas_width, atlas_height);

  // Transform to RGBA
  u8 *font_bitmap_rgba = (u8*)arena_push_array(arena, u8, sizeof(u8)*atlas_width*atlas_height*4);
  for (u32 i = 0; i < atlas_width*atlas_height; ++i) {
    font_bitmap_rgba[4*i + 0] = font_bitmap[i + 0];
    font_bitmap_rgba[4*i + 1] = font_bitmap[i + 0];
    font_bitmap_rgba[4*i + 2] = font_bitmap[i + 0];
    font_bitmap_rgba[4*i + 3] = font_bitmap[i + 0];
  }

  // Finally make the texture
  font.atlas = ogl_tex_make(font_bitmap_rgba, atlas_width, atlas_height, OGL_TEX_FORMAT_RGBA8U, (Ogl_Tex_Params){.wrap_s = OGL_TEX_WRAP_MODE_REPEAT, .wrap_t = OGL_TEX_WRAP_MODE_REPEAT});

  return font;
}

