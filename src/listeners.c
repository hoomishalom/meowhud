#include "../include/listeners.h"
#include "../include/types.h"
#include "../include/initializers.h"
#include "../include/utils.h"
#include <string.h>

static void output_handle_geometry(void *data, struct wl_output *wl_output,
                                   int32_t x, int32_t y, int32_t physical_width, 
                                   int32_t physical_height, int32_t subpixel,
                                   const char *make, const char *model, int32_t transform) {
  (void)data; (void)wl_output; (void)x; (void)y; (void)physical_width; (void)physical_height;
  (void)subpixel; (void)make; (void)model; (void)transform;
}

static void output_handle_mode(void *data, struct wl_output *wl_output,
                               uint32_t flags, int32_t width, int32_t height, int32_t refresh) {
  (void)refresh;
  MeowhudState *state = data;
  if (flags & WL_OUTPUT_MODE_CURRENT) {
    for (size_t i = 0; i < state->output_count; i++) {
      if (state->outputs[i].wl_output == wl_output) {
        state->outputs[i].width = width;
        state->outputs[i].height = height;
        break;
      }
    }
  }
}

static void output_handle_done(void *data, struct wl_output *wl_output) {
  (void)data; (void)wl_output;
}

static void output_handle_scale(void *data, struct wl_output *wl_output, int32_t factor) {
  (void)data; (void)wl_output; (void)factor;
}

const struct wl_output_listener output_listener = {
  .geometry = output_handle_geometry,
  .mode = output_handle_mode,
  .done = output_handle_done,
  .scale = output_handle_scale,
};

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version)
{
  (void)version; // Silence compiler warnings
  MeowhudState *state = data;

  if (strcmp(interface, wl_compositor_interface.name) == 0) {
    state->compositor = wl_registry_bind(registry, name, &wl_compositor_interface, 6);
  } else if (strcmp(interface, zwlr_layer_shell_v1_interface.name) == 0){
    state->layer_shell = wl_registry_bind(registry, name, &zwlr_layer_shell_v1_interface, 4);
  } else if (strcmp(interface, wl_shm_interface.name) == 0) {
    state->shm = wl_registry_bind(registry, name, &wl_shm_interface, 1);
  } else if (strcmp(interface, wl_output_interface.name) == 0) {
    state->output_count++;
    state->outputs = safe_realloc(state->outputs, state->output_count * sizeof(Output_s));
    
    Output_s *new_output = &state->outputs[state->output_count - 1];
    new_output->wl_output = wl_registry_bind(registry, name, &wl_output_interface, 3);
    new_output->width = 0;
    new_output->height = 0;
    
    wl_output_add_listener(new_output->wl_output, &output_listener, state);
  }

  // printf("interface: '%s', version: %d, name: %d\n", interface, version, name);
}


static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
  // This space deliberately left blank
  (void)data; // Silence compiler warnings
  (void)registry; // Silence compiler warnings
  (void)name; // Silence compiler warnings
}


const struct wl_registry_listener registry_listener = {
  .global = registry_handle_global,
  .global_remove = registry_handle_global_remove,
};

static void handle_configure(void *data,
                             struct zwlr_layer_surface_v1 *layer_surface,
                             uint32_t serial, uint32_t width, uint32_t height) {
  MeowhudState *state = data;

  zwlr_layer_surface_v1_ack_configure(layer_surface, serial);

  // instead of asserting that the width and height are the same, just comply to the compositor
  state->width = width;
  state->height = height;

  state->configured = true;

  // initialize the buffer with the new size
  init_buffer(state);
}

static void handle_closed(void *data, struct zwlr_layer_surface_v1 *layer_surface) {
  (void)data; // Silence compiler warnings
  (void)layer_surface; // Silence compiler warnings
}

const struct zwlr_layer_surface_v1_listener layer_surface_listener = {
  .configure = handle_configure,
  .closed = handle_closed,
};

static void handle_release(void *data, struct wl_buffer *buffer) {
  (void)data; // Silence compiler warnings
  (void)buffer; // Silence compiler warnings
}

const struct wl_buffer_listener buffer_listener = {
  .release = handle_release,
};
