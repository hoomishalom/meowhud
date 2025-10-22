#ifndef LISTENERS_H_INCLUDED
#define LISTENERS_H_INCLUDED

#include <wlr/types/wlr_layer_shell_v1.h>
#include <assert.h>

extern const struct wl_registry_listener registry_listener;
extern const struct zwlr_layer_surface_v1_listener layer_surface_listener;
extern const struct wl_buffer_listener buffer_listener;

#endif
