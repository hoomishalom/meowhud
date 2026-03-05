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
