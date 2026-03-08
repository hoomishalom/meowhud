#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

#include "types.h"

// Parse the options block from stdin up to the "DONE" marker.
void parse_options(MeowhudState *state);

#endif
