#define _GNU_SOURCE
#include "../include/initializers.h"
#include "../include/parser.h"
#include "../include/listeners.h"
#include "../include/types.h"
#include "../include/utils.h"
#include "../include/config.h"
#include "wlr-layer-shell-unstable-v1-protocol.h"

#include <sys/mman.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <fcft/fcft.h>
#include <pixman.h>

void init_state(MeowhudState *state) {
  // wayland globals
  state->compositor = NULL;
  state->layer_shell = NULL;
  state->shm = NULL;

  state->huds = NULL;
  state->hud_count = 0;

  state->display_mode = HUD_DISPLAY_MODE_MAIN;
  state->target_output_names = NULL;
  state->target_output_count = 0;

  state->display = NULL;
  state->registry = NULL;

  // application data
  state->color_fmt = PIXMAN_a8r8g8b8;

  // States
  state->running = true;

  // Non-blocking I/O state
  state->stdin_buffer_len = 0;
  state->frame_lines_head = NULL;
  state->frame_lines_tail = NULL;

  // Surface config
  state->requested_height = 0;
  state->requested_width = 0;
  state->anchor = 0;
  state->exclusive_zone = 0;

  // Initialize font
  setlocale(LC_ALL, "");

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

// gets and configures the layer for a specific HUD
static void activate_hud(OutputState *hud) {
  hud->surface = wl_compositor_create_surface(hud->global_state->compositor);

  hud->layer = zwlr_layer_shell_v1_get_layer_surface(
    hud->global_state->layer_shell,
    hud->surface,
    hud->wl_output,
    ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY,
    "meowhud"
  );

  zwlr_layer_surface_v1_set_anchor(hud->layer, hud->global_state->anchor);
  zwlr_layer_surface_v1_set_size(hud->layer, hud->global_state->requested_width, hud->global_state->requested_height);

  int32_t zone = hud->global_state->exclusive_zone;
  if (zone == CONFIG_EXCLUSIVE_AUTO) {
    bool is_left = (hud->global_state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_LEFT);
    bool is_right = (hud->global_state->anchor & ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT);
    
    // If it's anchored left XOR right, then it's a vertical bar (an ^ operation wouldn't work as the active bit is in different places)
    if ((is_left || is_right) && !(is_left && is_right)) {
      zone = hud->global_state->requested_width; 
    } else {
      zone = hud->global_state->requested_height; 
    }
  }
  zwlr_layer_surface_v1_set_exclusive_zone(hud->layer, zone);

  // Make the surface pass-through for mouse events
  struct wl_region *empty_region = wl_compositor_create_region(hud->global_state->compositor);
  wl_surface_set_input_region(hud->surface, empty_region);
  wl_region_destroy(empty_region);

  zwlr_layer_surface_v1_add_listener(hud->layer, &layer_surface_listener, hud);
  wl_surface_commit(hud->surface);
}

static void activate_huds(MeowhudState *state) {
  if (state->display_mode == HUD_DISPLAY_MODE_MAIN) {
    // Add one extra HUD for the main display (wl_output = NULL)
    state->hud_count++;
    state->huds = safe_realloc(state->huds, state->hud_count * sizeof(OutputState));
    OutputState *hud = &state->huds[state->hud_count - 1];
    hud->global_state = state;
    hud->wl_output = NULL;
    hud->name = NULL;
    hud->surface = NULL;
    hud->layer = NULL;
    hud->buff = NULL;
    hud->shm_pool = NULL;
    hud->mmapped = NULL;
    hud->fd = -1;
    hud->pix_img = NULL;
    hud->configured = false;
    hud->width = 0;
    hud->height = 0;
    activate_hud(hud);
  } else if (state->display_mode == HUD_DISPLAY_MODE_ALL) {
    for (size_t i = 0; i < state->hud_count; i++) {
      activate_hud(&state->huds[i]);
    }
  } else if (state->display_mode == HUD_DISPLAY_MODE_CHOSEN) {
    for (size_t i = 0; i < state->hud_count; i++) {
      if (state->huds[i].name) {
        for (size_t j = 0; j < state->target_output_count; j++) {
          if (strcmp(state->huds[i].name, state->target_output_names[j]) == 0) {
            activate_hud(&state->huds[i]);
            break;
          }
        }
      }
    }
  }
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

  // Note for myself, roundtrip also calls our functions for received wayaland events
  wl_display_roundtrip(state->display); // Roundtrip 1: gets the globals (outputs, compositor, etc.)
  wl_display_roundtrip(state->display); // Roundtrip 2: gets the properties of those globals (like output names)

  activate_huds(state);
}

void init_buffer(OutputState *hud) {

  // creates memory pool
  hud->stride = pixman_compute_stride(hud->global_state->color_fmt, hud->width);

  const uint32_t old_shm_size = hud->shm_size;
  hud->shm_size = (hud->stride) * (hud->height);

  hud->fd = memfd_create("meowhud_smp", 0); // creates a "file" in memory
  ftruncate(hud->fd, hud->shm_size);      // sets the size of the file

  if (hud->shm_pool != NULL) wl_shm_pool_destroy(hud->shm_pool);
  hud->shm_pool = wl_shm_create_pool(hud->global_state->shm, hud->fd, hud->shm_size); // creates shared memory pool

  if (hud->mmapped != NULL) munmap(hud->mmapped, old_shm_size);
  hud->mmapped = mmap(NULL, hud->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, hud->fd, 0); // "mounts" the file descriptor onto a memory location so it will be writeable

  if (hud->buff != NULL) wl_buffer_destroy(hud->buff);
  hud->buff = wl_shm_pool_create_buffer(hud->shm_pool, 0, hud->width, hud->height, hud->stride, WL_SHM_FORMAT_ARGB8888);

  // We use the entire pool for our single buffer 
  wl_shm_pool_destroy(hud->shm_pool);
  hud->shm_pool = NULL;
  close(hud->fd);
  hud->fd = -1;

  if (hud->pix_img != NULL) pixman_image_unref(hud->pix_img);
  hud->pix_img = pixman_image_create_bits_no_clear(
    hud->global_state->color_fmt,
    hud->width,
    hud->height,
    hud->mmapped,
    hud->stride
  );

  wl_surface_attach(hud->surface, hud->buff, 0, 0);

  wl_buffer_add_listener(hud->buff, &buffer_listener, hud);
}

void cleanup_state(MeowhudState *state) {
  if (!state) return;

  // Free Frame Lines (Linked List)
  FrameLineNode *curr_node = state->frame_lines_head;
  while (curr_node) {
    FrameLineNode *next = curr_node->next;
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

  // Free General Colors
  if (state->bg_color) pixman_image_unref(state->bg_color);
  if (state->default_text_color) pixman_image_unref(state->default_text_color);

  if (state->target_output_names) {
    for (size_t i = 0; i < state->target_output_count; i++) {
      free(state->target_output_names[i]);
    }
    free(state->target_output_names);
  }

  // Free HUD instances
  if (state->huds) {
    for (size_t i = 0; i < state->hud_count; i++) {
      OutputState *hud = &state->huds[i];
      if (hud->name) free(hud->name);
      if (hud->pix_img) pixman_image_unref(hud->pix_img);
      if (hud->mmapped) munmap(hud->mmapped, hud->shm_size);
      if (hud->fd >= 0) close(hud->fd);
      if (hud->layer) zwlr_layer_surface_v1_destroy(hud->layer);
      if (hud->surface) wl_surface_destroy(hud->surface);
      if (hud->buff) wl_buffer_destroy(hud->buff);
      if (hud->shm_pool) wl_shm_pool_destroy(hud->shm_pool);
      if (hud->wl_output) wl_output_destroy(hud->wl_output);
    }
    free(state->huds);
  }

  // Free Wayland objects (in reverse order of creation)
  if (state->layer_shell) zwlr_layer_shell_v1_destroy(state->layer_shell);
  if (state->shm) wl_shm_destroy(state->shm);
  if (state->compositor) wl_compositor_destroy(state->compositor);
  if (state->registry) wl_registry_destroy(state->registry);
  if (state->display) wl_display_disconnect(state->display);
}

