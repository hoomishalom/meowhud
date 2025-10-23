#ifndef DRAW_H_INCLUDED
#define DRAW_H_INCLUDED

#include <fcft/fcft.h>
#include <wayland-client.h>

#include <uchar.h>

#include "types.h"

void render_bar(MeowhudState *);

void render_chars(const char32_t *, pixman_image_t *, size_t, int, int,
                  pixman_image_t *, struct fcft_font *);

#endif
