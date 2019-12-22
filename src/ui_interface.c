#include "ui_interface.h"

const Tab PRIMARY_TABS[4] = {
    TAB_SONG,
    TAB_PATTERN,
    TAB_INSTRUMENT,
    TAB_ARPEGGIO};

const Tab SECONDARY_TABS[3] = {
    TAB_INSTRUMENT,
    TAB_WAVE,
    TAB_FILTER};

bool interface_set_layout(Interface *interface, int tab, Layout *layout) {
    return ref_list_set(interface->layouts, tab, layout);
}

ControlTable *init_song_params_table(Layout *layout, Song *const song) {
    char const *song_params_headers[3] = {"bp", "st", "title"};
    ControlTable *song_params_table = control_table_init(3, 2, 3, 23, 2,
                                                         song_params_headers, 0);
    if (song_params_table == NULL) {
        return NULL;
    }

    Control song_param_controls[3] = {
        control_init_int(&song->bpm, false, NULL, layout),
        control_init_int(&song->step, false, NULL, layout),
        control_init_text(&song->name, 17, true, NULL, layout),
    };

    if (!control_table_add(song_params_table, song_param_controls)) {
        control_table_free(song_params_table);
        return NULL;
    }

    return song_params_table;
}

ControlTable *init_song_arrangement_table(Layout *layout, Song *const song) {
    ControlTable *arrangement_table = control_table_init(8, 2, 6, 23, 9, NULL, 0);
    if (arrangement_table == NULL) {
        return NULL;
    }

    for (int i = 0; i < MAX_SONG_LENGTH; i++) {
        Control arrangement_row[8] = {
            control_init_int(&song->patterns[i][0], true, NULL, layout),
            control_init_int(&song->patterns[i][1], true, NULL, layout),
            control_init_int(&song->patterns[i][2], true, NULL, layout),
            control_init_int(&song->patterns[i][3], true, NULL, layout),
            control_init_int(&song->patterns[i][4], true, NULL, layout),
            control_init_int(&song->patterns[i][5], true, NULL, layout),
            control_init_int(&song->patterns[i][6], true, NULL, layout),
            control_init_int(&song->patterns[i][7], true, NULL, layout),
        };

        if (!control_table_add(arrangement_table, arrangement_row)) {
            control_table_free(arrangement_table);
            return NULL;
        }
    }

    return arrangement_table;
}

Layout *init_song_layout(Song *const song) {
    Layout *layout = layout_init(NULL);
    if (layout == NULL) {
        return NULL;
    }

    ControlTable *params_table = init_song_params_table(layout, song);
    if (params_table == NULL) {
        goto cleanup_layout;
    }

    if (!layout_add_table(layout, params_table)) {
        goto cleanup_song_table;
    }

    ControlTable *arrangement = init_song_arrangement_table(layout, song);
    if (arrangement == NULL) {
        goto cleanup_song_table;
    }

    if (!layout_add_table(layout, arrangement)) {
        goto cleanup_arrangement_table;
    }

    return layout;

cleanup_arrangement_table:
    control_table_free(arrangement);
cleanup_song_table:
    control_table_free(params_table);
cleanup_layout:
    layout_free(layout);
    return NULL;
}

void pattern_table_auto_set_insrument(Layout *layout, ControlTable *table,
                                     int x, int y) {
    State *state = layout->state;
    int in = state->vars[STATE_VAR_INSTRUMENT] - 1;
    for (int i = 0; i < state->song->step - 1; i++) {
        Control *row = ref_list_get(table->rows, i);
        for (int j = 0; j < MAX_PATTERN_VOICES; j++) {
            Control *control = &row[j * 3];
            if (control->rect.y == y && control->rect.x == x - 3 &&
                *control->control_int.value == EMPTY) {
                *control->control_int.value = in + 1;
            }
        }
    }
}

void handle_control_note_insert(void *self) {
    Control *control = self;
    Layout *layout = control->layout;
    ControlTable *pattern_table = ref_list_get(layout->tables, 1);
    pattern_table_auto_set_insrument(layout, pattern_table,
                                     control->rect.x, control->rect.y);
}

