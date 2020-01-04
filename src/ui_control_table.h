#ifndef UI_CONTROL_TABLE_H
#define UI_CONTROL_TABLE_H

#include "state.h"      // MAX_PATTERN_VOICES
#include "ui_base.h" // Cursor
#include "render.h" // Widget
#include "ui_control.h" // Control
#include "reflist.h" // RefList
#include <stdbool.h> // bool
#include <stdlib.h>  // malloc
#include <string.h>  // memcpy
#include <assert.h>  // assert

#define MAX_TABLE_COLUMNS 8
#define MAX_TABLE_VISIBLE_ROWS 8

typedef struct ControlTable ControlTable;

typedef struct {
    ControlTable *table;
    Control *row;
    int y;
    int x;
    int length;
    int column_count;
} ControlTableRow;

bool control_table_row_add_bool(ControlTableRow *row, volatile bool *value,
                                bool allow_empty, void (*on_change)(void *),
                                void *interface);

bool control_table_row_add_int(ControlTableRow *row, volatile int *value, bool allow_empty,
                               void (*on_change)(void *), void *interface,
                               int min, int max);

bool control_table_row_add_free_int(ControlTableRow *row, volatile int *value,
                                    bool allow_empty, void (*on_change)(void *),
                                    void *interface);

bool control_table_row_add_text(ControlTableRow *row,char **value, int width,
                          bool allow_empty, void (*on_change)(void *),
                          void *interface) {

bool control_table_row_add_note(ControlTableRow *row, volatile int *value, int *base_octave,
                                bool allow_empty, void (*on_change)(void *),
                                void *interface);

bool control_table_row_add_operator(volatile Operator *value,
                                    void (*on_change)(void *), void *interface);

void control_table_row_free(ControlTableRow *row);

struct ControlTable
{
    char **headers;
    int column_count;
    RefList *rows;
    Control *focus;
    int focus_row;
    int focus_column;
    int highlight_row;
    Widget *widget;
    bool creating_row;
};

ControlTable *control_table_init(int column_count, int x, int y,
                                 int width, int height, char const **headers,
                                 int highlight_row);

ControlTableRow *control_table_create_row(ControlTable *table);

bool control_table_add(ControlTable *table, ControlTableRow *row);

bool control_table_set(ControlTable *table, int n, ControlTableRow *row);

bool control_table_del(ControlTable *table, int n);

void control_table_clear(ControlTable *table);

void control_table_refresh(ControlTable const *table);

void control_table_update(ControlTable const *table, int draw_time);

void control_table_focus_first(ControlTable *table);

void control_table_focus_last(ControlTable *table);

bool control_table_focus_first_col(ControlTable *table, int y);

bool control_table_focus_add(ControlTable *table, int x, int y,
                             bool jump, Cursor *cursor);

bool control_table_focus_point(ControlTable *table, int x, int y, bool exact);

void control_table_focus_clear(ControlTable *table);

void control_table_free(ControlTable *table);

#endif // UI_CONTROL_TABLE_H
