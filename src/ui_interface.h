#ifndef UI_INTERFACE_H
#define UI_INTERFACE_H

#include "ui_base.h"    // Color
#include "ui_control.h" // Control
#include "ui_layout.h"  // Layout
#include "input.h"      // Input
#include <stdlib.h>       // malloc, free

#define MAX_TEXT_WIDTH 24
#define REFRESH_RATE_MSEC 32
#define INPUT_REPR_HIDE_DELAY 0

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
    RefList *layouts;
    char *input_repr;
    int input_repr_length;
    int input_repr_printed_at;
    Layout *focus;
    Control *focus_control;
    Widget *tab_widget;
    Widget *sub_tab_widget;
    Widget *input_repr_widget;
    Widget *edit_widget;
} Interface;

Interface *interface_init(State * const state);

void interface_update(Interface *interface, int draw_time);

void interface_refresh(Interface *interface);

void interface_handle_input(Interface *interface, Input const *input);

void interface_free(Interface *interface);

void init_colors(void);

#endif // UI_INTERFACE_H
