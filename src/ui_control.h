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
    CONTROL_TYPE_OPERATOR,
} ControlType;

typedef struct {
    volatile bool *value;
    int edit_value;
    char *edit_text;
    bool edit;
    bool reseted;
} ControlBool;

typedef struct {
    volatile int *value;
    int edit_value;
    char *edit_text;
    bool allow_empty;
    bool edit;
    bool reseted;
    int min;
    int max;
} ControlInt;

typedef struct {
    char **value;
    char *edit_value;
    bool allow_empty;
    bool edit;
    bool reseted;
    int width;
    int start;
} ControlText;

typedef struct {
    volatile int *value;
    int *base_octave;
    int edit_value;
    char *edit_text;
    bool allow_empty;
    bool edit;
} ControlNote;

typedef struct {
    volatile Operator *value;
    Operator edit_value;
    char edit_text;
    bool edit;
} ControlOperator;

typedef struct
{
    ControlType type;
    union {
        ControlBool control_bool;
        ControlInt control_int;
        ControlText control_text;
        ControlNote control_note;
        ControlOperator control_operator;
    };
    Rect rect;
    bool focus;
    bool edit;
    int focused_at;
    void (*on_change)(void *); // self
    void *interface;
    WINDOW *win;
    int draw_time;
} Control;

Control control_init_bool(volatile bool *value, bool allow_empty,
                          void (*on_change)(void *), void *interface);

Control control_init_int(volatile int *value, bool allow_empty,
                         void (*on_change)(void *), void *interface,
                         int min, int max);

Control control_init_free_int(volatile int *value, bool allow_empty,
                              void (*on_change)(void *), void *interface);

Control control_init_text(char **value, int width, bool allow_empty,
                          void (*on_change)(void *), void *interface);

Control control_init_note(volatile int *value, int *base_octave,
                          bool allow_empty, void (*on_change)(void *),
                          void *interface);

Control control_init_operator(volatile Operator *value,
                              void (*on_change)(void *), void *interface);

char *control_repr(Control *control);

void control_refresh(Control *control);

void control_update(Control *control, int time);

void control_draw(WINDOW *win, Control *control);

void control_focus(Control *control);

void control_discard_edit(Control *control);

void control_focus_clear(Control *control);

bool control_edit(Control *control);

bool control_handle_step_input(Control *control, int i);

bool control_handle_multiplier_input(Control *control, double i);

InputResult control_handle_input(Control *control, char key);

bool control_handle_wheel_input(Control *control, Point const *point, int i);

bool control_empty(Control* control);

void control_save_edit(Control *control);

void control_free(Control *control);

#endif // UI_CONTROL_H
