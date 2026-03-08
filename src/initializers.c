#define _GNU_SOURCE
#include "../include/initializers.h"
#include "../include/parser.h"
#include "../include/listeners.h"
#include "../include/types.h"
#include "../include/utils.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"

#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <fcft/fcft.h>
#include <pixman.h>

void init_state(MeowhudState *state) {
  // wayland globals
  state->compositor = NULL;
  state->layer_shell = NULL;
  state->shm = NULL;
  
  state->outputs = NULL;
  state->output_count = 0;

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

  // Non-blocking I/O state
  state->stdin_buffer_len = 0;
  state->frame_lines_head = NULL;
  state->frame_lines_tail = NULL;

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
    NULL, // TODO: this determines the output, add an option to choose which outputs to use
    ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
    ""
  );

  zwlr_layer_surface_v1_set_anchor(state->layer, state->anchor);
  zwlr_layer_surface_v1_set_size(state->layer, state->width, state->height);

  // Make the surface pass-through for mouse events
  struct wl_region *empty_region = wl_compositor_create_region(state->compositor);
  wl_surface_set_input_region(state->surface, empty_region);
  wl_region_destroy(empty_region);

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
  zwlr_layer_surface_v1_add_listener(state->layer, &layer_surface_listener, state);
}

void init_buffer(MeowhudState *state) {

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

  // We use the entire pool for our single buffer 
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

  wl_buffer_add_listener(state->buff, &buffer_listener, state);
}

void cleanup_state(MeowhudState *state) {
  if (!state) return;

  // Free Frame Lines (Linked List)
  FrameLineNode_s *curr_node = state->frame_lines_head;
  while (curr_node) {
    FrameLineNode_s *next = curr_node->next;
    free(curr_node->line);
    free(curr_node);
    curr_node = next;
  }

  // Free Content Rows
  if (state->content_rows) {
    free_rows(state->content_rows, state->row_count);
    state->content_rows = NULL;
  }

  // Free Fonts
  if (state->font) fcft_destroy(state->font);
  if (state->font_names) {
    for (uint32_t i = 0; i < state->font_count; i++) {
      free(state->font_names[i]);
    }
    free(state->font_names);
  }

  // Free General Colors & Images
  if (state->bg_color) pixman_image_unref(state->bg_color);
  if (state->default_text_color) pixman_image_unref(state->default_text_color);
  if (state->pix_img) pixman_image_unref(state->pix_img);

  // 5. Free Shared Memory & File Descriptor
  if (state->mmapped) munmap(state->mmapped, state->shm_size);
  if (state->fd >= 0) close(state->fd);

  // Free Output Tracking
  if (state->outputs) {
    for (size_t i = 0; i < state->output_count; i++) {
      if (state->outputs[i].wl_output) {
        wl_output_destroy(state->outputs[i].wl_output);
      }
    }
    free(state->outputs);
  }

  // Free Wayland Protocol Objects (In reverse order of creation)
  if (state->layer) zwlr_layer_surface_v1_destroy(state->layer);
  if (state->surface) wl_surface_destroy(state->surface);
  if (state->buff) wl_buffer_destroy(state->buff);
  if (state->shm_pool) wl_shm_pool_destroy(state->shm_pool);
  if (state->layer_shell) zwlr_layer_shell_v1_destroy(state->layer_shell);
  if (state->shm) wl_shm_destroy(state->shm);
  if (state->compositor) wl_compositor_destroy(state->compositor);
  if (state->registry) wl_registry_destroy(state->registry);
  if (state->display) wl_display_disconnect(state->display);
}

