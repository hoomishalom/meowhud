#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <pixman.h>
#include "types.h"

bool parse_uint32(const char *str, uint32_t *out, int base);
bool parse_int32(const char *str, int32_t *out, int base);
bool parse_uint64(const char *str, uint64_t *out, int base);

void *safe_malloc(size_t size);
void *safe_realloc(void *ptr, size_t size);
char *safe_strdup(const char *s);

pixman_color_t get_color_8_to_16(char *color_string);
void init_rows(Row_s ***rows, size_t row_count);
void free_rows(Row_s **rows, size_t row_count);

#endif
