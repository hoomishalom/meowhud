#include "../include/parser.h"
#include "pixman.h"
#include "types.h"
#include <errno.h>
#include <stdint.h>

extern int errno;

uint32_t pixman_compute_stride(pixman_format_code_t format, int width)
{
  uint32_t stride = 4 * width;

  return stride;
}

static pixman_color_t get_color_8_to_16(char *color_string){ 
  uint32_t raw_color = strtoul(color_string, NULL, 16); // 16 for hex
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
  for (size_t i = 0; i < row_count; i++) {
    Row_s *curr_row = rows[i];

    for (size_t j = 0; j < curr_row->left->section_count; j++) {
      free(curr_row->left->sections[j].text);
    }
    free(curr_row->left->sections);

    for (size_t j = 0; j < curr_row->right->section_count; j++) {
      free(curr_row->right->sections[j].text);
    }
    free(curr_row->right->sections);

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
    uint32_t font_count_max = atoi(value);

    if (font_count_max <= 0) {
      fprintf(stderr, "font_count is too small (is: %d, min: %d)\n",
              font_count_max, 0);
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
    int32_t width = atoi(value);

    if (width <= 0 && strcmp(value, "0") != 0) {
      fprintf(stderr, "invalid width (is: %s)\n", value);
      return;
    }

    state->width = (uint32_t) width;
  } else if (strcmp(option, "height") == 0) {
    int32_t height = atoi(value);

    if (height <= 0) {
      if (strcmp(value, "0")) {
        fprintf(stderr, "height can't be 0\n");
      } else {
        fprintf(stderr, "invalid height (is %s)\n", value);
      }

      return;
    }

    state->height = (uint32_t) height;
  } else if (strcmp(option, "row_count") == 0) { // TODO: doesnt allocate space
    uint32_t row_count = atoi(value);

    if (row_count <= 0) {
      fprintf( stderr,
              "row_count is too small or an error has occurred (is: %d, min: %d)\n",
              row_count, 0);
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

    state->anchor = strtoull(value, NULL, 2);
  } else if (strcmp(option, "row_spacing") == 0) {
    uint32_t row_spacing = strtoul(value, NULL, 10); 

    if (row_spacing == 0) {
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

  if (atoi(line_num) == 0) { // line_num is 0 or not a number, both are not valid
    fprintf(stderr, "Bad line number (is: %s)\n", line_num);
    valid = false;
  } else if ((uint32_t) atoi(line_num) > (state->row_count)) {
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

  uint32_t line_num = atoi(line_num_str);

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
    for (size_t j = 0; i < curr_row->left->section_count; i++) {
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

    for (size_t j = 0; i < curr_row->right->section_count; i++) {
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

void parse_frame(MeowhudState *state) {
  char *line = NULL;
  size_t length = 0;

  clear_sections(state); // clears sections for the next frame
  while (getline(&line, &length, stdin) != -1) {
    char *new_line = strchr(line, '\n'); // removes new line
    if (new_line) *new_line = '\0';

    if (strcmp(line, DONE_MARKER) == 0)
      break;

    handle_frame_line(state, line);
  }

  printf("test\n");
  free(line);
}
