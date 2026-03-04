#include "../include/utils.h"
#include <stdlib.h>
#include <errno.h>

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
