#include "../include/draw.h"

static void render_glyphs(
  pixman_image_t *result,
  int *x,
  const int *y,
  bool anchor_right,
  pixman_image_t *color,
  size_t count,
  struct fcft_font *font,
  const struct fcft_glyph *glyphs[]
)
{
  // TODO: disabled for now, will enable later when colors are supported
  // if (g->is_color_glyph) {
  //   pixman_image_composite32(
  //     PIXMAN_OP_OVER, g->pix, NULL, result, 0, 0, 0, 0,
  //     *x + g->x, *y + font->ascent - g->y, g->width, g->height);
  // } else {
  //   pixman_image_composite32(
  //     PIXMAN_OP_OVER, color, g->pix, result, 0, 0, 0, 0,
  //     *x + g->x, *y + font->ascent - g->y, g->width, g->height);
  // }
  if (anchor_right) { // places the pen at the correct spot
    size_t text_width = 0;
    for (size_t i = 0; i < count; i++) {
      text_width += glyphs[i]->advance.x;
    }

    *x -= text_width;
  } 

  for (size_t i = 0; i < count; i++) {
    const struct fcft_glyph *g = glyphs[i];
    if (g == NULL)
      continue;

    pixman_image_composite32(
      PIXMAN_OP_OVER, g->pix, NULL, result, 0, 0, 0, 0,
      *x + g->x, *y + font->ascent - g->y, g->width, g->height);
    *x += g->advance.x;
  }
}

void render_chars(
  const char32_t *text,
  pixman_image_t *result,
  size_t text_len,
  int x,
  int y,
  int width,
  bool anchor_right,
  pixman_image_t *color,
  struct fcft_font *font
)
{
  const struct fcft_glyph *glyphs[text_len];
  int text_width = 0;
  int avail_width = anchor_right ? x : (width - x);

  for (size_t i = 0; i < text_len; i++) {
    glyphs[i] = fcft_rasterize_char_utf32(font, text[i], FCFT_SUBPIXEL_DEFAULT);
    if (glyphs[i] == NULL)
      continue;

    text_width += glyphs[i]->advance.x;
    if (text_width > avail_width) {
      text_width -= glyphs[i]->advance.x;
      text_len = i;
    }
  }

  render_glyphs(result, &x, &y, anchor_right, color, text_len, font, glyphs);
}

void render_bar(MeowhudState *state) {
  wl_surface_attach(state->surface, state->buff, 0, 0);
  wl_surface_damage(state->surface, 0, 0, state->width, state->height);
  wl_surface_commit(state->surface);
  wl_display_flush(state->display);
} 
