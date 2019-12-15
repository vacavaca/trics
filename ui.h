#ifndef UI_H
#define UI_H

#include "input.h"   // Input
#include "music.h"   // Song
#include "util.h"    // Rect, sign
#include <assert.h>  // assert
#include <ncurses.h> // wmove wprintw
#include <stdbool.h> // bool
#include <stdlib.h>  // malloc
#include <string.h>  // memcpy

#define DEFAULT_LIST_CAPACITY 64
#define MAX_TEXT_WIDTH 24
#define MAX_TABLE_COLUMNS 8
#define MAX_TABLE_VISIBLE_ROWS 8
#define REFRESH_RATE_MSEC 32
#define CURSOR_BLINK_RATE_MSEC 250
#define INPUT_REPR_HIDE_DELAY 600

typedef enum
{
    UI_COLOR_BLUE=1,
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

typedef struct {
    size_t item_size;
    int cap;
    int length;
    void **array;
} RefList;

typedef enum {
    CONTROL_TYPE_BOOL,
    CONTROL_TYPE_INT,
    CONTROL_TYPE_TEXT
} ControlType;

typedef struct
{
    ControlType type;
    union {
        volatile bool *const bool_value;
        volatile int *const int_value;
        char *const text_value;
    };
    Rect rect;
    bool focus;
    int focused_at;
} Control;

typedef struct
{
    char **headers;
    int column_count;
    RefList *rows;
    Rect rect;
    Control* focus;
    int focus_row;
    int focus_column;
    int offset;
} ControlTable;

typedef enum {
    TAB_SONG,
    TAB_PATTERN,
    TAB_INSTRUMENT,
    TAB_WAVE,
    TAB_FILTER,
    TAB_ARPEGGIO,
} Tab;

typedef struct {
    RefList *tables;
    ControlTable* focus;
} Layout;

const Tab PRIMARY_TABS[4];

const Tab SECONDARY_TABS[3];

typedef struct
{
    Tab selected_tab;
    RefList *layouts;
    char *input_repr;
    int input_repr_length;
    int input_repr_printed_at;
    Layout* focus;
    Control* focus_control;
} Interface;

Interface *interface_init(Song *song);

void interface_draw(WINDOW *win, Interface *interface, int draw_time);

void interface_handle_input(Interface *interface, Input const *input);

void interface_fee(Interface *interface);

void init_colors(void);

#endif // UI_H