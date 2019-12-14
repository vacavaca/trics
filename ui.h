#ifndef UI_H
#define UI_H

#include "input.h"   // Input
#include "music.h"   // Song
#include <assert.h>  // assert
#include <ncurses.h> // wmove wprintw
#include <stdbool.h> // bool
#include <stdlib.h>  // malloc
#include <string.h>  // memcpy

#define DEFAULT_LIST_CAPACITY 64
#define MAX_TEXT_WIDTH 24
#define MAX_TABLE_COLUMNS 8
#define REFRESH_RATE_MSEC 8

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
    UI_COLOR_BRIGHT
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

typedef enum {
    DECOR_NONE,
    DECOR_BOLD,
    DECOR_REC,
    DECOR_ROW_MARK,
    DECOR_TAB_MARK,
    DECOR_INVERT,
} Decor;

typedef struct
{
    ControlType type;
    union {
        volatile bool *const bool_value;
        volatile int *const int_value;
        char *const text_value;
    };
    bool focusable;
    Decor decor;
    int x;
    int y;
    int width;
} Control;

typedef struct
{
    char **headers;
    int column_count;
    RefList *rows;
    int x;
    int y;
    int width;
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
} Interface;

Interface *interface_init(Song *song);

void interface_draw(WINDOW *win, Interface *interface, int draw_time);

void interface_handle_input(Interface *interface, Input const *input);

void interface_fee(Interface *interface);

void init_colors(void);

#endif // UI_H