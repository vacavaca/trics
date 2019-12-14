#include "ui.h"

RefList *ref_list_init(void) {
    const size_t item_size = sizeof(void *);
    const size_t cap = item_size * DEFAULT_LIST_CAPACITY;
    void **array = malloc(cap);
    if (array == NULL) {
        return NULL;
    }

    RefList *list = malloc(sizeof(RefList));
    if (list == NULL) {
        free(array);
        return NULL;
    }

    *list = (RefList){
        .item_size = item_size,
        .cap = cap,
        .length = 0,
        .array = array};

    return list;
}

bool ref_list_add(RefList *list, void *item) {
    if (list->length == list->cap) {
        const size_t next_cap = list->cap * 2;
        void **next = realloc(list->array, next_cap);
        if (next == NULL) {
            return false;
        }
        list->array = next;
        list->cap = next_cap;
    }

    list->array[list->length] = item;
    list->length += 1;
    return true;
}

bool ref_list_set(RefList *list, int n, void *item) {
    if (n >= list->cap) {
        const size_t next_cap = list->cap * 2;
        void **next = realloc(list->array, next_cap);
        if (next == NULL) {
            return false;
        }
        list->array = next;
        list->cap = next_cap;
    }

    list->array[n] = item;
    if (n >= list->length) {
        list->length = n + 1;
    }
    return true;
}

void *ref_list_get(RefList *list, int n) {
    if (n >= list->length) {
        return NULL;
    }

    return list->array[n];
}

void ref_list_free(RefList *list) {
    free(list->array);
    free(list);
}

Control control_init_bool(volatile bool *const value) {
    return (Control){
        .type = CONTROL_TYPE_BOOL,
        .bool_value = value,
        .width = 2};
}

Control control_init_int(volatile int *const value) {
    return (Control){
        .type = CONTROL_TYPE_INT,
        .int_value = value,
        .width = 2};
}

Control control_init_text(char *const value, int width) {
    return (Control){
        .type = CONTROL_TYPE_TEXT,
        .text_value = value,
        .width = width};
}

void control_draw_bool(WINDOW *win, Control const *control) {
    wmove(win, control->y, control->x);
    wprintw(win, "%2x", (*control->bool_value ? 255 : 0));
}

void control_draw_int(WINDOW *win, Control const *control) {
    wmove(win, control->y, control->x);
    int value = *control->int_value;
    if (value != EMPTY) {
        wprintw(win, "%02x", *control->int_value);
    } else {
        wprintw(win, "--");
    }
}

void control_draw_text(WINDOW *win, Control const *control) {
    const int len = strlen(control->text_value);
    char display_text[MAX_TEXT_WIDTH];

    memcpy(display_text, control->text_value, MAX_TEXT_WIDTH);
    if (len > control->width) {
        display_text[control->width] = '\0';
        display_text[control->width - 1] = '.';
        display_text[control->width - 2] = '.';
        display_text[control->width - 3] = '.';
    }

    wmove(win, control->y, control->x);
    wprintw(win, display_text);
}

void control_draw(WINDOW *win, Control const *control) {
    attron(COLOR_PAIR(UI_COLOR_BRIGHT));
    // attron(COLOR_PAIR(UI_COLOR_INVERSE));
    // attron(A_BOLD);
    if (control->type == CONTROL_TYPE_BOOL) {
        control_draw_bool(win, control);
    } else if (control->type == CONTROL_TYPE_INT) {
        control_draw_int(win, control);
    } else if (control->type == CONTROL_TYPE_TEXT) {
        control_draw_text(win, control);
    }
    attroff(COLOR_PAIR(UI_COLOR_BRIGHT));
    // attroff(A_BOLD);
    // attroff(COLOR_PAIR(UI_COLOR_INVERSE));
}

ControlTable *control_table_init(int column_count, int x, int y, int width,
                                 char const **headers) {
    assert(column_count > 0);
    assert(column_count <= MAX_TABLE_COLUMNS);

    RefList *list = ref_list_init();
    if (list == NULL) {
        return NULL;
    }

    char **table_headers = NULL;
    int i = 0;
    if (headers != NULL) {
        table_headers = malloc(sizeof(char *));
        if (table_headers == NULL) {
            goto cleanup_list;
        }

        for (; i < column_count; i++) {
            const size_t len = strlen(headers[i]);
            char *header = malloc(len + 1);
            if (header == NULL) {
                goto cleanup;
            }
            memcpy(header, headers[i], len + 1);
            table_headers[i] = header;
        }
    }

    ControlTable *table = malloc(sizeof(ControlTable));
    if (table == NULL) {
        goto cleanup;
    }

    *table = (ControlTable){
        .headers = table_headers,
        .column_count = column_count,
        .rows = list,
        .x = x,
        .y = y,
        .width = width};

    return table;

cleanup:
    for (int j = 0; j < i; j++) {
        free(table_headers[j]);
    }
    if (headers != NULL) {
        free(table_headers);
    }
cleanup_list:
    ref_list_free(list);
    return NULL;
}

