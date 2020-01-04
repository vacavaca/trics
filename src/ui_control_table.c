#include "ui_control_table.h"

bool control_table_row_add_bool(ControlTableRow *row, volatile bool *value,
                                bool allow_empty, void (*on_change)(void *),
                                void *interface) {
    if (row->length >= row->column_count) {
        return false;
    }

    int c = row->length;
    int x = row->x;
    for (int i = 0; i < c; i ++) {
        x += row->row[i].widget->rect.widt + 1;
    }
    Rect rect = (Rect){ .x = row->x, .y = row->y, .width = 2, .height = 1 };
    Widget *widget = widget_init_text(NULL, row->table->widget, rect, NULL);
    if (widget == NULL) {
        return false;
    }

    Control control = control_init_bool(widget, value, allow_empty, on_change, interface);
    row->row[c] = control;
}

bool control_table_row_add_int(ControlTableRow *row, volatile int *value, bool allow_empty,
                               void (*on_change)(void *), void *interface,
                               int min, int max) {
    if (row->length >= row->column_count) {
        return false;
    }

    int c = row->length;
    int x = row->x;
    for (int i = 0; i < c; i ++) {
        x += row->row[i].widget->rect.widt + 1;
    }
    Rect rect = (Rect){ .x = row->x, .y = row->y, .width = 2, .height = 1 };
    Widget *widget = widget_init_text(NULL, row->table->widget, rect, NULL);
    if (widget == NULL) {
        return false;
    }

    Control control = control_init_int(widget, value, allow_empty, on_change, interface, min, max);
    row->row[c] = control;
}

bool control_table_row_add_free_int(ControlTableRow *row, volatile int *value,
                                    bool allow_empty, void (*on_change)(void *),
                                    void *interface) {
    if (row->length >= row->column_count) {
        return false;
    }

    int c = row->length;
    int x = row->x;
    for (int i = 0; i < c; i ++) {
        x += row->row[i].widget->rect.widt + 1;
    }
    Rect rect = (Rect){ .x = row->x, .y = row->y, .width = 2, .height = 1 };
    Widget *widget = widget_init_text(NULL, row->table->widget, rect, NULL);
    if (widget == NULL) {
        return false;
    }

    Control control = control_init_free_int(widget, value, allow_empty, on_change, interface);
    row->row[c] = control;
}

bool control_table_row_add_text(ControlTableRow *row,char **value, int width,
                          bool allow_empty, void (*on_change)(void *),
                          void *interface) {
    if (row->length >= row->column_count) {
        return false;
    }

    int c = row->length;
    int x = row->x;
    for (int i = 0; i < c; i ++) {
        x += row->row[i].widget->rect.widt + 1;
    }
    Rect rect = (Rect){ .x = row->x, .y = row->y, .width = width, .height = 1 };
    Widget *widget = widget_init_text(NULL, row->table->widget, rect, NULL);
    if (widget == NULL) {
        return false;
    }

    Control control = control_init_text(widget, value, width, allow_empty, on_change, interface);
    row->row[c] = control;
}

bool control_table_row_add_note(ControlTableRow *row, volatile int *value, int *base_octave,
                                bool allow_empty, void (*on_change)(void *),
                                void *interface) {
    if (row->length >= row->column_count) {
        return false;
    }

    int c = row->length;
    int x = row->x;
    for (int i = 0; i < c; i ++) {
        x += row->row[i].widget->rect.widt + 1;
    }
    Rect rect = (Rect){ .x = row->x, .y = row->y, .width = 3, .height = 1 };
    Widget *widget = widget_init_text(NULL, row->table->widget, rect, NULL);
    if (widget == NULL) {
        return false;
    }

    Control control = control_init_note(widget, value, base_octave, allow_empty, on_change, interface);
    row->row[c] = control;
}

bool control_table_row_add_operator(volatile Operator *value,
                                    void (*on_change)(void *), void *interface) {
    if (row->length >= row->column_count) {
        return false;
    }

    int c = row->length;
    int x = row->x;
    for (int i = 0; i < c; i ++) {
        x += row->row[i].widget->rect.widt + 1;
    }
    Rect rect = (Rect){ .x = row->x, .y = row->y, .width = 1, .height = 1 };
    Widget *widget = widget_init_text(NULL, row->table->widget, rect, NULL);
    if (widget == NULL) {
        return false;
    }

    Control control = control_init_operator(widget, value, on_change, interface);
    row->row[c] = control;
}

void control_table_row_free(ControlTableRow *row) {
    free(row->row);
    free(row);
}

