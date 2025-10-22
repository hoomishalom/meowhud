#ifndef MOEWLINE_H_INCLUDED
#define MOEWLINE_H_INCLUDED

#define _GNU_SOURCE  

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <unistd.h>
#include <poll.h>
#include <sys/mman.h> 
#include <wlr/types/wlr_layer_shell_v1.h>
#include <wayland-client.h>
#include <wayland-client-protocol.h>
#include "../include/listeners.h"

#define WIDTH 500
#define HEIGHT 500
#define STRIDE (4 * WIDTH)
#define SHM_SIZE (STRIDE * HEIGHT)

typedef struct {
    uint8_t a;
    uint8_t r;
    uint8_t g;
    uint8_t b;
} Color;


typedef struct meowhud_state {
    // Wayland Globals
    struct wl_compositor *compositor;
    struct zwlr_layer_shell_v1 *layer_shell;
    struct wl_shm *shm;

    // Wayland Objects
    struct wl_display *display;
    struct wl_registry *registry;
    struct wl_surface *surface;
    struct zwlr_layer_surface_v1 *layer;
    struct wl_buffer *buff;
    struct wl_shm_pool *shm_pool;
    
    // Application Data
    Color *pixels;
    int fd;

    // State Flags
    bool configured;
    bool skip;
} MeowhudState;

#endif
