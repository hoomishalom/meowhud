#include "../include/parser.h"
#include "pixman.h"
#include "types.h"
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include "../include/utils.h"

extern int errno;

uint32_t pixman_compute_stride(pixman_format_code_t format, int width)
{
  uint32_t stride = 4 * width;

  return stride;
}

static pixman_color_t get_color_8_to_16(char *color_string){ 
  uint32_t raw_color = 0;
  if (color_string && *color_string != '\0') {
    if (!parse_uint32(color_string, &raw_color, 16)) {
      fprintf(stderr, "Failed to parse color string: %s\n", color_string);
    }
  }
  pixman_color_t color;

  // puts parts of raw_color and places it in the right place, and
  // adds trailing zeros because pixman_color_t uses 16 bit colors
  color.alpha = (((raw_color >> 24) & 0xff) << 8);
  color.red = (((raw_color >> 16) & 0xff) << 8);
  color.green = (((raw_color >> 8) & 0xff) << 8);
  color.blue = (((raw_color) & 0xff) << 8);

  return color;
}

static void init_rows(Row_s ***rows, size_t row_count) {
  *rows = (Row_s **)malloc(row_count * sizeof(Row_s *));

  for (size_t i = 0; i < row_count; i++) {
    Row_s *curr_row = (Row_s *)malloc(sizeof(Row_s));

    curr_row->left = (Text_s *)malloc(sizeof(Text_s));
    curr_row->left->sections = NULL;
    curr_row->left->section_count = 0;
    curr_row->left->section_count_max = 0;

    curr_row->right = (Text_s *)malloc(sizeof(Text_s));
    curr_row->right->sections = NULL;
    curr_row->right->section_count = 0;
    curr_row->right->section_count_max = 0;

    (*rows)[i] = curr_row;
  }
}

static void free_rows(Row_s **rows, size_t row_count) {
  if (!rows) return;
  for (size_t i = 0; i < row_count; i++) {
    Row_s *curr_row = rows[i];
    if (!curr_row) continue;

    if (curr_row->left) {
      for (size_t j = 0; j < curr_row->left->section_count; j++) {
        if (curr_row->left->sections[j].text) free(curr_row->left->sections[j].text);
        if (curr_row->left->sections[j].color) pixman_image_unref(curr_row->left->sections[j].color);
      }
      if (curr_row->left->sections) free(curr_row->left->sections);
      free(curr_row->left);
    }

    if (curr_row->right) {
      for (size_t j = 0; j < curr_row->right->section_count; j++) {
        if (curr_row->right->sections[j].text) free(curr_row->right->sections[j].text);
        if (curr_row->right->sections[j].color) pixman_image_unref(curr_row->right->sections[j].color);
      }
      if (curr_row->right->sections) free(curr_row->right->sections);
      free(curr_row->right);
    }

    free(curr_row);
  }
  free(rows);
}


static void init_fonts(MeowhudState *state) {
  state->font = fcft_from_name(
    state->font_count,
    (const char **)state->font_names,  // cast is to make lsp shut up
    NULL
  );
  if (!state->font) {
    fprintf(stderr, "Failed to load font\n");
    exit(EXIT_FAILURE);
  }
}

