#ifndef TYPES_H_INCLUDED
#define TYPES_H_INCLUDED

#include <stdint.h>
#include <pixman.h>
#include <stdbool.h>
#include <uchar.h>

typedef struct {
  char32_t *text;
  uint32_t len;
  pixman_image_t *color;
} TextSection;

typedef struct {
  TextSection *sections; // array of pointers to the sections
  uint32_t section_count_max; // this is the allocated amount
  uint32_t section_count; // this is the actual used amount
} Text;

typedef struct {
  Text *left;
  Text *right;
} Row;

typedef struct FrameLineNode {
  char *line;
  struct FrameLineNode *next;
} FrameLineNode;

typedef enum {
  HUD_DISPLAY_MODE_MAIN,
  HUD_DISPLAY_MODE_ALL,
  HUD_DISPLAY_MODE_CHOSEN
} DisplayMode;

// State of the whole program
typedef struct {
  // wayland globals
  struct wl_compositor *compositor;
  struct zwlr_layer_shell_v1 *layer_shell;
  struct wl_shm *shm;
  
  // monitors
  struct output_state *huds;
  size_t hud_count;

  // targeting config
  DisplayMode display_mode;
  char **target_output_names;
  size_t target_output_count;

  // wayland objects
  struct wl_display *display;
  struct wl_registry *registry;

  // application data
  pixman_format_code_t color_fmt;

  // state flags
  bool running;

  // non-blocking i/o state
  char stdin_buffer[4096];
  size_t stdin_buffer_len;
  FrameLineNode *frame_lines_head;
  FrameLineNode *frame_lines_tail;

  // surface config 
  uint32_t requested_width;   // requested dimensions for all of the surfaces
  uint32_t requested_height;  // 
  uint32_t anchor;
  int32_t exclusive_zone;

  // text config
  uint32_t font_count;
  char **font_names;
  struct fcft_font *font;

  // contents 
  pixman_image_t *bg_color;
  Row **content_rows;
  uint32_t row_count;
  uint32_t row_spacing;
  pixman_image_t *default_text_color;
} MeowhudState;

// State of each output (i.e, monitor)
typedef struct output_state {
  MeowhudState *global_state;
  struct wl_output *wl_output;
  char *name;
  
  // wayland surface objects
  struct wl_surface *surface;
  struct zwlr_layer_surface_v1 *layer;
  struct wl_buffer *buff;
  struct wl_shm_pool *shm_pool;

  // dimensions
  uint32_t monitor_width;
  uint32_t monitor_height;
  uint32_t width;
  uint32_t height;
  uint32_t stride;
  uint32_t shm_size;

  // render state
  void *mmapped;
  int fd;
  pixman_image_t *pix_img;
  bool configured;
} OutputState;

#endif
