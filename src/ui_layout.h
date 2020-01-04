#ifndef UI_LAYOUT_H
#define UI_LAYOUT_H

#include "ui_control_table.h" // ControlTable
#include "reflist.h" // RefList
#include <stdbool.h> // bool
#include <stdlib.h>  // malloc
#include "state.h"   // Song, State

typedef struct {
    RefList *tables;
    ControlTable *focus;
    State *state;
} Layout;

Layout *layout_init(State *const state);

bool layout_add_table(Layout *layout, ControlTable *table);

void layout_refresh(Layout *layout);

void layout_update(Layout *layout, int draw_time);

void layout_focus_clear(Layout *layout);

void layout_focus_first(Layout *layout);

void layout_focus_last(Layout *layout);

void layout_focux_first_col_or_same(Layout *layout, int x, int y);

void layout_focus_point(Layout *layout, int x, int y, bool exact);

void layout_focus_add(Layout *layout, int x, int y, bool jump);

void layout_free(Layout *layout);

#endif // UI_LAYOUT_H