// checks required fields which are:
// width, height (if row_count isn't set), font_count_max, font_name,
// bg_color, row_count 
static void check_required(MeowhudState *state) {
  bool valid = true;

  if (state->height == 0 && state->row_count == 0) {
    fprintf(stderr, "height is not set (you could set row_count and"
            " height will be set automatically)\n"); 
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

static void handle_options_line(char *line,
                                MeowhudState *state) {
  char *option = strsep(&line, DELIMITER);
  char *value = strsep(&line, DELIMITER);

  if (strcmp(option, "font_count_max") == 0) {
    uint32_t font_count_max;

    if (!parse_uint32(value, &font_count_max, 10) || font_count_max <= 0) {
      fprintf(stderr, "font_count is too small or invalid (is: %s, min: %d)\n",
              value, 0);
      return;
    }

    for (size_t i = 0; i < state->font_count; i++) {
      free(state->font_names[i]);
    }
    free(state->font_names);
    state->font_count = 0;

    state->font_count_max = font_count_max; 
    state->font_names = (char **)malloc(state->font_count_max * sizeof(char *));

    for (size_t i = 0; i < (state->font_count_max); i++) {
      state->font_names[i] = NULL;
    }
  } else if (strcmp(option, "font_name") == 0) {
    if (((state->font_count_max) - (state->font_count)) <=  0) {
      fprintf(stderr, "No more space for fonts\n");
      return;
    }

    state->font_names[state->font_count] = (char *)malloc((strlen(value) + 1) * sizeof(char));
    strcpy(state->font_names[state->font_count], value);
    (state->font_count)++;
  } else if (strcmp(option, "width") == 0) {
    int32_t width;

    if (!parse_int32(value, &width, 10)) {
      fprintf(stderr, "invalid width (is: %s)\n", value);
      return;
    }

    if (width <= 0 && strcmp(value, "0") != 0) {
      fprintf(stderr, "invalid width (is: %s)\n", value);
      return;
    }

    state->width = (uint32_t) width;
  } else if (strcmp(option, "height") == 0) {
    int32_t height;

    if (!parse_int32(value, &height, 10)) {
      fprintf(stderr, "invalid height (is %s)\n", value);
      return;
    }

    if (height <= 0) {
      if (strcmp(value, "0") == 0) {
        fprintf(stderr, "height can't be 0\n");
      } else {
        fprintf(stderr, "invalid height (is %s)\n", value);
      }

      return;
    }

    state->height = (uint32_t) height;
  } else if (strcmp(option, "row_count") == 0) { // TODO: doesnt allocate space
    uint32_t row_count;

    if (!parse_uint32(value, &row_count, 10) || row_count <= 0) {
      fprintf( stderr,
              "row_count is too small or invalid (is: %s, min: %d)\n",
              value, 0);
      return;
    }

    free_rows(state->content_rows, state->row_count);
    state->row_count = row_count;
    init_rows(&state->content_rows, state->row_count);
  } else if (strcmp(option, "bg_color") == 0) { // doesn't check that value has actual hex
    if (strlen(value) != LENGTH_OF_BG_COLOR) {
      fprintf(stderr,
              "bg_color isn't of the correct length (is: %lu, should: %d)\n",
              strlen(value), LENGTH_OF_BG_COLOR);
      return;
    }

    pixman_color_t color = get_color_8_to_16(value);

    if (state->bg_color) pixman_image_unref(state->bg_color);
    state->bg_color = pixman_image_create_solid_fill(&color);
  } else if (strcmp(option, "default_text_color") == 0) { // doesn't check that vlue has actual hex
    if (strlen(value) != LENGTH_OF_TEXT_COLOR) {
      fprintf(stderr,
              "default_text_color isn't of the correct length (is: %lu, should: %d)\n",
              strlen(value), LENGTH_OF_TEXT_COLOR);
      return;
    }

    pixman_color_t color = get_color_8_to_16(value);

    pixman_image_unref(state->default_text_color);
    state->default_text_color = pixman_image_create_solid_fill(&color);
  } else if (strcmp(option, "anchor") == 0) { // doesnt check value has actual bits (and not any number)
    if (strlen(value) != LENGTH_OF_ANCHOR) {
      fprintf(stderr,
              "anchor isn't of the correct length (is: %lu, should: %d)\n",
              strlen(value), LENGTH_OF_ANCHOR);
      return;
    }

    uint64_t anchor;
    if (!parse_uint64(value, &anchor, 2)) {
      fprintf(stderr, "invalid anchor (is: %s)\n", value);
      return;
    }
    state->anchor = (uint32_t)anchor;
  } else if (strcmp(option, "row_spacing") == 0) {
    uint32_t row_spacing;

    if (!parse_uint32(value, &row_spacing, 10) || row_spacing == 0) {
      fprintf(stderr, "row_spacing produced an error, must be positive integer\n");
      return;
    }

    state->row_spacing = row_spacing;
  } else {
    fprintf(stderr, "Unknown option: %s\n", option);
  }
}

void parse_options(MeowhudState *state) {
  char *line = NULL;
  size_t length = 0;

  while (getline(&line, &length, stdin) != -1) {
    char *new_line = strchr(line, '\n'); // removes new line
    if (new_line) *new_line = '\0';

    if (strcmp(line, DONE_MARKER) == 0)
      break;

    handle_options_line(line, state);
  }

  check_required(state);

  init_fonts(state);

  if (state->height == 0) { // automatically set height
    state->height = state->row_count * (state->font->height + state->row_spacing) - state->row_spacing;
  }

  free(line);
}

static bool verify_frame_values(MeowhudState *state, char *line_num, char *alignment,
                                char *color, char *text) {
  bool valid = true;

  if (line_num == NULL || alignment == NULL || color == NULL || text == NULL) {
    fprintf(stderr, "Not enough fields, format is: line_num%salignment%scolor%stext\n",
            DELIMITER, DELIMITER, DELIMITER);
    return false; // no need to check the rest of the conditions
  }

  uint32_t parsed_line_num;
  if (!parse_uint32(line_num, &parsed_line_num, 10) || parsed_line_num == 0) { // line_num is 0 or not a number, both are not valid
    fprintf(stderr, "Bad line number (is: %s)\n", line_num);
    valid = false;
  } else if (parsed_line_num > state->row_count) {
    fprintf(stderr, "line number is too big (is: %s, max: %u)\n", line_num, state->row_count);
    valid = false;
  }

  if (strcmp(alignment, LEFT_ALIGNMENT) != 0 && strcmp(alignment, RIGHT_ALIGNMENT)) {
    fprintf(stderr, "Bad alignment (is: %s, should be %s or %s)\n", alignment,
            LEFT_ALIGNMENT, RIGHT_ALIGNMENT);
    valid = false;
  }

  if (strlen(color) != LENGTH_OF_TEXT_COLOR && strlen(color) != 0) {
    fprintf(stderr, "Bad length of color (is: %lu, should: %d)", strlen(color), LENGTH_OF_TEXT_COLOR);
    valid = false;
  }

  if (strlen(text) == 0) {
    fprintf(stderr, "text should be of a positive length (is: 0)\n");
    valid = false;
  }

  return valid;
}

// transforms src (utf8) to dest (char32_t) and returns the length of dest
static uint32_t utf8_to_char32(char *src, char32_t **dest) {
  size_t len = strlen(src);
  char32_t *out = malloc((len + 1) * sizeof(char32_t)); // worst-case size
  if (!out) return 0;

  mbstate_t state = {0};
  const char *ptr = src;
  char32_t c32;
  size_t out_i = 0;

  while (*ptr) {
    // translate a sequence of utf8 chars to a char32_t and returns
    // the amount of utf8 bytes that were tranlsated
    size_t bytes = mbrtoc32(&c32, ptr, MB_CUR_MAX, &state);

    if (bytes == (size_t)-1) { // this means bad utf8 data
      fprintf(stderr, "Invalid utf8 sequence\n");
      free(out);
      return 0;
    } else if (bytes == (size_t)-2) { // this means an incomplete sequence of multibyte data
      fprintf(stderr, "Incomplete utf8 sequence\n");
      free(out);
      return 0;
    } else if (bytes == 0) { // Null character encountered
      break;
    }

    out[out_i++] = c32;
    ptr += bytes;
  }

  out[out_i] = U'\0'; // Null-terminate char32_t string

  out = realloc(out, (out_i + 1) * sizeof(char32_t));

  (*dest) = out;

  return out_i;
}

static void add_section_to_text(Text_s *text, char *data, pixman_image_t *color) {
  char32_t *data_32;
  uint32_t len = utf8_to_char32(data, &data_32);

  if (len == 0) return;

  TextSection_s section = {
    .text = data_32,
    .len = len,
    .color = color
  };

  if (text->sections == NULL || text->section_count_max == text->section_count) {
    text->sections = realloc(text->sections,(size_t)(text->section_count + 1) * sizeof(TextSection_s));
    text->section_count_max++;
  }
  text->sections[(text->section_count)++] = section;

  if (text->section_count_max - text->section_count > DOWNSIZING_THRESHOLD) {
    text->sections = realloc(text->sections, (text->section_count) * sizeof(TextSection_s));
  }
}

static void handle_frame_line(MeowhudState *state, char *line) {
  char *line_num_str = strsep(&line, DELIMITER);
  char *alignment = strsep(&line, DELIMITER);
  char *color_str = strsep(&line, DELIMITER);
  char *text = strsep(&line, DELIMITER);

  bool valid = verify_frame_values(state, line_num_str, alignment, color_str, text);

  if (!valid) return;

  uint32_t line_num;
  parse_uint32(line_num_str, &line_num, 10);

  pixman_image_t *color;
  pixman_color_t temp_color = get_color_8_to_16(color_str);
  if (strlen(color_str) == 8) {
    color = pixman_image_create_solid_fill(&temp_color);
  } else {
    color = NULL;
  }

  if (strcmp(alignment, LEFT_ALIGNMENT) == 0) {
    add_section_to_text(state->content_rows[line_num - 1]->left, text, color);
  } else if (strcmp(alignment, RIGHT_ALIGNMENT) == 0) {
    add_section_to_text(state->content_rows[line_num - 1]->right, text, color);
  }
}

static void clear_sections(MeowhudState *state) {
  for (size_t i = 0; i < state->row_count; i++) {
    Row_s *curr_row = state->content_rows[i];
    for (size_t j = 0; j < curr_row->left->section_count; j++) {
      TextSection_s curr_section = curr_row->left->sections[j];

      if (curr_section.text != NULL) {
        free(curr_section.text);
      }

      if (curr_section.color != NULL) {
        pixman_image_unref(curr_section.color);
      }

      curr_section.len = 0; // shouldn't matter, for good measure
    } 
    curr_row->left->section_count = 0;

    for (size_t j = 0; j < curr_row->right->section_count; j++) {
      TextSection_s curr_section = curr_row->right->sections[j];

      if (curr_section.text != NULL) {
        free(curr_section.text);
      }

      if (curr_section.color != NULL) {
        pixman_image_unref(curr_section.color);
      }

      curr_section.len = 0; // shouldn't matter, for good measure
    } 
    curr_row->right->section_count = 0;
  }
}

static void append_frame_line(MeowhudState *state, const char *line) {
  FrameLineNode_s *node = malloc(sizeof(FrameLineNode_s));
  node->line = strdup(line);
  node->next = NULL;

  if (!state->frame_lines_head) {
    state->frame_lines_head = node;
    state->frame_lines_tail = node;
  } else {
    state->frame_lines_tail->next = node;
    state->frame_lines_tail = node;
  }
}

static void free_frame_lines(MeowhudState *state) {
  FrameLineNode_s *curr = state->frame_lines_head;
  while (curr) {
    FrameLineNode_s *next = curr->next;
    free(curr->line);
    free(curr);
    curr = next;
  }
  state->frame_lines_head = NULL;
  state->frame_lines_tail = NULL;
}

bool parse_frame(MeowhudState *state) {
  char temp_buf[1024];
  ssize_t bytes_read;
  bool frame_ready = false;

  // Read non-blockingly until EAGAIN or EOF
  while ((bytes_read = read(STDIN_FILENO, temp_buf, sizeof(temp_buf))) > 0) {
    // Ensure we don't overflow the persistent buffer
    if (state->stdin_buffer_len + bytes_read > sizeof(state->stdin_buffer) - 1) {
      fprintf(stderr, "stdin buffer overflow, dropping data\n");
      state->stdin_buffer_len = 0; // simplistic recovery
      continue;
    }

    memcpy(state->stdin_buffer + state->stdin_buffer_len, temp_buf, bytes_read);
    state->stdin_buffer_len += bytes_read;
    state->stdin_buffer[state->stdin_buffer_len] = '\0';

    // Parse out complete lines separated by '\n'
    char *newline_pos;
    while ((newline_pos = strchr(state->stdin_buffer, '\n')) != NULL) {
      *newline_pos = '\0'; // Null-terminate the line

      // Strip optional \r for Windows line endings compatibility
      char *cr_pos = strchr(state->stdin_buffer, '\r');
      if (cr_pos) *cr_pos = '\0';

      if (strcmp(state->stdin_buffer, DONE_MARKER) == 0) {
        frame_ready = true;
      } else {
        append_frame_line(state, state->stdin_buffer);
      }

      // Shift the remaining buffer down
      size_t line_len = (newline_pos - state->stdin_buffer) + 1;
      size_t remaining_len = state->stdin_buffer_len - line_len;
      memmove(state->stdin_buffer, newline_pos + 1, remaining_len);
      state->stdin_buffer_len = remaining_len;
      state->stdin_buffer[state->stdin_buffer_len] = '\0';
    }
  }

  // If we received EOF, stdin closed
  if (bytes_read == 0) {
    state->running = false;
  }

  if (frame_ready) {
    clear_sections(state); // clears sections for the next frame

    // Apply all buffered lines to the new frame
    FrameLineNode_s *curr = state->frame_lines_head;
    while (curr) {
      // handle_frame_line uses strsep and modifies the string, but curr->line is strdup'd so it's safe.
      handle_frame_line(state, curr->line);
      curr = curr->next;
    }

    free_frame_lines(state);
    return true;
  }

  return false;
}
