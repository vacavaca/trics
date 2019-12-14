#include "input.h"
#include "music.h"
#include "ui.h"
#include <ncurses.h> // ncurses functions
#include <signal.h>  // signal
#include <termios.h> // TC constants
#include <time.h>    // nanosleep
#include <unistd.h>  // STDIN_FILENO

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

    Song *song = song_init("Song Title");

    Interface *interface = interface_init(song);

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
    start_color();
    init_pair(1, COLOR_YELLOW, COLOR_BLACK);

    int t = 0;
    while (true) {
        sleep_msec(REFRESH_RATE_MSEC);

        Input input;
        if (input_read(&input)) {
            if (input.type == INPUT_TYPE_KEY && !input.key.special && input.key.modifier == MODIFIER_KEY_CTRL && input.key.ch == 'c') {
                return 0;
            }

            interface_handle_input(interface, &input);
            // size_t repr_len = strlen(repr);
            // if (repr_len > 0) {
            //     last_write = i;
            //     wclear(win);
            // }
            // wmove(win, 15, 25 - repr_len);
            // attron(COLOR_PAIR(1));
            // wprintw(win, repr);
            // attroff(COLOR_PAIR(1));
        }

        interface_draw(win, interface, t * REFRESH_RATE_MSEC);
        wmove(win, 25, 1);
        wprintw(win, "%d", t * REFRESH_RATE_MSEC);
        wrefresh(win);
        t += 1;
    }

    return 0;
}
