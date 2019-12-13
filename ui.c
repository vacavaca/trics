#include "ui.h"

Control control_init_bool(volatile bool *const value) {
    return (Control){.type = CONTROL_TYPE_BOOL, .bool_value = value};
}

Control control_init_int(volatile int *const value) {
    return (Control){.type = CONTROL_TYPE_INT, .int_value = value};
}

Control control_init_text(char *const value) {
    return (Control){.type = CONTROL_TYPE_INT, .text_value = value};
}

int control_table_init(int column_count, int x, int y, int width,
                       char const **headers, ControlTable *table) {
    assert(column_count > 0);
    assert(width <= MAX_TABLE_WIDTH);

    Control const **controls =
        (Control const **)malloc(sizeof(Control *) * DEFAULT_VEC_CAPACITY);
    if (controls == NULL) {
        return 0;
    }

    *table = (ControlTable){.headers = headers,
                            .column_count = column_count,
                            .controls = controls,
                            .capacity = DEFAULT_VEC_CAPACITY,
                            .length = 0,
                            .x = x,
                            .y = y,
                            .width = width};

    return 1;
}

int control_table_add(ControlTable *table, Control const *row) {
    if (table->length == table->capacity) {
        int next_cap = table->capacity * 2;
        Control const **ex;
        ex = realloc(table->controls, next_cap);
        if (ex == NULL) {
            return 0;
        }

        table->capacity = next_cap;
        table->controls = ex;
    }

    table->controls[table->length] = row;
    table->length += 1;

    return 1;
}

void control_table_free(ControlTable *table) {
    free(table->controls);
    table->controls = NULL;
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

int interface_init(Interface *interface) {
    ControlTable *tables =
        (ControlTable *)malloc(sizeof(ControlTable *) * DEFAULT_VEC_CAPACITY);

    if (tables == NULL) {
        return 0;
    }

    *interface = (Interface){
        .selected_tab = TAB_SONG,
        .tables = tables,
        .focus = NULL,
        .capacity = DEFAULT_VEC_CAPACITY};

    return 1;
}

int interface_add_table(Interface *interface, ControlTable table) {
    if (interface->tables_count == interface->capacity) {
        ControlTable const **ex;
        int next_cap = interface->capacity * 2;
        ex = realloc(interface->tables, next_cap);
        if (ex == NULL) {
            return 0;
        }

        interface->capacity = next_cap;
        interface->tables = ex;
    }

    interface->tables[interface->tables_count] = table;
    interface->tables_count += 1;

    return 1;
}

void interface_next_primary_tab(Interface *interface) {
    int i = 0;
    for (; i < 4; i++) {
        if (PRIMARY_TABS[i] == interface->selected_tab)
            break;
    }

    interface->selected_tab = PRIMARY_TABS[i < 3 ? i + 1 : 3];
}

void interface_prev_primary_tab(Interface *interface) {
    int i = 0;
    for (; i < 4; i++) {
        if (PRIMARY_TABS[i] == interface->selected_tab)
            break;
    }

    interface->selected_tab = PRIMARY_TABS[i > 0 ? i - 1 : 0];
}

int draw_tabs(WINDOW *win, Interface *interface) {
    if (wmove(win, 1, 1) != OK) {
        return 0;
    }

    if (wprintw(win, interface->selected_tab == TAB_SONG ? "*SONG" : " SONG") != OK) {
        return 0;
    }

    if (wmove(win, 1, 7) != OK) {
        return 0;
    }

    if (wprintw(win, interface->selected_tab == TAB_PATTERN ? "*PATT" : " PATT") != OK) {
        return 0;
    }

    if (wmove(win, 1, 13) != OK) {
        return 0;
    }

    const bool instrument_tab = interface->selected_tab == TAB_INSTRUMENT ||
                                interface->selected_tab == TAB_WAVE ||
                                interface->selected_tab == TAB_FILTER;
    if (wprintw(win, instrument_tab ? "*INST" : " INST") != OK) {
        return 0;
    }

    if (wmove(win, 1, 19) != OK) {
        return 0;
    }

    if (wprintw(win, interface->selected_tab == TAB_ARPEGGIO ? "*ARP" : " ARP") != OK) {
        return 0;
    }

    return 1;
}

int interface_draw(WINDOW *win, Interface *interface) {
    if (!draw_tabs(win, interface)) {
        return 0;
    }

    return 1;
}

void interface_free(Interface *interface) {
    free(interface->tables);
    interface->tables = NULL;
}
