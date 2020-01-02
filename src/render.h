#ifndef RENDER_H
#define RENDER_H

#include "util.h" // Rect
#include <stdbool.h> // bool

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
    Color fg;
    Color bg;
    bool bold;
    Rect rect;
} Text;

// Container

typedef struct {
    int scroll;
    Rect rect;
    RefList *children;
} Container;

Container *container_init(Rect rect);

void container_free(Container *container);

/*
 * Renderer
 *
 * This is the implementation dependent part
 */

typedef struct Renderer Renderer;

Renderer *renderer_init(Rect rect, ...);

void renderer_clear(Renderer *renderer);

void renderer_move(Renderer *renderer, Point position);

void renderer_render(Renderer *renderer, Text *text);

int renderer_get_updated_at(Renderer *renderer);

void renderer_free(Renderer *renderer);

// Widget

typedef struct Widget Widget;

typedef enum {
    WIDGET_TYPE_TEXT,
    WIDGET_TYPE_CONTAINER,
} WidgetType;

struct Widget {
    Renderer *renderer;
    Widget *parent;
    WidgetType type;
    union {
        Text text;
        Container container;
    };
}

Widget *widget_init_text(Widget *parent, Point position, char *text);

Widget *widget_init_container(Widget *parent, Rect rect);

void widget_refresh(Widget *widget);

void widget_free(Widget *widget);

#endif // RENDER_H
