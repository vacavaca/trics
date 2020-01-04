#ifndef RENDER_H
#define RENDER_H

#include "util.h" // Rect
#include "reflist.h" // RefList
#include <stdbool.h> // bool
#include <assert.h> // assert

typedef enum {
    UI_COLOR_BLUE = 1,
    UI_COLOR_GREEN,
    UI_COLOR_CYAN,
    UI_COLOR_RED,
    UI_COLOR_MAGENTA,
    UI_COLOR_YELLOW,
    UI_COLOR_WHITE,
    UI_COLOR_GREY,
    UI_COLOR_BRIGHT,
    UI_COLOR_INVERSE,
} Color;

// Text

typedef struct {
    char *text;
    Color color;
    bool bold;
} Text;


typedef struct Widget Widget;

// Container

typedef struct {
    int scroll;
    int rscroll;
    RefList *children;
    bool rendered;
} Container;

Container *container_init(void);

bool container_add(Container *container, Widget *widget);

void container_remove(Widget *widget);

void container_reset_scroll(Container *container);

void container_scroll(Container *container, int n);

void container_scroll_to_last(Container *container);

void container_scroll_to_position(Container *container, int y);

void container_move(Container *container, int x, int y);

void container_refresh(Container *container, Rect const *rect);

void container_free(Container *container);

/*
 * Renderer
 *
 * This is the implementation dependent part
 */

typedef struct Renderer Renderer;

typedef struct RendererParams RendererParams;

void renderer_setup(void);

void renderer_winch(void);

Renderer *renderer_init(RendererParams *params, Rect rect);

void renderer_clear(Renderer *renderer);

void renderer_move(Renderer *renderer, int x, int y);

void renderer_render(Renderer *renderer, Text *text);

void renderer_free(Renderer *renderer);

void renderer_teardown(void);

// Widget

typedef enum {
    WIDGET_TYPE_TEXT,
    WIDGET_TYPE_CONTAINER,
} WidgetType;

struct Widget {
    Renderer *renderer;
    Widget *parent;
    WidgetType type;
    Rect rect;
    union {
        Text text;
        Container *container;
    };
    int refreshed_at;
};

Widget *widget_init_text(RendererParams *params, Widget *parent,
                         Rect rect, char *txt);

Widget *widget_init_container(RendererParams *params, Widget *parent,
                              Rect rect);

void widget_move(Widget *widget, int x, int y);

void widget_clear(Widget *widget);

void widget_refresh(Widget *widget);

void widget_free(Widget *widget);

#endif // RENDER_H
