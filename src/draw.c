#include "../include/draw.h"

void draw_bar(MeowhudState *state) {
    wl_surface_attach(state->surface, state->buff, 0, 0);
    wl_surface_damage(state->surface, 0, 0, state->height, state->width);
    wl_surface_commit(state->surface);
    wl_display_flush(state->display);
} 
