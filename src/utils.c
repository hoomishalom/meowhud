#include "../include/utils.h"
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

bool parse_uint32(const char *str, uint32_t *out, int base) {
  if (!str || *str == '\0') return false;
  char *endptr;
  errno = 0;
  unsigned long val = strtoul(str, &endptr, base);
  if (errno != 0 || *endptr != '\0' || val > UINT32_MAX) return false;
  *out = (uint32_t)val;
  return true;
}

bool parse_int32(const char *str, int32_t *out, int base) {
  if (!str || *str == '\0') return false;
  char *endptr;
  errno = 0;
  long val = strtol(str, &endptr, base);
  if (errno != 0 || *endptr != '\0' || val > INT32_MAX || val < INT32_MIN) return false;
  *out = (int32_t)val;
  return true;
}

bool parse_uint64(const char *str, uint64_t *out, int base) {
  if (!str || *str == '\0') return false;
  char *endptr;
  errno = 0;
  unsigned long long val = strtoull(str, &endptr, base);
  if (errno != 0 || *endptr != '\0') return false;
  *out = (uint64_t)val;
  return true;
}

void *safe_malloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr && size > 0) {
    fprintf(stderr, "Fatal: failed to allocate %zu bytes\n", size);
    exit(EXIT_FAILURE);
  }
  return ptr;
}

void *safe_realloc(void *ptr, size_t size) {
  void *new_ptr = realloc(ptr, size);
  if (!new_ptr && size > 0) {
    fprintf(stderr, "Fatal: failed to reallocate %zu bytes\n", size);
    exit(EXIT_FAILURE);
  }
  return new_ptr;
}

char *safe_strdup(const char *s) {
  if (!s) return NULL;
  char *new_s = strdup(s);
  if (!new_s) {
    fprintf(stderr, "Fatal: failed to duplicate string\n");
    exit(EXIT_FAILURE);
  }
  return new_s;
}

pixman_color_t get_color_8_to_16(char *color_string) {
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

void init_rows(Row_s ***rows, size_t row_count) {
  *rows = (Row_s **)safe_malloc(row_count * sizeof(Row_s *));

  for (size_t i = 0; i < row_count; i++) {
    Row_s *curr_row = (Row_s *)safe_malloc(sizeof(Row_s));

    curr_row->left = (Text_s *)safe_malloc(sizeof(Text_s));
    curr_row->left->sections = NULL;
    curr_row->left->section_count = 0;
    curr_row->left->section_count_max = 0;

    curr_row->right = (Text_s *)safe_malloc(sizeof(Text_s));
    curr_row->right->sections = NULL;
    curr_row->right->section_count = 0;
    curr_row->right->section_count_max = 0;

    (*rows)[i] = curr_row;
  }
}

void free_rows(Row_s **rows, size_t row_count) {
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
