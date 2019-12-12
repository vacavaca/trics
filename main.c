#include "music.h"
#include "ui.h"
#include <ncurses.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

static WINDOW *win;

void resizeHandler(int sig) {
    endwin();
    wclear(win);
    winsertln(win);
}

#define BLOCK_SIZE 17

int main(int argc, char *argv[]) {

    struct pollfd fd = {
        .fd = 0,
        .events = POLLIN};

    char *buff = NULL;
    bool print = false;
    size_t len = 0;
 struct termios raw;
  tcgetattr(STDIN_FILENO, &raw);
  raw.c_lflag &= ~(ECHO);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    while(1) {
        if (POLLIN == poll(&fd, 1, 0)) {
            char tmp[BLOCK_SIZE];
            size_t read_size = read(0, tmp, BLOCK_SIZE - 1);
            tmp[BLOCK_SIZE - 1] = 0;

            len += read_size / sizeof(char);
            if (buff == NULL) {
                buff = malloc(sizeof(char) * (read_size + 1));
                buff[0] = 0;
            } else {
                buff = realloc(buff, sizeof(char) * (len + 1));
            }
            strcat(buff, tmp);
            buff[len] = 0;
            if (read_size < BLOCK_SIZE - 1) {
                print = true;
            } else {
                print = false;
            }
        }
 
        if (print) {
            printf("print %d\n", len);
            print = false;
            printf("result: ");
            for (int i = 0; i < len; i ++) {
                printf("%d ", buff[i]);
            }
            printf("\n");
            free(buff);
            len = 0;
            buff = NULL;
        }
    }

    Interface interface;

    if (!interface_init(&interface)) {
        fprintf(stderr, "Failed to initialize interface\n");
        exit(1);
    }

    win = initscr();
    signal(SIGWINCH, resizeHandler);
    cbreak();
    noecho();
    keypad(stdscr, TRUE);
    curs_set(0);
    //timeout(8);
    timeout(500);
    while (true) {
        int c = wgetch(win);

        wmove(win, 0, 0);
        wprintw(win, "%d\n", c);
        if (!interface_draw(win, &interface)) {
            endwin();
            fprintf(stderr, "Failed to draw interface\n");
            return 1;
        }
        wrefresh(win);
    }

    endwin();

    return 0;
}