bool pattern_table_update_pattern(Layout *layout, ControlTable *table) {
    State *state = layout->state;
    int n = state->vars[STATE_VAR_PATTERN] - 1;
    while (n >= state->patterns->length) {
        if (state_create_pattern(state) == -1) {
            return false;
        }
    }

    Pattern *pattern = ref_list_get(state->patterns, n);
    if (pattern == NULL) {
        return false;
    }

    int in = state->vars[STATE_VAR_INSTRUMENT] - 1;
    Instrument *instrument = ref_list_get(state->instruments, in);
    if (instrument == NULL) {
        return false;
    }

    int *octave = (int *)&instrument->octave;

    for (int i = 0; i < state->song->step - 1; i++) {
        Control row[3 * MAX_PATTERN_VOICES];
        for (int j = 0; j < MAX_PATTERN_VOICES; j++) {
            volatile Step *step = &pattern->steps[i][j];
            row[0 + j * 3] = control_init_int(&step->instrument, true,
                                              NULL, layout);

            row[1 + j * 3] = control_init_note(&step->note, octave,
                                               true, handle_control_note_insert, layout);

            row[2 + j * 3] = control_init_int(&step->arpeggio, true,
                                              NULL, layout);
        }

        if (!control_table_set(table, i, row)) {
            return false;
        }
    }

    return true;
}

void handle_control_select_pattern(void *self) {
    Control *control = self;
    Layout *layout = control->layout;
    ControlTable *pattern_table = ref_list_get(layout->tables, 1);
    pattern_table_update_pattern(layout, pattern_table);
}

void pattern_table_transpose(Layout *layout, ControlTable *table) {
    State *state = layout->state;
    for (int i = 0; i < state->song->step - 1; i++) {
        Control *row = ref_list_get(table->rows, i);
        for (int j = 0; j < MAX_PATTERN_VOICES; j++) {
            Control *control = &row[j * 3 + 1];
            int note = *control->control_note.value;
            int d = 12 * (state->vars[STATE_VAR_TRANSPOSE + j] - 5);
            if (d != 0 && note != EMPTY && note != NONE) {
                note += d;
                if (note <= EMPTY) {
                    note = EMPTY;
                }

                if (note > 109) {
                    note = 109;
                }

                *control->control_note.value = note;
            }
        }
    }

    for (int i = 0; i < MAX_PATTERN_VOICES; i++) {
        state->vars[STATE_VAR_TRANSPOSE + i] = 5;
    }
}

void handle_control_transpose(void *self) {
    Control *control = self;
    Layout *layout = control->layout;
    ControlTable *pattern_table = ref_list_get(layout->tables, 1);
    pattern_table_transpose(layout, pattern_table);
}

ControlTable *init_pattern_params_table(Layout *layout) {
    char const *headers[3] = {"##", "t1", "t2"};
    State *state = layout->state;
    ControlTable *table = control_table_init(3, 2, 3, 8, 2, headers, 0);
    if (table == NULL) {
        return NULL;
    }

    Control controls[3] = {
        control_init_int(&state->vars[STATE_VAR_PATTERN], false,
                         handle_control_select_pattern, layout),
        control_init_int(&state->vars[STATE_VAR_TRANSPOSE], false,
                         handle_control_transpose, layout),
        control_init_int(&state->vars[STATE_VAR_TRANSPOSE + 1], false,
                         handle_control_transpose, layout),
    };

    if (!control_table_add(table, controls)) {
        control_table_free(table);
        return NULL;
    }

    return table;
}

ControlTable *init_pattern_table(Layout *layout) {
    char const *headers[6] = {"in", "nt", "ar", "in", "nt", "ar"};
    ControlTable *table = control_table_init(6, 2, 6, 23, 9, headers, 4);
    if (table == NULL) {
        return NULL;
    }

    if (!pattern_table_update_pattern(layout, table)) {
        control_table_free(table);
        return NULL;
    }

    return table;
}

Layout *init_pattern_layout(State *const state) {
    Layout *layout = layout_init(state);
    if (layout == NULL) {
        return NULL;
    }

    ControlTable *params_table = init_pattern_params_table(layout);
    if (params_table == NULL) {
        goto cleanup_layout;
    }

    if (!layout_add_table(layout, params_table)) {
        goto cleanup_params_table;
    }

    ControlTable *pattern_table = init_pattern_table(layout);
    if (pattern_table == NULL) {
        goto cleanup_params_table;
    }

    if (!layout_add_table(layout, pattern_table)) {
        goto cleanup_pattern_table;
    }

    return layout;

cleanup_pattern_table:
    control_table_free(pattern_table);
cleanup_params_table:
    control_table_free(params_table);
cleanup_layout:
    layout_free(layout);
    return NULL;
}

