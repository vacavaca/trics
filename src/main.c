#include "input.h"
#include "state.h"
#include "ui_interface.h"
#include <ncurses.h> // ncurses functions
#include <signal.h>  // signal
#include <stdbool.h>  // bool
#include <termios.h> // TC constants
#include <time.h>    // nanosleep
#include <unistd.h>  // STDIN_FILENO
#include <stdio.h>  // fprintf
#include <stdlib.h>  // exit

static WINDOW *win;

struct termios orig_termios;

void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_mouse_tracking(void) {
    printf("\x1B[?1000h");
}

void disable_mouse_tracking(void) {
    printf("\x1B[?1000l");
}

void handle_resize(int sig) {
    endwin();
    wclear(win);
    winsertln(win);
}

void handle_exit(void) {
    disable_raw_mode();
    endwin();
    disable_mouse_tracking();
}

void sleep_msec(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    struct timespec r;
    nanosleep(&ts, &r);
}

int main(int argc, char *argv[]) {
    State *state = state_init((char *)"Song Title");
    if (state == NULL) {
        fprintf(stderr, "Failed to initialize state\n");
        exit(1);
    }

    Interface *interface = interface_init(state);

    if (interface == NULL) {
        fprintf(stderr, "Failed to initialize interface\n");
        exit(1);
    }

    atexit(handle_exit);
    signal(SIGWINCH, handle_resize);

    enable_mouse_tracking();

    win = initscr();
    enable_raw_mode();
    curs_set(0);
    init_colors();

    int t = 0;
    Input *inputs = NULL;
    while (true) {
        sleep_msec(REFRESH_RATE_MSEC);

        int len = 0;
        inputs = input_read(&len);

        for (int i = 0; i < len; i++) {
            Input input = inputs[i];
            Input test = input_init_modified_key(MODIFIER_KEY_CTRL, 'c');
            if (input_eq(&input, &test)) {
                goto cleanup;
            }

            interface_handle_input(interface, &input);
        }

        free_input(inputs);
        inputs = NULL;

        interface_draw(win, interface, t * REFRESH_RATE_MSEC);
        wrefresh(win);
        t += 1;
    }

cleanup:
    if (inputs != NULL) {
        free_input(inputs);
    }
    interface_free(interface);
    state_free(state);
    return 0;
}
