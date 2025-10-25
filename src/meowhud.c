#include "../include/meowhud.h"


int main(int argc, char *argv[]) {
  fcft_init(FCFT_LOG_COLORIZE_NEVER, false, FCFT_LOG_CLASS_NONE);

  MeowhudState state;
  init_state(&state);

  parse_options(&state);

  init_hud(&state);


  wl_display_roundtrip(state.display);
  wl_surface_commit(state.surface);

  // Test
  int count = 0;
  char32_t *text_l = U"abcd";
  char32_t *text_r = U"efghi";

  TextSection_s section1 = {
    text_l,
    4,
    state.default_text_color,
  };

  TextSection_s section2 = {
    text_r,
    5,
    state.default_text_color,
  };

  TextSection_s *sections1[] = {&section1, &section2};
  TextSection_s *sections2[] = {&section2};

  Text_s text1 = {
    sections1,
    2
  };

  Text_s text2 = {
    sections2,
    1
  };

  Row_s row = {
    &text1,
    &text2
  };

  state.content_rows = (Row_s **)malloc(5 * sizeof(Row_s *));
  state.content_rows[0] = &row;
  state.content_rows[1] = &row;
  state.content_rows[2] = &row;
  state.content_rows[3] = &row;
  state.content_rows[4] = &row;
  state.row_count = 5;

  // End Test

  while (state.running) {
    if (state.configured) {
      render_bg(&state);
      render_rows(&state);

      draw_bar(&state);

      wl_display_dispatch(state.display);
    }
    usleep(50000);
  }

  return EXIT_FAILURE;
}
