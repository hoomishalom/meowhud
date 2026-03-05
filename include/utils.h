#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

bool parse_uint32(const char *str, uint32_t *out, int base);
bool parse_int32(const char *str, int32_t *out, int base);
bool parse_uint64(const char *str, uint64_t *out, int base);

void *safe_malloc(size_t size);
void *safe_realloc(void *ptr, size_t size);
char *safe_strdup(const char *s);

#endif
