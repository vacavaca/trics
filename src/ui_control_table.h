#ifndef UI_CONTROL_TABLE_H
#define UI_CONTROL_TABLE_H

#include "state.h"      // MAX_PATTERN_VOICES
#include "ui_base.h" // Cursor
#include "ui_control.h" // Control
#include "reflist.h" // RefList
#include <ncurses.h> // wmove wprintw
#include <stdbool.h> // bool
#include <stdlib.h>  // malloc
#include <string.h>  // memcpy
#include <assert.h>  // assert

#define MAX_TABLE_COLUMNS 8
#define MAX_TABLE_VISIBLE_ROWS 8

typedef struct
{
    char **headers;
    int column_count;
    RefList *rows;
    Rect rect;
    Control *focus;
    int focus_row;
    int focus_column;
    int offset;
    int highlight_row;
    WINDOW *win;
} ControlTable;

ControlTable *control_table_init(int column_count, int x, int y,
                                 int width, int height, char const **headers,
                                 int highlight_row);

bool control_table_add(ControlTable *table, Control const *row);

bool control_table_set(ControlTable *table, int n, Control const *row);

bool control_table_del(ControlTable *table, int n);

void control_table_clear(ControlTable *table);

void control_table_draw(WINDOW *win, ControlTable const *table, int draw_time);

void control_table_update(ControlTable const *table, int draw_time);

void control_table_refresh(ControlTable const *table);

void control_table_focus_first(ControlTable *table);

void control_table_focus_last(ControlTable *table);

bool control_table_focus_first_col(ControlTable *table, int y);

bool control_table_focus_add(ControlTable *table, int x, int y,
                             bool jump, Cursor *cursor);

bool control_table_focus_point(ControlTable *table, int x, int y, bool exact);

void control_table_focus_clear(ControlTable *table);

void control_table_free(ControlTable *table);

#endif // UI_CONTROL_TABLE_H
