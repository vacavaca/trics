#ifndef UI_BASE_H
#define UI_BASE_H

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

typedef struct{
    bool handled;
    bool done;
} InputResult;

typedef struct {
    int x;
    int y;
    bool jump;
} Cursor;

#endif // UI_BASE_H