bool control_table_add(ControlTable *table, Control const *row) {
    const size_t size = sizeof(Control) * table->column_count;
    Control *table_row = malloc(size);
    if (table_row == NULL) {
        return false;
    }

    memcpy(table_row, row, size);

    int y_offset = table->headers != NULL ? 1 : 0;
    int x_offset = 0;
    for (int i = 0; i < table->column_count; i++) {
        table_row[i].y = table->y + y_offset + table->rows->length;
        table_row[i].x = table->x + x_offset;
        x_offset += table_row[i].width + 1;
    }

    if (!ref_list_add(table->rows, table_row)) {
        free(table_row);
        return false;
    }

    return true;
}

void control_table_draw(WINDOW *win, ControlTable const *table) {
    if (table->headers != NULL) {
        int offset = 0;
        Control *first_row = ref_list_get(table->rows, 0);
        for (int i = 0; i < table->column_count; i++) {
            int x = table->x + offset;
            offset += first_row[i].width + 1;

            wmove(win, table->y, x);
            wprintw(win, table->headers[i]);
        }
    }

    for (int i = 0; i < table->rows->length; i++) {
        Control *row = ref_list_get(table->rows, i);
        for (int j = 0; j < table->column_count; j++) {
            control_draw(win, &row[j]);
        }
    }
}

void control_table_free(ControlTable *table) {
    for (int i = 0; i < table->rows->length; i++) {
        free(table->rows->array[i]);
    }
    for (int i = 0; i < table->column_count; i++) {
        free(table->headers[i]);
    }
    free(table->headers);
    ref_list_free(table->rows);
    free(table);
}

Layout *layout_init(void) {
    RefList *tables = ref_list_init();
    if (tables == NULL) {
        return NULL;
    }

    Layout *layout = malloc(sizeof(Layout));

    *layout = (Layout){
        .tables = tables};

    return layout;
}

bool layout_add_table(Layout *layout, ControlTable *table) {
    return ref_list_add(layout->tables, table);
}

void layout_draw(WINDOW *win, Layout const *layout) {
    for (int i = 0; i < layout->tables->length; i++) {
        control_table_draw(win, ref_list_get(layout->tables, i));
    }
}

void layout_free(Layout *layout) {
    ref_list_free(layout->tables);
    free(layout);
}

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
    ControlTable *song_params_table = control_table_init(3, 2, 3, 23,
                                                         song_params_headers);
    if (song_params_table == NULL) {
        return NULL;
    }

    Control song_param_controls[3] = {
        control_init_int(&song->bpm),
        control_init_int(&song->step),
        control_init_text((char *const)song->name, 17),
    };

    if (!control_table_add(song_params_table, song_param_controls)) {
        control_table_free(song_params_table);
        return NULL;
    }

    return song_params_table;
}

ControlTable *init_song_arrangement_table(Layout *layout, Song *const song) {
    ControlTable *arrangement_table = control_table_init(8, 2, 6, 23, NULL);
    if (arrangement_table == NULL) {
        return NULL;
    }

    for (int i = 0; i < 8; i++) {
        Control arrangement_row[8] = {
            control_init_int(&song->patterns[i][0]),
            control_init_int(&song->patterns[i][1]),
            control_init_int(&song->patterns[i][2]),
            control_init_int(&song->patterns[i][3]),
            control_init_int(&song->patterns[i][4]),
            control_init_int(&song->patterns[i][5]),
            control_init_int(&song->patterns[i][6]),
            control_init_int(&song->patterns[i][7]),
        };

        if (!control_table_add(arrangement_table, arrangement_row)) {
            control_table_free(arrangement_table);
            return NULL;
        }
    }

    return arrangement_table;
}

Layout* init_song_layout(Song *const song) {
    Layout *layout = layout_init();
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

    ControlTable *arrangement_table = init_song_arrangement_table(layout, song);
    if (arrangement_table == NULL) {
        goto cleanup_song_table;
    }

    if (!layout_add_table(layout, arrangement_table)) {
        goto cleanup_arrangement_table;
    }

    return layout;

cleanup_arrangement_table:
    control_table_free(arrangement_table);
cleanup_song_table:
    control_table_free(params_table);
cleanup_layout:
    layout_free(layout);
    return NULL;
}

bool init_layouts(Interface *interface, Song *const song) {
    Layout *song_layout = init_song_layout(song);
    if (song_layout == NULL) {
        return false;
    }
    interface_set_layout(interface, TAB_SONG, song_layout);

    return true;

    return false;
}

