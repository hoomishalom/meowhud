#ifndef INITIALIZERS_H_INCLUDED
#define INITIALIZERS_H_INCLUDED

#define _GNU_SOURCE
#include <sys/mman.h>

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h> 

#include <fcft/fcft.h>
#include <locale.h>

#include <wayland-client.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#include "pixman.h"

#include "types.h"
#include "listeners.h"
#include "parser.h"

void init_state(MeowhudState *state);

void init_hud(MeowhudState *state);

#endif
