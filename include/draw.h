#ifndef DRAW_H_INCLUDED
#define DRAW_H_INCLUDED

#include <uchar.h>
#include <stdbool.h>
#include <stddef.h>

#include <fcft/fcft.h>
#include "types.h"

// puts the text, formatted on pix_img
void render_chars(const char32_t *text, pixman_image_t *result, size_t text_len,
                  int *x, int y, int surface_width, bool anchor_right,
                  pixman_image_t *color, struct fcft_font *font);


// puts content_rows formatted on pix_img 
void render_rows(OutputState *hud);
 
// puts bg on pix_img 
void render_bg(OutputState *hud);

// draws pix_img to the surface
void draw_hud(OutputState *hud);

#endif
