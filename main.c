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

#define BLOCK_SIZE 17

int main(int argc, char *argv[]) {

    struct pollfd fd = {
        .fd = 0,
        .events = POLLIN};

    char *buff = NULL;
    bool print = false;
    size_t len = 0;
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
            printf(buff);
            free(buff);
            len = 0;
            buff = NULL;
        }
    }
    return 0;
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