bool init_layouts(Interface *interface, State *const state) {
    Layout *song_layout = init_song_layout(state->song);
    if (song_layout == NULL) {
        return false;
    }

    if (!interface_set_layout(interface, TAB_SONG, song_layout)) {
        goto cleanup_song_layout;
    }

    Layout *pattern_layout = init_pattern_layout(state);
    if (pattern_layout == NULL) {
        goto cleanup_song_layout;
    }

    if (!interface_set_layout(interface, TAB_PATTERN, pattern_layout)) {
        goto cleanup_pattern_layout;
    }

    return true;

cleanup_pattern_layout:
    layout_free(pattern_layout);
cleanup_song_layout:
    layout_free(song_layout);
    return false;
}

Interface *interface_init(State *const state) {
    RefList *layouts = ref_list_init();
    if (layouts == NULL) {
        return NULL;
    }

    Interface *interface = malloc(sizeof(Interface));
    if (interface == NULL) {
        ref_list_free(layouts);
        return NULL;
    }

    *interface = (Interface){
        .selected_tab = TAB_SONG,
        .layouts = layouts,
        .input_repr = NULL,
        .input_repr_length = 0,
        .input_repr_printed_at = -1,
        .focus = NULL,
        .focus_control = NULL};

    if (!init_layouts(interface, state)) {
        ref_list_free(layouts);
        free(interface);
        return NULL;
    }

    return interface;
}

bool interface_is_editing(Interface const *interface) {
    return interface->focus_control != NULL && interface->focus_control->edit;
}

void draw_primary_tabs(WINDOW *win, Interface const *interface) {
    const char *name;

    attron(COLOR_PAIR(UI_COLOR_BRIGHT));

    wmove(win, 1, 1);
    name = interface->selected_tab == TAB_SONG ? "*SONG" : " SONG";
    wprintw(win, name);

    wmove(win, 1, 7);
    name = interface->selected_tab == TAB_PATTERN ? "*PATT" : " PATT";
    wprintw(win, name);

    wmove(win, 1, 13);
    const bool instrument_tab = interface->selected_tab == TAB_INSTRUMENT ||
                                interface->selected_tab == TAB_WAVE ||
                                interface->selected_tab == TAB_FILTER;
    name = instrument_tab ? "*INST" : " INST";
    wprintw(win, name);

    wmove(win, 1, 19);
    name = interface->selected_tab == TAB_ARPEGGIO ? "*ARP" : " ARP";
    wprintw(win, name);

    attroff(COLOR_PAIR(UI_COLOR_BRIGHT));
}

void draw_secondary_tabs(WINDOW *win, Interface const *interface) {
    attron(COLOR_PAIR(UI_COLOR_BRIGHT));

    if (interface->selected_tab != TAB_INSTRUMENT &&
        interface->selected_tab != TAB_WAVE &&
        interface->selected_tab != TAB_FILTER) {
        return;
    }

    const char *name;

    wmove(win, 2, 1);
    name = interface->selected_tab == TAB_INSTRUMENT ? "*INST" : " INST";
    wprintw(win, name);

    wmove(win, 2, 7);
    name = interface->selected_tab == TAB_WAVE ? "*WAVE" : " WAVE";
    wprintw(win, name);

    wmove(win, 2, 13);
    name = interface->selected_tab == TAB_FILTER ? "*FILT" : " FILT";
    wprintw(win, name);

    attroff(COLOR_PAIR(UI_COLOR_BRIGHT));
}

void draw_filled_input_repr(WINDOW *win, Interface *interface, int draw_time) {
    int offset = interface_is_editing(interface) ? 22 : 25;
    wmove(win, 15, offset - interface->input_repr_length);
    wprintw(win, interface->input_repr);
    if (interface->input_repr_printed_at == -1) {
        interface->input_repr_printed_at = draw_time;
    }
}

void clear_input_repr(Interface *interface) {
    if (interface->input_repr != NULL) {
        input_repr_free(interface->input_repr);
        interface->input_repr = NULL;
    }
    interface->input_repr_length = 0;
    interface->input_repr_printed_at = -1;
}

void draw_input_repr(WINDOW *win, Interface *interface, int draw_time) {
    attron(COLOR_PAIR(UI_COLOR_GREY));
    if (interface->input_repr_printed_at != -1) {
        const int repr_printed_at = interface->input_repr_printed_at;
        if (draw_time - repr_printed_at < INPUT_REPR_HIDE_DELAY &&
            interface->input_repr != NULL) {
            draw_filled_input_repr(win, interface, draw_time);
        } else {
            clear_input_repr(interface);
        }
    } else if (interface->input_repr != NULL) {
        if (interface->input_repr != NULL) {
            draw_filled_input_repr(win, interface, draw_time);
        }
    }
    attroff(COLOR_PAIR(UI_COLOR_GREY));
}

