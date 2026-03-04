#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

bool parse_uint32(const char *str, uint32_t *out, int base);
bool parse_int32(const char *str, int32_t *out, int base);
bool parse_uint64(const char *str, uint64_t *out, int base);

#endif
