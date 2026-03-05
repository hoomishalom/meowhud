#ifndef INITIALIZERS_H_INCLUDED
#define INITIALIZERS_H_INCLUDED

#include "types.h"

void init_state(MeowhudState *state);

void init_hud(MeowhudState *state);

void init_buffer(MeowhudState *state);

void cleanup_state(MeowhudState *state);

#endif