void draw_edit_value(WINDOW *win, Interface *interface) {
    if (!interface_is_editing(interface)) {
        return;
    }

    if (interface->focus_control->type == CONTROL_TYPE_BOOL ||
        interface->focus_control->type == CONTROL_TYPE_INT) {
        wmove(win, 15, 23);
        wprintw(win, control_repr(interface->focus_control));
    }
}

void draw_layout(WINDOW *win, Interface *interface, int draw_time) {
    Layout const *layout = ref_list_get(interface->layouts,
                                        interface->selected_tab);
    if (layout != NULL) {
        layout_draw(win, layout, draw_time);
    }
}

void interface_draw(WINDOW *win, Interface *interface, int draw_time) {
    wclear(win);
    draw_primary_tabs(win, interface);
    draw_secondary_tabs(win, interface);
    draw_input_repr(win, interface, draw_time);
    draw_edit_value(win, interface);
    draw_layout(win, interface, draw_time);
}

void interface_focus_clear(Interface *interface) {
    if (interface->focus != NULL) {
        layout_focus_clear(interface->focus);
    }

    interface->focus = NULL;
    interface->focus_control = NULL;
}

Tab get_selected_primary_tab(Interface *interface) {
    if (interface->selected_tab != TAB_WAVE &&
        interface->selected_tab != TAB_FILTER) {
        return interface->selected_tab;
    } else {
        return TAB_INSTRUMENT;
    }
}

void next_primary_tab(Interface *interface) {
    Tab primary_tab = get_selected_primary_tab(interface);
    int i = 0;
    for (; i < 4; i++) {
        if (PRIMARY_TABS[i] == primary_tab)
            break;
    }

    if (i == 4) {
        return;
    }

    Tab selected_tab = PRIMARY_TABS[i < 3 ? i + 1 : 3];
    if (interface->selected_tab != selected_tab) {
        interface_focus_clear(interface);
    }
    interface->selected_tab = selected_tab;
}

void prev_primary_tab(Interface *interface) {
    Tab primary_tab = get_selected_primary_tab(interface);
    int i = 0;
    for (; i < 4; i++) {
        if (PRIMARY_TABS[i] == primary_tab)
            break;
    }

    if (i == 4) {
        return;
    }

    Tab selected_tab = PRIMARY_TABS[i > 0 ? i - 1 : 0];
    if (interface->selected_tab != selected_tab) {
        interface_focus_clear(interface);
    }
    interface->selected_tab = selected_tab;
}

void next_secondary_tab(Interface *interface) {
    int i = 0;
    for (; i < 3; i++) {
        if (SECONDARY_TABS[i] == interface->selected_tab)
            break;
    }

    if (i == 3) {
        return;
    }

    Tab selected_tab = SECONDARY_TABS[i < 2 ? i + 1 : 2];
    if (interface->selected_tab != selected_tab) {
        interface_focus_clear(interface);
    }
    interface->selected_tab = selected_tab;
}

void prev_secondary_tab(Interface *interface) {
    int i = 0;
    for (; i < 3; i++) {
        if (SECONDARY_TABS[i] == interface->selected_tab)
            break;
    }

    if (i == 3) {
        return;
    }

    Tab selected_tab = SECONDARY_TABS[i > 0 ? i - 1 : 0];
    if (interface->selected_tab != selected_tab) {
        interface_focus_clear(interface);
    }

    interface->selected_tab = selected_tab;
}

void interface_set_tab(Interface *interface, Tab tab) {
    if (interface->selected_tab != tab) {
        interface_focus_clear(interface);
    }

    interface->selected_tab = tab;
}

