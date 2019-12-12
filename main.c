#include "music.h"
#include "ui.h"
#include <ncurses.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static WINDOW *win;

void resizeHandler(int sig) {
    endwin();
    wclear(win);
    winsertln(win);
}

int main(int argc, char *argv[]) {

    struct pollfd fd = {
        .fd = 0,
        .events = POLLIN};
    int i = 0;
    int cap = 10;
    while (1) {
        if (POLLIN == poll(&fd, 1, 0)) {
            size_t size = cap;
            size_t offset = 0;
            size_t total = 0;
            printf("malloc: %d\n", cap);
            char *buff = malloc(sizeof(char) * cap);
            while (size >= cap) {
                if (cap < offset + size) {
                    printf("realloc: %d\n", cap + offset);
                    realloc(buff, sizeof(char) * cap + offset * sizeof(char));
                }
                size = read(0, buff + offset * sizeof(char), cap);
                total += size;
                printf("read: %d\n", size);
                offset += cap;
            }
            printf("total: %d\n", total);

            char *realbuff = malloc(total);
            memcpy(realbuff, buff, total);
            printf("result: ");
            printf(realbuff);
            printf("\n");
            free(buff);
            buff = NULL;
        }
    }

    return 0;

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
    timeout(8);
    while (true) {
        int c = wgetch(win);
        if (c == KEY_RIGHT | KEY_ENTER) {
            interface_next_primary_tab(&interface);
        }
        if (c == KEY_LEFT | KEY_ENTER) {
            interface_prev_primary_tab(&interface);
        }
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
