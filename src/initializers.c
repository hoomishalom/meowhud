#include "../include/initializers.h"
#include "parser.h"
#include "pixman.h"
#include "types.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"

void init_state(MeowhudState *state) {
  // wayland globals
  state->compositor = NULL;
  state->layer_shell = NULL;
  state->shm = NULL;

  state->display = NULL;
  state->registry = NULL;
  state->surface = NULL;
  state->layer = NULL;
  state->buff = NULL;
  state->shm_pool = NULL;

  // Memory stuff
  state->mmapped = NULL;
  state->fd = -1;

  // States
  state->configured = false;
  state->running = true;

  // Surface config
  state->height = 0;
  state->width = 0;
  state->stride = 0;
  state->shm_size = 0;
  state->anchor = 0;

  // Initialize font
  setlocale(LC_ALL, "");

  state->font_count_max = 0;
  state->font_count = 0;
  state->font_names = NULL;
  state->bg_color = NULL;

  state->row_spacing = 0;
  state->row_count = 0;
  state->content_rows = NULL;  
  // this is the default defulat (double default) color
  pixman_color_t default_text_color = { 
    .red = 0x0000,
    .green = 0x0000,
    .blue = 0x0000,
    .alpha = 0xffff
  };
  state->default_text_color = pixman_image_create_solid_fill(&default_text_color);
}

// gets and configures the layer, assumes state already has surface
static void get_and_configure_layer(MeowhudState *state) {
  state->layer = zwlr_layer_shell_v1_get_layer_surface(
    state->layer_shell,
    state->surface,
    NULL,
    ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
    ""
  );

  zwlr_layer_surface_v1_set_anchor(state->layer, state->anchor);
  zwlr_layer_surface_v1_set_size(state->layer, state->width, state->height);
  wl_surface_commit(state->surface);
}

void init_hud(MeowhudState *state) {
  //  gets display
  state->display = wl_display_connect(NULL);
  if (!(state->display)) {
    fprintf(stderr, "Failed to establish connection with display :( !\n");
    exit(EXIT_FAILURE);
  }

  // gets registry
  state->registry = wl_display_get_registry(state->display);
  if (!(state->registry)) {
    fprintf(stderr, "Failed to get registery!\n");
    exit(EXIT_FAILURE);
  }

  wl_registry_add_listener(state->registry, &registry_listener, state);
  wl_display_roundtrip(state->display); // needs to get the compositor

  state->surface = wl_compositor_create_surface(state->compositor);
  get_and_configure_layer(state);

  // creates memory pool
  state->stride = pixman_compute_stride(state->color_fmt, state->width);

  const uint32_t old_shm_size = state->shm_size;
  state->shm_size = (state->stride) * (state->height);

  state->fd = memfd_create("meowhud_smp", 0); // creates a "file" in memory
  ftruncate(state->fd, state->shm_size);      // sets the size of the file

  if (state->shm_pool != NULL) wl_shm_pool_destroy(state->shm_pool);
  state->shm_pool = wl_shm_create_pool( state->shm, state->fd, state->shm_size); // creates shared memory pool

  if (state->mmapped != NULL) munmap(state->mmapped, old_shm_size);
  state->mmapped = mmap(NULL, state->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, state->fd, 0); // "mounts" the file descriptor onto a memory location so it will be writeable

  if (state->buff != NULL) wl_buffer_destroy(state->buff);
  state->buff = wl_shm_pool_create_buffer(state->shm_pool, 0, state->width, state->height, state->stride, WL_SHM_FORMAT_ARGB8888);

  /* We use the entire pool for our single buffer */
  wl_shm_pool_destroy(state->shm_pool);
  state->shm_pool = NULL;
  close(state->fd);
  state->fd = -1;
  state->color_fmt = PIXMAN_a8r8g8b8;

  state->pix_img = pixman_image_create_bits_no_clear(
    state->color_fmt,
    state->width,
    state->height,
    state->mmapped,
    state->stride
  );

  wl_surface_attach(state->surface, state->buff, 0, 0);

  zwlr_layer_surface_v1_add_listener(state->layer, &layer_surface_listener, state);
  wl_buffer_add_listener(state->buff, &buffer_listener, state);
}

