#include "render.h"
#include "reflist.h" // RefList
#include <ncurses.h>

struct Renderer {
    Rect rect;
    Rect wrect;
    bool rendered;
    WINDOW *win;
};

void renderer_setup(void) {
    initscr();
    curs_set(0);

    start_color();
    init_pair(UI_COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(UI_COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(UI_COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(UI_COLOR_RED, COLOR_RED, COLOR_BLACK);
    init_pair(UI_COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(UI_COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(UI_COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
    init_pair(UI_COLOR_GREY, 8, COLOR_BLACK);
    init_pair(UI_COLOR_BRIGHT, 15, COLOR_BLACK);
    init_pair(UI_COLOR_INVERSE, 16, COLOR_WHITE);
}

void renderer_winch(void) {
    /*
    endwin();
    wclear(win);
    winsertln(win);
    */
}

Renderer *renderer_init(RendererParams *params, Rect rect) {
    WINDOW *win = newwin(rect.height, rect.width, rect.y, rect.x);
    if (win == NULL) {
        return NULL;
    }

    Renderer *renderer = malloc(sizeof(Renderer));
    if (renderer == NULL) {
        goto cleanup;
    }

    *renderer = (Renderer){
        .rect = rect,
        .wrect = rect,
        .rendered = false,
        .win = win
    };

    return renderer;

cleanup:
    delwin(win);
    return NULL;
}

void renderer_clear(Renderer *renderer) {
    wclear(renderer->win);
    wrefresh(renderer->win);
}

void renderer_move(Renderer *renderer, int x, int y) {
    renderer->rect.x = x;
    renderer->rect.y = y;
}

void renderer_render(Renderer *renderer, Text *text) {
    if (!renderer->rendered || !rect_eq(&renderer->rect, &renderer->wrect)) {
        mvwin(renderer->win, renderer->rect.y, renderer->rect.x);
        renderer->wrect = renderer->rect;
    }

    if (renderer->rendered) {
        wclear(renderer->win);
    }


    if (text->text != NULL) {
        if (text->bold) {
            attron(A_BOLD);
        }

        attron(COLOR_PAIR(text->color));

        wprintw(renderer->win, text->text);

        attroff(COLOR_PAIR(text->color));

        if (text->bold) {
            attroff(A_BOLD);
        }
    }

    wrefresh(renderer->win);

    renderer->rendered = true;
}

void renderer_free(Renderer *renderer) {
    delwin(renderer->win);
    free(renderer);
}

void renderer_teardown(void) {
    endwin();
}

