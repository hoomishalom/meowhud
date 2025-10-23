#ifndef DRAW_H_INCLUDED
#define DRAW_H_INCLUDED

#include <fcft/fcft.h>
#include <wayland-client.h>

#include <uchar.h>

#include "types.h"

void render_bar(MeowhudState *);


static void render_glyphs(
  pixman_image_t *result,
  int *x,
  const int *y,
  bool anchor_right,
  pixman_image_t *color,
  size_t count,
  struct fcft_font *font,
  const struct fcft_glyph *glyphs[]
);


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
);

#endif
