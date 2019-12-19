#ifndef UI_CONTROL_H
#define UI_CONTROL_H

#include "state.h" // EMPTY
#include "ui_base.h" // InputResult
#include "util.h" // Rect
#include <stdbool.h> // bool
#include <ncurses.h> // ncurses
#include <stdlib.h> // malloc
#include <string.h> // memcpy
#include <errno.h> // errno
#include <math.h> // round

#define CURSOR_BLINK_RATE_MSEC 250

typedef enum {
    CONTROL_TYPE_BOOL,
    CONTROL_TYPE_INT,
    CONTROL_TYPE_TEXT,
    CONTROL_TYPE_NOTE,
    CONTROL_TYPE_OP,
    CONTROL_TYPE_SELF_INT
} ControlType;

typedef struct
{
    ControlType type;
    union {
        volatile bool *bool_value;
        volatile int *int_value;
        char ** text_value;
        int self_int_value;
    };
    char *edit_value;
    Rect rect;
    bool focus;
    int focused_at;
    bool edit;
    bool text_edit_reseted;
    bool num_edit_reseted;
    bool allow_empty;

    void (*on_edit)(void *); // self
    void *layout;
} Control;

Control control_init_bool(volatile bool *const value, bool allow_empty);

Control control_init_int(volatile int *const value, bool allow_empty);

Control control_init_text(char **const value, int width, bool allow_empty);

Control control_init_self_int(int value, bool allow_empty, void *layout,
                              void (*on_edit)(void *));

char *control_repr(Control* control, bool cut, bool ellipsis);

void control_draw(WINDOW *win, Control *control, int draw_time);

void control_focus(Control *control);

void control_focus_clear(Control *control);

bool control_edit(Control *control);

void control_handle_step_input(Control *control, int i);

void control_handle_multiplier_input(Control *control, double i);

InputResult control_handle_input(Control *control, char key);

bool control_handle_wheel_input(Control *control, Point const *point, int i);

void control_save_edit(Control *control);

void control_discard_edit(Control *control);

bool control_empty(Control* control);

void control_free(Control *control);

#endif // UI_CONTROL_H
