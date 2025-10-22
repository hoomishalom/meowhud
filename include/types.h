#ifndef STATE_H_INCLUDED
#define STATE_H_INCLUDED

#include <stdint.h>
#include <stdbool.h>

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
    bool running;

    // Surface Settings
    int width;
    int height;
    int stride;
    int shm_size;

} MeowhudState;

#endif
