#include "../include/config.h"
#include "../include/utils.h"
#include "../include/parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcft/fcft.h>

#define MAX_HUD_HEIGHT 500
#define MAX_HUD_WIDTH 500

typedef void (*OptionHandler)(MeowhudState *state, const char *value);

typedef struct {
  const char *name;
  OptionHandler handler;
} OptionMap;

static void handle_font_count_max(MeowhudState *state, const char *value) {
  uint32_t font_count_max;
  if (!parse_uint32(value, &font_count_max, 10) || font_count_max == 0) {
    fprintf(stderr, "font_count is too small or invalid (is: %s, min: %d)\n", value, 0);
    return;
  }

  for (size_t i = 0; i < state->font_count; i++) {
    free(state->font_names[i]);
  }
  free(state->font_names);
  state->font_count = 0;

  state->font_count_max = font_count_max; 
  state->font_names = (char **)safe_malloc(state->font_count_max * sizeof(char *));

  for (size_t i = 0; i < state->font_count_max; i++) {
    state->font_names[i] = NULL;
  }
}

static void handle_font_name(MeowhudState *state, const char *value) {
  if (state->font_count_max == 0 || state->font_count >= state->font_count_max) {
    fprintf(stderr, "No more space for fonts\n");
    return;
  }
  state->font_names[state->font_count] = safe_strdup(value);
  state->font_count++;
}

static void handle_width(MeowhudState *state, const char *value) {
  int32_t width;
  if (!parse_int32(value, &width, 10) || (width <= 0 && strcmp(value, "0") != 0)) {
    fprintf(stderr, "invalid width (is: %s)\n", value);
    return;
  }
  state->width = (uint32_t)width;
}

static void handle_height(MeowhudState *state, const char *value) {
  int32_t height;
  if (!parse_int32(value, &height, 10) || height < 0) {
    fprintf(stderr, "invalid height (is %s)\n", value);
    return;
  }
  if (height == 0 && strcmp(value, "0") == 0) {
    fprintf(stderr, "height can't be 0\n");
    return;
  }
  state->height = (uint32_t)height;
}

static void handle_row_count(MeowhudState *state, const char *value) {
  uint32_t row_count;
  if (!parse_uint32(value, &row_count, 10) || row_count == 0) {
    fprintf(stderr, "row_count is too small or invalid (is: %s, min: %d)\n", value, 0);
    return;
  }
  free_rows(state->content_rows, state->row_count);
  state->row_count = row_count;
  init_rows(&state->content_rows, state->row_count);
}

static void handle_bg_color(MeowhudState *state, const char *value) {
  if (strlen(value) != LENGTH_OF_BG_COLOR) {
    fprintf(stderr, "bg_color isn't of the correct length (is: %zu, should: %d)\n", strlen(value), LENGTH_OF_BG_COLOR);
    return;
  }
  pixman_color_t color = get_color_8_to_16((char *)value);
  if (state->bg_color) pixman_image_unref(state->bg_color);
  state->bg_color = pixman_image_create_solid_fill(&color);
}

static void handle_default_text_color(MeowhudState *state, const char *value) {
  if (strlen(value) != LENGTH_OF_TEXT_COLOR) {
    fprintf(stderr, "default_text_color isn't of the correct length (is: %zu, should: %d)\n", strlen(value), LENGTH_OF_TEXT_COLOR);
    return;
  }
  pixman_color_t color = get_color_8_to_16((char *)value);
  if (state->default_text_color) pixman_image_unref(state->default_text_color);
  state->default_text_color = pixman_image_create_solid_fill(&color);
}

static void handle_anchor(MeowhudState *state, const char *value) {
  if (strlen(value) != LENGTH_OF_ANCHOR) {
    fprintf(stderr, "anchor isn't of the correct length (is: %zu, should: %d)\n", strlen(value), LENGTH_OF_ANCHOR);
    return;
  }
  uint64_t anchor;
  if (!parse_uint64(value, &anchor, 2)) {
    fprintf(stderr, "invalid anchor (is: %s)\n", value);
    return;
  }
  state->anchor = (uint32_t)anchor;
}

static void handle_row_spacing(MeowhudState *state, const char *value) {
  uint32_t row_spacing;
  if (!parse_uint32(value, &row_spacing, 10) || row_spacing == 0) {
    fprintf(stderr, "row_spacing produced an error, must be positive integer\n");
    return;
  }
  state->row_spacing = row_spacing;
}

static const OptionMap option_handlers[] = {
  {"font_count_max", handle_font_count_max},
  {"font_name", handle_font_name},
  {"width", handle_width},
  {"height", handle_height},
  {"row_count", handle_row_count},
  {"bg_color", handle_bg_color},
  {"default_text_color", handle_default_text_color},
  {"anchor", handle_anchor},
  {"row_spacing", handle_row_spacing},
};

static void handle_options_line(char *line, MeowhudState *state) {
  char *option = strsep(&line, DELIMITER);
  char *value = strsep(&line, DELIMITER);

  if (!option || !value) return;

  size_t num_handlers = sizeof(option_handlers) / sizeof(option_handlers[0]);
  for (size_t i = 0; i < num_handlers; i++) {
    if (strcmp(option, option_handlers[i].name) == 0) {
      option_handlers[i].handler(state, value);
      return;
    }
  }
  fprintf(stderr, "Unknown option: %s\n", option);
}

static void check_required(MeowhudState *state) {
  bool valid = true;
  if (state->height == 0 && state->row_count == 0) {
    fprintf(stderr, "height is not set (you could set row_count and height will be set automatically)\n"); 
    valid = false;
  }
  if (state->font_count_max == 0) {
    fprintf(stderr, "font_count_max is not set\n");
    valid = false;
  }
  if (state->font_count_max > 0 && state->font_names[0] == NULL) {
    fprintf(stderr, "atleast one font_name must be set\n");
    valid = false;
  }
  if (state->bg_color == NULL) {
    fprintf(stderr, "bg_color must be set\n");
    valid = false;
  }
  if (state->row_count == 0) {
    fprintf(stderr, "row_count must be set\n");
    valid = false;
  }
  if (!valid) exit(EXIT_FAILURE);
}

static void init_fonts(MeowhudState *state) {
  state->font = fcft_from_name(
    state->font_count,
    (const char **)state->font_names,
    NULL
  );
  if (!state->font) {
    fprintf(stderr, "Failed to load font\n");
    exit(EXIT_FAILURE);
  }
}

void parse_options(MeowhudState *state) {
  char *line = NULL;
  size_t length = 0;

  while (getline(&line, &length, stdin) != -1) {
    char *new_line = strchr(line, '\n');
    if (new_line) *new_line = '\0';

    if (strcmp(line, DONE_MARKER) == 0) break;
    handle_options_line(line, state);
  }

  check_required(state);
  init_fonts(state);

  if (state->height == 0) {
    state->height = state->row_count * (state->font->height + state->row_spacing) - state->row_spacing;
  }

  free(line);
}
