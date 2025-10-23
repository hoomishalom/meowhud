#include "../include/meowhud.h"


int main(int argc, char *argv[]) {
  fcft_init(FCFT_LOG_COLORIZE_NEVER, false, FCFT_LOG_CLASS_NONE);

  MeowhudState state;
  init_state(&state);
  init_hud(&state);

  wl_display_roundtrip(state.display);
  wl_surface_commit(state.surface);

  int temp = 0; 
  int temp1 = 0; 
  int count = 0;
  while (state.running) {
    if (state.configured) {
      memset(state.mmapped, state.bg, 4 * state.width * state.height); // writes all 1s (i.e. not transparent and white)
      char32_t *text_l = U"left";
      char32_t *text_r = U"right";
      render_chars(text_l, state.pix_img, 4, 0, 100, state.width, false, NULL, state.font);
      render_chars(text_r, state.pix_img, 5, state.width, 100, state.width, true, NULL, state.font);

      temp += 50;
      temp1 += 60;
      temp %= state.width; 
      temp1 %= state.height; 

      count++;

      render_bar(&state);
      printf("count:%d\n", count);
      wl_display_dispatch(state.display);
    }
    usleep(50000);
  }

  return EXIT_FAILURE;
}
