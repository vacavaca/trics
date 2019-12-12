#ifndef UI_H
#define UI_H

#include <assert.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define DEFAULT_VEC_CAPACITY 64
#define MAX_TABLE_WIDTH 8

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

Control control_init_bool(volatile bool *const value);

Control control_init_int(volatile int *const value);

Control control_init_text(char *const value);

typedef struct
{
    char const **headers;
    int column_count;
    Control const **controls;
    int capacity;
    int length;
    int x;
    int y;
    int width;
} ControlTable;

int control_table_init(int column_count, int x, int y, int width,
                       char const **headers, ControlTable *table);

int control_table_add(ControlTable *table, Control const *row);

void control_table_free(ControlTable *table);

typedef enum {
    TAB_SONG,
    TAB_PATTERN,
    TAB_INSTRUMENT,
    TAB_WAVE,
    TAB_FILTER,
    TAB_ARPEGGIO,
} Tab;

const Tab PRIMARY_TABS[4];

const Tab SECONDARY_TABS[3];

typedef struct
{
    Tab selected_tab;
    bool continuation;
    ControlTable *tables;
    Control *focus;
    int tables_count;
    int capacity;
    bool show_filename;
    char const *filename;
} Interface;

int interface_init(Interface *interface);

int interface_add_table(Interface *interface, ControlTable table);

void interface_next_primary_tab(Interface *interface);

void interface_prev_primary_tab(Interface *interface);

int interface_draw(WINDOW *win, Interface *interface);

int interface_fee(Interface *interface);

#endif // UI_H