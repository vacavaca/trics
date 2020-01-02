#ifndef UI_BASE_H
#define UI_BASE_H

#include <stdbool.h> // bool

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
