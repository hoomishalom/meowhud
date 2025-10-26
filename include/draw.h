#ifndef DRAW_H_INCLUDED
#define DRAW_H_INCLUDED

#include <uchar.h>
#include <string.h>
#include <stdio.h>

#include <fcft/fcft.h>
#include <wayland-client.h>

#include "types.h"

// puts the text, formatted on pix_img
void render_chars(const char32_t *text, pixman_image_t *result, size_t text_len,
                  int *x, int y, int surface_width, bool anchor_right,
                  pixman_image_t *color, struct fcft_font *font);


// puts content_rows formatted on pix_img 
void render_rows(MeowhudState *state);
 
// puts bg on pix_img 
void render_bg(MeowhudState *state);

// draws pix_img to the surface
void draw_hud(MeowhudState *state);

#endif
