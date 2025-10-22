#include "../include/meowhud.h"

static void init_state(MeowhudState *state) {
    state->compositor = NULL;
    state->layer_shell = NULL;
    state->shm = NULL;

    state->display = NULL;
    state->registry = NULL;
    state->surface = NULL;
    state->layer = NULL;
    state->buff = NULL;
    state->shm_pool = NULL;

    state->pixels = NULL;
    state->fd = -1;

    state->configured = false;
    state->skip = false;
    state->running = true;

    state->height = 500;
    state->width = 500;
    state->stride = 4 * state->width;
    state->shm_size = state->stride * state->height;
}

// gets and configures the layer, assumes state already ahs surface
static void get_and_configure_layer(MeowhudState *state) {
    state->layer = zwlr_layer_shell_v1_get_layer_surface(state->layer_shell, state->surface, NULL, ZWLR_LAYER_SHELL_V1_LAYER_OVERLAY, "");

    zwlr_layer_surface_v1_set_anchor(state->layer, ZWLR_LAYER_SURFACE_V1_ANCHOR_RIGHT | ZWLR_LAYER_SURFACE_V1_ANCHOR_TOP);
    zwlr_layer_surface_v1_set_size(state->layer, state->width, state->height);
    wl_surface_commit(state->surface);
}

static void init_hud(MeowhudState *state) {
    //  gets display
    state->display = wl_display_connect(NULL);
    if (!(state->display)) {
        fprintf(stderr, "Failed to establish connection with display :( !\n");
        exit(EXIT_FAILURE);
    }
    printf("YES DISPLAY!\n");

    // gets registry
    state->registry = wl_display_get_registry(state->display);
    if (!(state->registry)) {
        fprintf(stderr, "Failed to get registery!\n");
        exit(EXIT_FAILURE);
    }
    printf("YES REGISTRY\n");

    wl_registry_add_listener(state->registry, &registry_listener, state);
    wl_display_roundtrip(state->display);    // needs to get the compositor

    state->surface = wl_compositor_create_surface(state->compositor);
    get_and_configure_layer(state);
    
    // creates memory pool
    state->fd = memfd_create("name", 0); // creates a "file" in memory
    ftruncate(state->fd, state->shm_size); // sets teh size of the file

    state->shm_pool = wl_shm_create_pool(state->shm, state->fd, state->shm_size); // creates shared memory pool

    state->pixels = mmap(NULL, state->shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, state->fd, 0); // "mounts" the file descriptor onto a memory location so it will be writeable
    
    state->buff = wl_shm_pool_create_buffer(state->shm_pool, 0, state->width, state->height, state->stride, WL_SHM_FORMAT_ARGB8888);

    wl_surface_attach(state->surface, state->buff, 0, 0);

    zwlr_layer_surface_v1_add_listener(state->layer, &layer_surface_listener, state);
    wl_buffer_add_listener(state->buff, &buffer_listener, state);
}

int main(int argc, char* argv[]) {
    MeowhudState state;
    init_state(&state);
    init_hud(&state);

    memset(state.pixels, 0xff, state.shm_size);  // writes all 1s (i.e. not transparent and white)
    
    wl_display_roundtrip(state.display);
    wl_surface_commit(state.surface);


    int temp = 0;
    while (state.running) {
        if (state.configured) {
            memset(state.pixels, temp, state.shm_size);  // writes all 1s (i.e. not transparent and white)
            draw_bar(&state);
            temp += 10;
            temp = temp % 255;
        } 
        sleep(1);
    }

    return EXIT_FAILURE;
}
