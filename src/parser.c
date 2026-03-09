#include "../include/parser.h"
#include "../include/utils.h"

#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcft/fcft.h>
#include <pixman.h>

#include "types.h"

extern int errno;

uint32_t pixman_compute_stride(pixman_format_code_t format, int width) {
  (void)format;

  uint32_t stride = 4 * width;

  return stride;
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
    fprintf(stderr, "Bad length of color (is: %zu, should: %d)", strlen(color),
            LENGTH_OF_TEXT_COLOR);
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
  char32_t *out = safe_malloc((len + 1) * sizeof(char32_t)); // worst-case size

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

  out = safe_realloc(out, (out_i + 1) * sizeof(char32_t));

  (*dest) = out;

  return out_i;
}

static void add_section_to_text(Text *text, char *data, pixman_image_t *color) {
  char32_t *data_32;
  uint32_t len = utf8_to_char32(data, &data_32);

  if (len == 0) return;

  TextSection section = {
    .text = data_32,
    .len = len,
    .color = color
  };

  if (text->sections == NULL || text->section_count_max == text->section_count) {
    text->sections = safe_realloc(text->sections,(size_t)(text->section_count + 1) * sizeof(TextSection));
    text->section_count_max++;
  }
  text->sections[(text->section_count)++] = section;

  if (text->section_count_max - text->section_count > DOWNSIZING_THRESHOLD) {
    text->sections = safe_realloc(text->sections, (text->section_count) * sizeof(TextSection));
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
    Row *curr_row = state->content_rows[i];
    for (size_t j = 0; j < curr_row->left->section_count; j++) {
      TextSection curr_section = curr_row->left->sections[j];

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
      TextSection curr_section = curr_row->right->sections[j];

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
  FrameLineNode *node = safe_malloc(sizeof(FrameLineNode));
  node->line = safe_strdup(line);
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
  FrameLineNode *curr = state->frame_lines_head;
  while (curr) {
    FrameLineNode *next = curr->next;
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
    FrameLineNode *curr = state->frame_lines_head;
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
