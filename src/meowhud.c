#include <assert.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

#include <fcft/fcft.h>
#include <locale.h>

#include <wayland-client.h>
#include <wlr/types/wlr_layer_shell_v1.h>

#include "../include/draw.h"
#include "../include/types.h"
#include "../include/initializers.h"
#include "../include/parser.h"

static volatile sig_atomic_t keep_running = 1;

static void handle_signal(int sig) {
  (void)sig; // Silence compiler warnings

  keep_running = 0;
}

int main(int argc, char *argv[]) {
  (void)argc; // Silence compiler warnings
  (void)argv; // Silence compiler warnings

  fcft_init(FCFT_LOG_COLORIZE_NEVER, false, FCFT_LOG_CLASS_NONE);

  struct sigaction sa;
  sa.sa_handler = handle_signal;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGTERM, &sa, NULL);

  MeowhudState state;
  init_state(&state);

  parse_options(&state);

  // Set stdin to non-blocking mode for frame reading
  int flags = fcntl(STDIN_FILENO, F_GETFL, 0);
  fcntl(STDIN_FILENO, F_SETFL, flags | O_NONBLOCK);

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

  int exit_code = EXIT_SUCCESS;

  while (state.running && keep_running) {
    int ret = poll(pfds, 2, -1); // -1 means no timeout

    if (ret > 0) {
      // poll stdin
      if (pfds[0].revents & POLLIN) { // will be zero if POLLIN isnt one
        if (state.configured) {
          if (parse_frame(&state)) {
            render_bg(&state);
            render_rows(&state);

            draw_hud(&state);
          }
        }
        pfds[0].events = POLLIN; // safer this way
      } else if (pfds[0].revents & POLLHUP) {
        fprintf(stderr, "stdin hung up\n");
        exit_code = EXIT_FAILURE;
        break;
      } else if (pfds[0].revents & POLLNVAL) {
        fprintf(stderr, "stdin not open\n");
        exit_code = EXIT_FAILURE;
        break;
      } else if (pfds[0].revents & POLLERR) {
        fprintf(stderr, "stdin hung up\n");
        exit_code = EXIT_FAILURE;
        break;
      }

      // poll display
      if (pfds[1].revents & POLLIN) {
        wl_display_dispatch(state.display);
      } else if (pfds[1].revents & POLLHUP) {
        fprintf(stderr, "display fd hung up\n");
        exit_code = EXIT_FAILURE;
        break;
      } else if (pfds[1].revents & POLLNVAL) {
        fprintf(stderr, "display fd not open\n");
        exit_code = EXIT_FAILURE;
        break;
      } else if (pfds[1].revents & POLLERR) {
        fprintf(stderr, "display fd hung up\n");
        exit_code = EXIT_FAILURE;
        break;
      }
      pfds[1].events = POLLIN; // safer this way
    } else if (ret == -1) { // Note: there is also a return value of 0, won't be reached due to no timeout
      if (errno == EINTR) {
        continue;
      }
      fprintf(stderr, "poll failed\n");
      exit_code = EXIT_FAILURE;
      break;
    }
  }

  cleanup_state(&state);
  return exit_code;
}
