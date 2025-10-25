#include "../include/parser.h"

int pixman_compute_stride(pixman_format_code_t format, int width)
{
    // Get bits per pixel from format
    int bpp = PIXMAN_FORMAT_BPP(format);

    // Convert width * bits → bytes (rounded up)
    int stride = (width * bpp + 7) / 8;

    // Pixman requires stride to be 4-byte aligned
    stride = (stride + 3) & ~3;

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

    curr_row->right = (Text_s *)malloc(sizeof(Text_s));
    curr_row->right->sections = NULL;
    curr_row->right->section_count = 0;

    (*rows)[i] = curr_row;
  }
}

static void free_rows(Row_s **rows, size_t row_count) {
  for (size_t i = 0; i < row_count; i++) {
    Row_s *curr_row = rows[i];

    for (size_t j = 0; j < curr_row->left->section_count; j++) {
      free(curr_row->left->sections[j]->text);
    }
    free(curr_row->left->sections);

    for (size_t j = 0; j < curr_row->right->section_count; j++) {
      free(curr_row->right->sections[j]->text);
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

/* checks required fields which are:
 * width, height (if row_count isn't set), font_count_max, font_name,
 * bg_color, row_count 
 */
static void check_required(MeowhudState *state) {
  bool valid = true;

  if (state->width == 0) {
    fprintf(stderr, "width is not set\n"); 
    valid = false;
  }

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
    uint32_t width = atoi(value);

    if (width <= 0) {
      fprintf(stderr,
              "width is too small or an error has occurred (is: %d, min: %d)\n",
              width, 0);
      return;
    }

    state->width = width;
  } else if (strcmp(option, "height") == 0) {
    uint32_t height = atoi(value);
    
    if (height <= 0) {
      fprintf(stderr,
              "height is too small or an error has occurred (is: %d, min: %d)\n",
              height, 0);
      return;
    }

    state->height = height;
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