bool handle_key_tab_switch(Interface *interface, Input const *input) {
    if (input->type != INPUT_TYPE_KEY ||
        ((input->key.modifier & MODIFIER_KEY_CTRL) != MODIFIER_KEY_CTRL &&
         (input->key.modifier & MODIFIER_KEY_ALT) != MODIFIER_KEY_ALT)) {
        return false;
    }

    const Input test_alt_left = input_init_modified_special(
        MODIFIER_KEY_ALT,
        SPECIAL_KEY_LEFT);
    const Input test_alt_h = input_init_modified_key(MODIFIER_KEY_ALT, 'h');

    const Input test_alt_right = input_init_modified_special(
        MODIFIER_KEY_ALT,
        SPECIAL_KEY_RIGHT);
    const Input test_alt_l = input_init_modified_key(MODIFIER_KEY_ALT, 'l');

    if (input_eq(input, &test_alt_h) || input_eq(input, &test_alt_left)) {
        prev_secondary_tab(interface);
        return true;
    }

    if (input_eq(input, &test_alt_l) || input_eq(input, &test_alt_right)) {
        next_secondary_tab(interface);
        return true;
    }

    const Input test_ctrl_left = input_init_modified_special(
        MODIFIER_KEY_CTRL,
        SPECIAL_KEY_LEFT);
    const Input test_ctrl_h = input_init_modified_key(MODIFIER_KEY_CTRL, 'h');

    const Input test_ctrl_right = input_init_modified_special(
        MODIFIER_KEY_CTRL,
        SPECIAL_KEY_RIGHT);
    const Input test_ctrl_l = input_init_modified_key(MODIFIER_KEY_CTRL, 'l');

    if (input_eq(input, &test_ctrl_h) || input_eq(input, &test_ctrl_left)) {
        prev_primary_tab(interface);
        return true;
    }

    if (input_eq(input, &test_ctrl_l) || input_eq(input, &test_ctrl_right)) {
        next_primary_tab(interface);
        return true;
    }

    return false;
}

bool handle_mouse_tab_switch(Interface *interface, Input const *input) {
    if (!input_mouse_event_eq(input, MOUSE_EVENT_PRESS, MOUSE_BUTTON)) {
        return false;
    }

    const int x = input->mouse.point.x;
    const int y = input->mouse.point.y;

    const Tab primary_tab = get_selected_primary_tab(interface);
    const Tab selected_tab = interface->selected_tab;

    if (y == 2) {
        if (x >= 3 && x < 7 && primary_tab != TAB_SONG) {
            interface_set_tab(interface, TAB_SONG);
            return true;
        }
        if (x >= 9 && x < 13 && primary_tab != TAB_PATTERN) {
            interface_set_tab(interface, TAB_PATTERN);
            return true;
        }
        if (x >= 15 && x < 19 && primary_tab != TAB_INSTRUMENT) {
            interface_set_tab(interface, TAB_INSTRUMENT);
            return true;
        }
        if (x >= 21 && x < 24 && primary_tab != TAB_ARPEGGIO) {
            interface_set_tab(interface, TAB_ARPEGGIO);
            return true;
        }
    } else if (primary_tab == TAB_INSTRUMENT && y == 3) {
        if (x >= 3 && x < 7 && selected_tab != TAB_INSTRUMENT) {
            interface_set_tab(interface, TAB_INSTRUMENT);
            return true;
        }
        if (x >= 9 && x < 13 && selected_tab != TAB_WAVE) {
            interface_set_tab(interface, TAB_WAVE);
            return true;
        }
        if (x >= 15 && x < 19 && selected_tab != TAB_FILTER) {
            interface_set_tab(interface, TAB_FILTER);
            return true;
        }
    }

    return false;
}

bool handle_quick_tab(Interface *interface, Input const *input) {
    const Input test_alt_s = input_init_modified_key(MODIFIER_KEY_ALT, 's');
    if (input_eq(input, &test_alt_s)) {
        interface_set_tab(interface, TAB_SONG);
        return true;
    }

    const Input test_alt_p = input_init_modified_key(MODIFIER_KEY_ALT, 'p');
    if (input_eq(input, &test_alt_p)) {
        interface_set_tab(interface, TAB_PATTERN);
        return true;
    }

    const Input test_alt_i = input_init_modified_key(MODIFIER_KEY_ALT, 'i');
    if (input_eq(input, &test_alt_i)) {
        interface_set_tab(interface, TAB_INSTRUMENT);
        return true;
    }

    const Input test_alt_w = input_init_modified_key(MODIFIER_KEY_ALT, 'w');
    if (input_eq(input, &test_alt_w)) {
        interface_set_tab(interface, TAB_WAVE);
        return true;
    }

    const Input test_alt_f = input_init_modified_key(MODIFIER_KEY_ALT, 'f');
    if (input_eq(input, &test_alt_f)) {
        interface_set_tab(interface, TAB_FILTER);
        return true;
    }

    const Input test_alt_a = input_init_modified_key(MODIFIER_KEY_ALT, 'a');
    if (input_eq(input, &test_alt_a)) {
        interface_set_tab(interface, TAB_ARPEGGIO);
        return true;
    }

    return false;
}

void update_focus_control(Interface *interface) {
    Layout *layout = interface->focus;
    if (layout == NULL) {
        interface->focus_control = NULL;
        return;
    }

    ControlTable *table = layout->focus;
    if (table == NULL) {
        interface->focus_control = NULL;
        return;
    }

    interface->focus_control = table->focus;
}

