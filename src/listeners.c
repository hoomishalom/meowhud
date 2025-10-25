#include "../include/listeners.h"
#include "../include/meowhud.h"

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
  MeowhudState *state = data;

  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 6);
  } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0){
    state->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 4);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
  } 

  // printf("interface: '%s', version: %d, name: %d\n", interface, version, name);
}


static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
  // This space deliberately left blank
}


const struct wl_registry_listener registry_listener = {
  .global = registry_handle_global,
  .global_remove = registry_handle_global_remove,
};

static void handle_configure(void *data,
                             struct zwlr_layer_surface_v1 *layer_surface,
                             uint32_t serial, uint32_t width, uint32_t height) {
  MeowhudState *state = data;

  assert(width == (state->width));
  assert(height == (state->height));

  state->configured = true;
  zwlr_layer_surface_v1_ack_configure(layer_surface, serial);
}

static void handle_closed(void *data, struct zwlr_layer_surface_v1 *layer_surface) {
}

const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
  .configure = handle_configure,
  .closed = handle_closed,
};

static void handle_release(void *data, struct wl_buffer *buffer) {
  MeowhudState *state = data;
}

const struct wl_buffer_listener buffer_listener = {
  .release = handle_release,
};
