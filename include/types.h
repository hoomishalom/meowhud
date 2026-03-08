#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include <stdint.h>
#include <pixman.h>
#include <stdbool.h>
#include <uchar.h>

typedef struct {
  char32_t *text;
  uint32_t len;
  pixman_image_t *color;
} TextSection_s;

typedef struct {
  TextSection_s *sections; // array of pointers to the sections
  uint32_t section_count_max; // this is the allocated amount
  uint32_t section_count; // this is the actual used amount
} Text_s;

typedef struct {
  Text_s *left;
  Text_s *right;
} Row_s;

typedef struct FrameLineNode {
  char *line;
  struct FrameLineNode *next;
} FrameLineNode_s;

typedef struct {
  struct wl_output *wl_output;
  uint32_t width;
  uint32_t height;
} Output_s;

typedef struct meowhud_state {
  // wayland globals
  struct wl_compositor *compositor;
  struct zwlr_layer_shell_v1 *layer_shell;
  struct wl_shm *shm;
  
  Output_s *outputs;
  size_t output_count;

  // wayland objects
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_surface *surface;
  struct zwlr_layer_surface_v1 *layer;
  struct wl_buffer *buff;
  struct wl_shm_pool *shm_pool;

  // application data
  void *mmapped;
  pixman_format_code_t color_fmt;
  pixman_image_t *pix_img;
  int fd;

  // state flags
  bool configured;
  bool running;

  // non-blocking i/o state
  char stdin_buffer[4096];
  size_t stdin_buffer_len;
  FrameLineNode_s *frame_lines_head;
  FrameLineNode_s *frame_lines_tail;

  // surface config 
  uint32_t width;
  uint32_t height;
  uint32_t stride;
  uint32_t shm_size;
  uint32_t anchor;

  // text config
  uint32_t font_count_max;
  uint32_t font_count;
  char **font_names;
  struct fcft_font *font;

  // contents 
  pixman_image_t *bg_color;
  Row_s **content_rows;
  uint32_t row_count;
  uint32_t row_spacing;
  pixman_image_t *default_text_color;
} MeowhudState;


#endif