void focus_first(Interface *interface) {
    Layout *focus_layout = ref_list_get(interface->layouts,
                                        interface->selected_tab);
    if (focus_layout != interface->focus) {
        if (interface->focus != NULL) {
            layout_focus_clear(interface->focus);
        }
        interface->focus = focus_layout;
    }

    if (interface->focus != NULL) {
        layout_focus_first(interface->focus);
    }
    update_focus_control(interface);
}

void focus_last(Interface *interface) {
    Layout *focus_layout = ref_list_get(interface->layouts,
                                        interface->selected_tab);
    if (focus_layout != interface->focus) {
        if (interface->focus != NULL) {
            layout_focus_clear(interface->focus);
        }
        interface->focus = focus_layout;
    }

    if (interface->focus != NULL) {
        layout_focus_last(interface->focus);
    }
    update_focus_control(interface);
}

void focus_first_col_or_same(Interface *interface, int x, int y) {
    Layout *focus_layout = ref_list_get(interface->layouts,
                                        interface->selected_tab);
    if (focus_layout != interface->focus) {
        if (interface->focus != NULL) {
            layout_focus_clear(interface->focus);
        }
        interface->focus = focus_layout;
    }

    if (interface->focus != NULL) {
        layout_focux_first_col_or_same(interface->focus, x, y);
    }
    update_focus_control(interface);
}

void focus_point(Interface *interface, int x, int y, bool exact) {
    Layout *focus_layout = ref_list_get(interface->layouts,
                                        interface->selected_tab);
    if (focus_layout != interface->focus) {
        if (interface->focus != NULL) {
            layout_focus_clear(interface->focus);
        }
        interface->focus = focus_layout;
    }

    if (interface->focus != NULL) {
        layout_focus_point(interface->focus, x, y, exact);
    }
    update_focus_control(interface);
}

void focus_add(Interface *interface, int x, int y, bool jump) {
    Layout *focus_layout = ref_list_get(interface->layouts,
                                        interface->selected_tab);
    if (focus_layout != interface->focus) {
        if (interface->focus != NULL) {
            layout_focus_clear(interface->focus);
        }
        interface->focus = focus_layout;
    }

    if (interface->focus != NULL) {
        layout_focus_add(interface->focus, x, y, jump);
    }
    update_focus_control(interface);
}


bool interface_discard_edit(Interface *interface) {
    if (interface->focus_control == NULL) {
        return false;
    }

    control_discard_edit(interface->focus_control);
    return true;
}

bool interface_edit(Interface *interface) {
    if (interface->focus_control == NULL) {
        return false;
    }

    if (interface->focus_control->edit) {
        if (!interface_discard_edit(interface)) {
            return false;
        }
    }

    return control_edit(interface->focus_control);
}

bool interface_save_edit(Interface *interface) {
    if (interface->focus_control == NULL) {
        return false;
    }

    control_save_edit(interface->focus_control);
    return true;
}

bool handle_step_focus(Interface *interface, Input const *input) {
    const Input test_h = input_init_key('h');
    const Input test_left = input_init_special(SPECIAL_KEY_LEFT);
    if (input_eq(input, &test_h) || input_eq(input, &test_left)) {
        focus_add(interface, -1, 0, true);
        return true;
    }

    const Input test_j = input_init_key('j');
    const Input test_down = input_init_special(SPECIAL_KEY_DOWN);
    if (input_eq(input, &test_j) || input_eq(input, &test_down)) {
        focus_add(interface, 0, 1, true);
        return true;
    }

    const Input test_k = input_init_key('k');
    const Input test_up = input_init_special(SPECIAL_KEY_UP);
    if (input_eq(input, &test_k) || input_eq(input, &test_up)) {
        focus_add(interface, 0, -1, true);
        return true;
    }

    const Input test_l = input_init_key('l');
    const Input test_right = input_init_special(SPECIAL_KEY_RIGHT);
    if (input_eq(input, &test_l) || input_eq(input, &test_right)) {
        focus_add(interface, 1, 0, true);
        return true;
    }

    return false;
}

