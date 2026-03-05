#ifndef LISTENERS_H_INCLUDED
#define LISTENERS_H_INCLUDED

#include <wayland-client.h>
#include "wlr-layer-shell-unstable-v1-protocol.h"

extern const struct wl_registry_listener registry_listener;
extern const struct zwlr_layer_surface_v1_listener layer_surface_listener;
extern const struct wl_buffer_listener buffer_listener;

#endif
