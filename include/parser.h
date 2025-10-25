#ifndef PARSER_H_INCLUDED
#define PARSER_H_INCLUDED

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "types.h"

#include "pixman.h"
#include "fcft/fcft.h"

#define DELIMITER ";"
#define DONE_MARKER "DONE"
#define LENGTH_OF_BG_COLOR 8
#define LENGTH_OF_TEXT_COLOR 8 
#define LENGTH_OF_ANCHOR 4

// calculates the appropriate stride value
int pixman_compute_stride(pixman_format_code_t format, int width);

void parse_options(MeowhudState *state);

#endif