bool handle_screen_focus(Interface *interface, Input const *input) {
    const Input test_j = input_init_modified_key(MODIFIER_KEY_CTRL, 'j');
    const Input test_down = input_init_modified_special(MODIFIER_KEY_CTRL,
                                                        SPECIAL_KEY_DOWN);
    if (input_eq(input, &test_j) || input_eq(input, &test_down)) {
        focus_add(interface, 0, 8, false);
        return true;
    }

    const Input test_k = input_init_modified_key(MODIFIER_KEY_CTRL, 'k');
    const Input test_up = input_init_modified_special(MODIFIER_KEY_CTRL,
                                                      SPECIAL_KEY_UP);
    if (input_eq(input, &test_k) || input_eq(input, &test_up)) {
        focus_add(interface, 0, -8, false);
        return true;
    }

    return false;
}

bool handle_edge_focus(Interface *interface, Input const *input) {
    const Input test_j = input_init_modified_key(MODIFIER_KEY_ALT, 'j');
    const Input test_down = input_init_modified_special(MODIFIER_KEY_ALT,
                                                        SPECIAL_KEY_DOWN);
    if (input_eq(input, &test_j) || input_eq(input, &test_down)) {
        focus_last(interface);
        return true;
    }

    const Input test_k = input_init_modified_key(MODIFIER_KEY_ALT, 'k');
    const Input test_up = input_init_modified_special(MODIFIER_KEY_ALT,
                                                      SPECIAL_KEY_UP);
    if (input_eq(input, &test_k) || input_eq(input, &test_up)) {
        focus_first(interface);
        return true;
    }

    return false;
}

bool handle_wheel_focus(Interface *interface, Input const *input) {
    if (input_mouse_event_eq(input, MOUSE_EVENT_PRESS,
                             MOUSE_BUTTON_WHEEL_UP)) {
        focus_first_col_or_same(interface, input->mouse.point.x,
                                input->mouse.point.y);
        focus_add(interface, 0, -2, false);
        return true;
    }

    if (input_mouse_event_eq(input, MOUSE_EVENT_PRESS,
                             MOUSE_BUTTON_WHEEL_DOWN)) {
        focus_first_col_or_same(interface, input->mouse.point.x,
                                input->mouse.point.y);
        focus_add(interface, 0, 2, false);
        return true;
    }

    return false;
}

bool handle_mouse_focus(Interface *interface, Input const *input) {
    if (input_mouse_event_eq(input, MOUSE_EVENT_PRESS, MOUSE_BUTTON)) {
        focus_point(interface, input->mouse.point.x - 1,
                     input->mouse.point.y, false);
        interface_edit(interface);
        return true;
    }

    return false;
}

bool handle_focus(Interface *interface, Input const *input) {
    if (!interface_is_editing(interface) &&
        handle_step_focus(interface, input)) {
        return true;
    }

    if (handle_screen_focus(interface, input)) {
        return true;
    }

    if (handle_edge_focus(interface, input)) {
        return true;
    }

    if (handle_wheel_focus(interface, input)) {
        return true;
    }

    if (handle_mouse_focus(interface, input)) {
        return true;
    }

    return false;
}

bool handle_edit(Interface *interface, Input const *input) {
    if (interface->focus_control == NULL) {
        return false;
    }

    Input test_space = input_init_key(' ');
    Input test_enter = input_init_special(SPECIAL_KEY_ENTER);
    bool is_space = input_eq(input, &test_space);
    bool is_enter = input_eq(input, &test_enter);
    bool is_editing = interface_is_editing(interface);

    if (!is_editing) {
        if (is_space || is_enter) {
            return interface_edit(interface);
        }
    } else if (is_enter) {
        return interface_save_edit(interface);
    }

    Input test_esc = input_init_special(SPECIAL_KEY_ESC);
    if (input_eq(input, &test_esc)) {
        return interface_discard_edit(interface);
    }

    Input test_bsp = input_init_key(127);
    Input test_del = input_init_special(SPECIAL_KEY_DEL);

    if (interface->focus_control != NULL && !is_editing &&
        (input_eq(input, &test_bsp) || input_eq(input, &test_del))) {
        if (control_empty(interface->focus_control)) {
            control_save_edit(interface->focus_control);
        }
    }

    return false;
}

