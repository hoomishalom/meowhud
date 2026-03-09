#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

#include "types.h"
#include "pixman.h"

#define DELIMITER ";"
#define DONE_MARKER "DONE"
#define RIGHT_ALIGNMENT "r"
#define LEFT_ALIGNMENT "l"
#define LENGTH_OF_BG_COLOR 8
#define LENGTH_OF_TEXT_COLOR 8 
#define LENGTH_OF_ANCHOR 4

// the minimum difference between the amount of allocated sections
// and used sections in a Text
#define DOWNSIZING_THRESHOLD 10

// calculates the appropriate stride value
uint32_t pixman_compute_stride(pixman_format_code_t format, int width);

void parse_options(MeowhudState *state);

bool parse_frame(MeowhudState *state);

#endif