Interface *interface_init(Song *const song) {
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
        .input_repr_printed_at = -1};

    if (!init_layouts(interface, song)) {
        ref_list_free(layouts);
        free(interface);
        return NULL;
    }

    return interface;
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
    wmove(win, 15, 25 - interface->input_repr_length);
    wprintw(win, interface->input_repr);
    if (interface->input_repr_printed_at == -1) {
        interface->input_repr_printed_at = draw_time;
    }
}

void clear_input_repr(Interface *interface) {
    if (interface->input_repr != NULL) {
        free(interface->input_repr);
        interface->input_repr = NULL;
    }
    interface->input_repr_length = 0;
    interface->input_repr_printed_at = -1;
}

void draw_input_repr(WINDOW *win, Interface *interface, int draw_time) {
    attron(COLOR_PAIR(UI_COLOR_GREY));
    if (interface->input_repr_printed_at != -1) {
        if (draw_time - interface->input_repr_printed_at < 600 &&
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

void draw_layout(WINDOW *win, Interface *interface) {
    Layout const *layout = ref_list_get(interface->layouts, interface->selected_tab);
    if (layout != NULL) {
        layout_draw(win, layout);
    }
}

void interface_draw(WINDOW *win, Interface *interface, int draw_time) {
    wclear(win);
    draw_primary_tabs(win, interface);
    draw_secondary_tabs(win, interface);
    draw_input_repr(win, interface, draw_time);
    draw_layout(win, interface);
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

    interface->selected_tab = PRIMARY_TABS[i < 3 ? i + 1 : 3];
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

    interface->selected_tab = PRIMARY_TABS[i > 0 ? i - 1 : 0];
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

    interface->selected_tab = SECONDARY_TABS[i < 2 ? i + 1 : 2];
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

    interface->selected_tab = SECONDARY_TABS[i > 0 ? i - 1 : 0];
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

    const int x = input->mouse.x;
    const int y = input->mouse.y;

    const Tab primary_tab = get_selected_primary_tab(interface);
    const Tab selected_tab = interface->selected_tab;

    if (y == 2) {
        if (x >= 3 && x < 7 && primary_tab != TAB_SONG) {
            interface->selected_tab = TAB_SONG;
            return true;
        }
        if (x >= 9 && x < 13 && primary_tab != TAB_PATTERN) {
            interface->selected_tab = TAB_PATTERN;
            return true;
        }
        if (x >= 15 && x < 19 && primary_tab != TAB_INSTRUMENT) {
            interface->selected_tab = TAB_INSTRUMENT;
            return true;
        }
        if (x >= 21 && x < 24 && primary_tab != TAB_ARPEGGIO) {
            interface->selected_tab = TAB_ARPEGGIO;
            return true;
        }
    } else if (primary_tab == TAB_INSTRUMENT && y == 3) {
        if (x >= 3 && x < 7 && selected_tab != TAB_INSTRUMENT) {
            interface->selected_tab = TAB_INSTRUMENT;
            return true;
        }
        if (x >= 9 && x < 13 && selected_tab != TAB_WAVE) {
            interface->selected_tab = TAB_WAVE;
            return true;
        }
        if (x >= 15 && x < 19 && selected_tab != TAB_FILTER) {
            interface->selected_tab = TAB_FILTER;
            return true;
        }
    }

    return false;
}

bool handle_quick_tab(Interface *interface, Input const *input) {
    const Input test_alt_s = input_init_modified_key(MODIFIER_KEY_ALT, 's');
    if (input_eq(input, &test_alt_s)) {
        interface->selected_tab = TAB_SONG;
        return true;
    }

    const Input test_alt_p = input_init_modified_key(MODIFIER_KEY_ALT, 'p');
    if (input_eq(input, &test_alt_p)) {
        interface->selected_tab = TAB_PATTERN;
        return true;
    }

    const Input test_alt_i = input_init_modified_key(MODIFIER_KEY_ALT, 'i');
    if (input_eq(input, &test_alt_i)) {
        interface->selected_tab = TAB_INSTRUMENT;
        return true;
    }

    const Input test_alt_w = input_init_modified_key(MODIFIER_KEY_ALT, 'w');
    if (input_eq(input, &test_alt_w)) {
        interface->selected_tab = TAB_WAVE;
        return true;
    }

    const Input test_alt_f = input_init_modified_key(MODIFIER_KEY_ALT, 'f');
    if (input_eq(input, &test_alt_f)) {
        interface->selected_tab = TAB_FILTER;
        return true;
    }

    const Input test_alt_a = input_init_modified_key(MODIFIER_KEY_ALT, 'a');
    if (input_eq(input, &test_alt_a)) {
        interface->selected_tab = TAB_ARPEGGIO;
        return true;
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