bool handle_edit_key_input(Interface *interface, Input const *input) {
    if (!input->key.special) {
        if (input->key.modifier == MODIFIER_KEY_CTRL) {
            int i;
            if (input->key.ch == 'j') {
                i = -12;
            } else if (input->key.ch == 'k') {
                i = 12;
            }

            if (i != 0) {
                control_handle_step_input(interface->focus_control, i);
                return true;
            }
        } else if (input->key.modifier == MODIFIER_KEY_ALT) {
            double i = 1;
            if (input->key.ch == 'j') {
                i = 0.5;
            } else if (input->key.ch == 'k') {
                i = 2;
            }

            if (i != 1) {
                control_handle_multiplier_input(interface->focus_control, i);
                return true;
            }
        }

        InputResult result = control_handle_input(interface->focus_control,
                                                  input->key.ch);
        if (result.done) {
            interface_save_edit(interface);
        }

        return result.handled;
    } else {
        Input test_up = input_init_special(SPECIAL_KEY_UP);
        Input test_oct_up = input_init_modified_special(MODIFIER_KEY_CTRL,
                                                        SPECIAL_KEY_UP);
        Input test_mult_up = input_init_modified_special(MODIFIER_KEY_ALT,
                                                         SPECIAL_KEY_UP);
        Input test_down = input_init_special(SPECIAL_KEY_DOWN);
        Input test_oct_down = input_init_modified_special(MODIFIER_KEY_CTRL,
                                                          SPECIAL_KEY_DOWN);
        Input test_mult_down = input_init_modified_special(MODIFIER_KEY_ALT,
                                                          SPECIAL_KEY_DOWN);

        double m = 1;
        if (input_eq(input, &test_mult_up)) {
            m = 2;
        } else if (input_eq(input, &test_mult_down)) {
            m = 0.5;
        }

        if (m != 1) {
            control_handle_multiplier_input(interface->focus_control, m);
            return true;
        }


        int i = 0;
        if (input_eq(input, &test_up)) {
            i = 1;
        } else if (input_eq(input, &test_oct_up)) {
            i = 12;
        } else if (input_eq(input, &test_down)) {
            i = -1;
        } else if (input_eq(input, &test_oct_down)) {
            i = -12;
        }

        if (i != 0) {
            control_handle_step_input(interface->focus_control, i);
        }
        return true;
    }
}

bool handle_edit_mouse_input(Interface *interface, Input const *input) {
    bool is_wheel_up = input_mouse_event_eq(input, MOUSE_EVENT_PRESS,
                                            MOUSE_BUTTON_WHEEL_UP);
    bool is_wheel_down = input_mouse_event_eq(input, MOUSE_EVENT_PRESS,
                                              MOUSE_BUTTON_WHEEL_DOWN);

    if (is_wheel_up) {
        return control_handle_wheel_input(interface->focus_control,
                                          &input->mouse.point, 1);
    } else if (is_wheel_down) {
        return control_handle_wheel_input(interface->focus_control,
                                          &input->mouse.point, -1);
    }

    return false;
}


bool handle_edit_input(Interface *interface, Input const *input) {
    if (!interface_is_editing(interface)) {
        return false;
    }

    if (input->type == INPUT_TYPE_KEY) {
        return handle_edit_key_input(interface, input);
    } else if (input->type == INPUT_TYPE_MOUSE) {
        return handle_edit_mouse_input(interface, input);
    }

    return false;
}

bool try_handle_input(Interface *interface, Input const *input) {
    if (handle_key_tab_switch(interface, input)) {
        return true;
    }

    if (handle_mouse_tab_switch(interface, input)) {
        return true;
    }

    if (handle_quick_tab(interface, input)) {
        return true;
    }

    if (handle_edit(interface, input)) {
        return true;
    }

    if (handle_edit_input(interface, input)) {
        return true;
    }

    if (handle_focus(interface, input)) {
        return true;
    }

    return false;
}

void interface_handle_input(Interface *interface, Input const *input) {
    if (input->type != INPUT_TYPE_MOUSE ||
        input->mouse.event != MOUSE_EVENT_RELEASE) {
        clear_input_repr(interface);
    }
    if (try_handle_input(interface, input)) {
        char *repr = input_repr(input);
        if (repr != NULL) {
            interface->input_repr = repr;
            interface->input_repr_length = strlen(repr);
        }
    }
}

void interface_free(Interface *interface) {
    for (int i = 0; i < interface->layouts->length; i++) {
        layout_free(ref_list_get(interface->layouts, i));
    }
    if (interface->input_repr != NULL) {
        input_repr_free(interface->input_repr);
    }
    ref_list_free(interface->layouts);
    free(interface);
}

void init_colors(void) {
    start_color();
    init_pair(UI_COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(UI_COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(UI_COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
    init_pair(UI_COLOR_RED, COLOR_RED, COLOR_BLACK);
    init_pair(UI_COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
    init_pair(UI_COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(UI_COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
    init_pair(UI_COLOR_GREY, 8, COLOR_BLACK);
    init_pair(UI_COLOR_BRIGHT, 15, COLOR_BLACK);
    init_pair(UI_COLOR_INVERSE, 16, COLOR_WHITE);
}
