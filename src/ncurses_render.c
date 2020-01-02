#include "render.h"
#include "reflist.h" // RefList
#include <ncurses.h>

struct Renderer {
    Renderer *parent;
    Rect rect;
    RefList *children;
    int updated_at;
    WINDOW *win;
};

Renderer *renderer_init(Renderer *parent, Rect rect, ...) {
    WINDOW *win = newwin(rect.height, rect.width, rect.y, rect.x);
    if (win == NULL) {
        goto cleanup_ap;
    }

    RefList *children = ref_list_init();
    if (children == NULL) {
        goto cleanup_win;
    }

    Renderer *renderer = malloc(sizeof(Renderer));
    if (renderer == NULL) {
        goto cleanup_children;
    }

    *renderer = (Renderer){
        .parent = parent,
        .rect = rect,
        .children = children,
        .updated_at = -1,
        .win = win
    };

    if (parent != NULL && !ref_list_add(parent->children, renderer)) {
        goto cleanup;
    }

    va_end(ap);
    return renderer;

cleanup:
    free(renderer);
cleanup_children:
    ref_list_free(children);
cleanup_win:
    delwin(win);
cleanup_ap:
    va_end(ap);
    return NULL;
}

void renderer_clear(Renderer *renderer) {
    wclear(renderer->win);
}

void renderer_print(Renderer *renderer, char *str) {
    wprintw(renderer->win, str);
}

void renderer_scroll(Renderer *renderer, int n) {

}

void renderer_refresh(Renderer *renderer, int time) {
    renderer->updated_at = time;
    wrefresh(renderer->win);
}

int renderer_get_updated_at(Renderer *renderer) {
    return renderer->updated_at;
}

void renderer_free(Renderer *renderer) {
    if (renderer->parent != NULL) {
        Renderer *parent = renderer->parent;
        for (int i = 0; i < parent->children->length; i ++) {
            if (ref_list_get(parent->children, i) == renderer) {
                ref_list_del(parent->children, i);
                break;
            }
        }
    }

    for (int i = 0; i < renderer->children->length; i ++) {
        renderer_free(ref_list_get(renderer->children, i);
    }

    ref_list_free(renderer->children);
    delwin(renderer->win);
    free(renderer);
}
