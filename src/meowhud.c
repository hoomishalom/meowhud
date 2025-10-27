#include "../include/meowhud.h"


int main(int argc, char *argv[]) {
  fcft_init(FCFT_LOG_COLORIZE_NEVER, false, FCFT_LOG_CLASS_NONE);

  MeowhudState state;
  init_state(&state);

  parse_options(&state);

  init_hud(&state);

  wl_display_roundtrip(state.display);
  wl_surface_commit(state.surface);

  struct pollfd poll_stdin;
  poll_stdin.fd = STDIN_FILENO;
  poll_stdin.events = POLLIN;

  struct pollfd poll_display;
  poll_display.fd = wl_display_get_fd(state.display);
  poll_display.events = POLLIN;

  struct pollfd pfds[2] = {poll_stdin, poll_display};

  while (state.running) {
    int ret = poll(pfds, 2, -1); // -1 means no timeout

    if (ret > 0) {
      // poll stdin
      if (pfds[0].revents & POLLIN) { // will be zero if POLLIN isnt one
        if (state.configured) {
          parse_frame(&state);

          render_bg(&state);
          render_rows(&state);

          draw_hud(&state);
        }
        pfds[0].events = POLLIN; // safer this way
      } else if (pfds[0].revents & POLLHUP) {
        fprintf(stderr, "stdin hung up\n");
        return EXIT_FAILURE;
      } else if (pfds[0].revents & POLLNVAL) {
        fprintf(stderr, "stdin not open\n");
        return EXIT_FAILURE;
      } else if (pfds[0].revents & POLLERR) {
        fprintf(stderr, "stdin hung up\n");
        return EXIT_FAILURE;
      }

      // poll display
      if (pfds[1].revents & POLLIN) {
        wl_display_dispatch(state.display);
      } else if (pfds[1].revents & POLLHUP) {
        fprintf(stderr, "display fd hung up\n");
        return EXIT_FAILURE;
      } else if (pfds[1].revents & POLLNVAL) {
        fprintf(stderr, "display fd not open\n");
        return EXIT_FAILURE;
      } else if (pfds[1].revents & POLLERR) {
        fprintf(stderr, "display fd hung up\n");
        return EXIT_FAILURE;
      }
      pfds[1].events = POLLIN; // safer this way
    } else if (ret == -1) { // Note: there is also a return value of 0, won't be reached due to no timeout
      fprintf(stderr, "poll failed\n");
      return EXIT_FAILURE;
    }
  }

  return EXIT_FAILURE;
}