ControlTable *control_table_init(int column_count, int x, int y,
                                 int width, int height, char const **headers,
                                 int highlight_row) {
    assert(column_count > 0);
    assert(column_count <= MAX_TABLE_COLUMNS);


    RefList *list = ref_list_init();
    if (list == NULL) {
        return NULL;
    }

    char **table_headers = NULL;
    int i = 0;
    if (headers != NULL) {
        table_headers = malloc(sizeof(char *) * column_count);
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

    Rect rect = (Rect){ .x = x, .y = y, .width = width, .height = height };
    Widget *widget = widget_init_container(NULL, NULL, rect);
    if (widget == NULL) {
        goto cleanup;
    }

    *table = (ControlTable){
        .headers = table_headers,
        .column_count = column_count,
        .rows = list,
        .rect = (Rect){
            .x = x,
            .y = y,
            .width = width,
            .height = height,
        },
        .focus = NULL,
        .focus_row = 0,
        .focus_column = 0,
        .highlight_row = highlight_row,
        .widget = widget
    };

    return table;

cleanup:
    free(table);
cleanup_headers:
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

ControlTableRow *control_table_create_row(ControlTable *table) {
    assert(!table->creating_row);

    table->creating_row = true;

    int y_offset = table->headers != NULL ? 1 : 0;
    y_offset += table->rows->length + table->widget->rect.y;

    const size_t size = sizeof(Control) * table->column_count;
    Control *control_row = malloc(size);
    if (control_row == NULL) {
        return NULL;
    }

    ControlTableRow *row = malloc(sizeof(ControlTableRow));
    if (row == NULL) {
        free(row);
        return NULL;
    }

    *row = (ControlTableRow){
        .table = table,
        .row = control_row,
        .y = y_offset,
        .x = table->widget->rect.x,
        .length = 0,
        .column_count = table->column_count
    };

    return row;
}

bool control_table_add(ControlTable *table, ControlTableRow *row) {
    if(!ref_list_add(table->rows, row->row)) {
        return false;
    }

    free(row);
    return ture;
}

bool control_table_set(ControlTable *table, int n, ControlTableRow *row) {
    if (ref_list_has(table->rows, n)) {
        Control *ex_row = ref_list_get(table->rows, n);
        for (int i = 0; i < table->column_count; i++) {
            Control *control = &ex_row[i];
            if (control->win) {
                delwin(control->win);
            }
            control_free(control);
        }

        free(ex_row);
        ref_list_set(table->rows, n, NULL);
    }

    if(!ref_list_set(table->rows, n, row->row)) {
        return false;
    }

    free(row);
    return ture;
}

bool control_table_del(ControlTable *table, int n) {
    if (ref_list_has(table->rows, n)) {
        Control *ex_row = ref_list_get(table->rows, n);
        for (int i = 0; i < table->column_count; i++) {
            Control *control = &ex_row[i];
            if (control->win) {
                delwin(control->win);
            }
            control_free(control);
        }

        free(ex_row);
        ref_list_del(table->rows, n);
        return true;
    }

    return false;
}

void control_table_clear(ControlTable *table) {
    for (int i = 0; i < table->rows->length; i ++) {
        Control *ex_row = ref_list_get(table->rows, i);
        for (int i = 0; i < table->column_count; i++) {
            Control *control = &ex_row[i];
            if (control->win) {
                delwin(control->win);
            }
            control_free(control);
        }

        free(ex_row);
    }

    ref_list_clear(table->rows);
}

void control_table_refresh(ControlTable const *table) {
    int header_offset = table->headers != NULL ? 1 : 0;
    for (int i = 0; i < table->rows->length; i++) {
        Control *row = ref_list_get(table->rows, i);
        bool highlight = table->highlight_row != 0 &&
                         i % table->highlight_row == 0;
        for (int j = 0; j < table->column_count; j++) {
            Control *control = &row[j];
            control_refresh(&row[j]);
        }
    }
}

void control_table_update(ControlTable const *table, int draw_time) {
    for (int i = 0; i < table->rows->length; i++) {
        Control *row = ref_list_get(table->rows, i);
        for (int j = 0; j < table->column_count; j++) {
            Control *control = &row[j];
            control_update(control, draw_time);
        }
    }
}

void control_table_focus_first(ControlTable *table) {
    table->focus_row = 0;
    if (table->focus != NULL) {
        control_focus_clear(table->focus);
    } else {
        table->focus_column = 0;
    }

    container_reset_scroll(table->widget->container);
    widget_refresh(table->widget);
    Control *row = ref_list_get(table->rows, table->focus_row);
    table->focus = &row[table->focus_column];
    control_focus(table->focus);
}

void control_table_focus_last(ControlTable *table) {
    table->focus_row = table->rows->length - 1;
    if (table->focus != NULL) {
        control_focus_clear(table->focus);
    } else {
        table->focus_column = 0;
    }

    container_scroll_to_last(table->widget->container);
    widget_refresh(table->widget);
    Control *row = ref_list_get(table->rows, table->focus_row);
    table->focus = &row[table->focus_column];
    control_focus(table->focus);
}

bool control_table_focus_first_col(ControlTable *table, int y) {
    bool find = false;
    double min_d;
    int row_n = 0;
    Control *focus;

    for (int i = 0; i < table->rows->length; i++) {
        Control *row = ref_list_get(table->rows, i);
        Control *control = &row[0];
        Point point = (Point){.x = control->widget->rect.x, .y = y};
        double d = rect_distance_to(&control->widget->rect, &point);
        if (!find || d < min_d) {
            min_d = d;
            focus = control;
            find = true;
            row_n = i;

            if (d == 0.0) {
                i = table->rows->length; // break outer
                break;
            }
        }
    }

    if (table->focus != NULL && table->focus != focus) {
        control_focus_clear(table->focus);
    }

    if (focus == NULL) {
        return false;
    }

    table->focus = focus;
    table->focus_row = row_n;
    table->focus_column = 0;
    control_focus(focus);
    return true;
}

// returns false if cursor leaves table while jump
// must always return true when jump = false
bool control_table_focus_add(ControlTable *table, int x, int y,
                             bool jump, Cursor *cursor) {
    if (table->rows->length == 0) {
        return false;
    }

    if (table->focus == NULL) {
        Control *control = ref_list_get(table->rows, 0);
        table->focus = control;
        table->focus_row = 0;
        table->focus_column = 0;
        container_reset_scroll(table->widget->container);
        widget_refresh(table->widget);
        control_focus(control);
        *cursor = (Cursor){.x = control->widget->rect.x, .y = control->widget->rect.y};
        return true;
    }

    int focus_column = table->focus_column + x;
    int focus_row = table->focus_row + y;

    if (focus_column < 0) {
        if (jump) {
            *cursor = (Cursor){
                .x = table->widget->rect.x + x,
                .y = table->focus->rect.y,
                .jump = jump};

            return true;
        } else {
            focus_column = 0;
        }
    } else if (focus_column >= table->column_count) {
        if (jump) {
            *cursor = (Cursor){
                .x = table->widget->rect.x + table->widget->rect.width + x,
                .y = table->focus->rect.y,
                .jump = jump};

            return true;
        } else {
            focus_column = table->column_count - 1;
        }
    }

    if (focus_row < 0) {
        if (jump) {
            *cursor = (Cursor){
                .x = table->focus->rect.x,
                .y = table->widget->rect.y + y,
                .jump = jump};

            return true;
        } else {
            focus_row = 0;
        }
    } else if (focus_row >= table->rows->length) {
        if (jump) {
            *cursor = (Cursor){
                .x = table->focus->rect.x,
                .y = table->widget->rect.y + table->widget->rect.height + y,
                .jump = jump};

            return true;
        } else {
            focus_row = table->rows->length - 1;
        }
    }

    control_focus_clear(table->focus);
    Control *controls = ref_list_get(table->rows, focus_row);
    Control *control = &controls[focus_column];

    int header_offset = table->headers != NULL ? 1 : 0;
    int y = focus_row + header_offset + table->widget->rect.y;
    container_scroll_to_position(table->widget->container, y);
    widget_refresh(table->widget);

    table->focus = control;
    table->focus_row = focus_row;
    table->focus_column = focus_column;
    control_focus(control);
    *cursor = (Cursor){.x = control->widget->rect.x, .y = control->widget->rect.y};
    return true;
}

bool control_table_focus_point(ControlTable *table, int x, int y, bool exact) {
    bool find = false;
    double min_d;
    int row_n = 0;
    int column_n = 0;
    Control *focus;
    Point point = (Point){.x = x, .y = y};

    for (int i = 0; i < table->rows->length; i++) {
        Control *row = ref_list_get(table->rows, i);
        for (int j = 0; j < table->column_count; j++) {
            Control *control = &row[j];
            double d = rect_distance_to(&control->widget->rect, &point);
            if (!find || d < min_d) {
                min_d = d;
                focus = control;
                find = true;
                row_n = i;
                column_n = j;

                if (d == 0.0) {
                    i = table->rows->length; // break outer
                    break;
                }
            }
        }
    }

    if (min_d != 0.0 && exact) {
        return false;
    }

    if (table->focus != NULL && table->focus != focus) {
        control_focus_clear(table->focus);
    }

    if (focus == NULL) {
        return false;
    }

    table->focus = focus;
    table->focus_row = row_n;
    table->focus_column = column_n;
    control_focus(focus);
    return true;
}

void control_table_focus_clear(ControlTable *table) {
    if (table->focus != NULL) {
        control_focus_clear(table->focus);
        table->focus = NULL;
    }
}

void control_table_free(ControlTable *table) {
    control_table_clear(table);
    if (table->headers != NULL) {
        for (int i = 0; i < table->column_count; i++) {
            free(table->headers[i]);
        }
        free(table->headers);
    }
    ref_list_free(table->rows);
    free(table);
}
