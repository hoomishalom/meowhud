#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include <stdint.h>
#include <pixman.h>
#include <stdbool.h>
#include <uchar.h>

typedef struct {
  char32_t *text;
  size_t len;
  pixman_image_t *color;
} TextSection_s;

typedef struct {
  TextSection_s **sections; // array of pointers to the sections
  size_t section_count;
} Text_s;

typedef struct {
  Text_s *left;
  Text_s *right;
} Row_s;

typedef struct meowhud_state {
  // Wayland Globals
  struct wl_compositor *compositor;
  struct zwlr_layer_shell_v1 *layer_shell;
  struct wl_shm *shm;

  // Wayland Objects
  struct wl_display *display;
  struct wl_registry *registry;
  struct wl_surface *surface;
  struct zwlr_layer_surface_v1 *layer;
  struct wl_buffer *buff;
  struct wl_shm_pool *shm_pool;

  // Application Data
  void *mmapped;
  pixman_format_code_t color_fmt;
  pixman_image_t *pix_img;
  int fd;

  // State Flags
  bool configured;
  bool running;

  // Surface Config 
  uint32_t width;
  uint32_t height;
  uint32_t stride;
  uint32_t shm_size;
  uint32_t anchor;

  // Text Config
  uint32_t font_count_max;
  uint32_t font_count;
  char **font_names;
  struct fcft_font *font;

  // Contents 
  pixman_image_t *bg_color;
  Row_s **content_rows;
  size_t row_count;
  size_t row_spacing;
  pixman_image_t *default_text_color;
} MeowhudState;


#endif
