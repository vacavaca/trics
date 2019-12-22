#include "ui_control_table.h"

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
        .offset = 0,
        .highlight_row = highlight_row};

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
        table_row[i].rect.y = table->rect.y + y_offset + table->rows->length;
        table_row[i].rect.x = table->rect.x + x_offset;
        x_offset += table_row[i].rect.width + 1;
    }

    if (!ref_list_add(table->rows, table_row)) {
        free(table_row);
        return false;
    }


    return true;
}

bool control_table_set(ControlTable *table, int n, Control const *row) {
    if (ref_list_has(table->rows, n)) {
        Control *ex_row = ref_list_get(table->rows, n);
        for (int i = 0; i < table->column_count; i++) {
            control_free(&ex_row[i]);
        }

        free(ex_row);
        ref_list_set(table->rows, n, NULL);
    }

    const size_t size = sizeof(Control) * table->column_count;
    Control *table_row = malloc(size);
    if (table_row == NULL) {
        return false;
    }

    memcpy(table_row, row, size);

    int y_offset = (table->headers != NULL ? 1 : 0) + n;
    int x_offset = 0;
    for (int i = 0; i < table->column_count; i++) {
        table_row[i].rect.y = table->rect.y + y_offset;
        table_row[i].rect.x = table->rect.x + x_offset;
        x_offset += table_row[i].rect.width + 1;
    }

    if (!ref_list_set(table->rows, n, table_row)) {
        free(table_row);
        return false;
    }

    return true;
}

void control_table_draw(WINDOW *win, ControlTable const *table, int draw_time) {
    if (table->headers != NULL) {
        int offset = 0;
        Control *first_row = ref_list_get(table->rows, 0);
        for (int i = 0; i < table->column_count; i++) {
            int x = table->rect.x + offset;
            if (table->rows->length > 0) {
                offset += first_row[i].rect.width + 1;
            } else {
                offset += 3;
            }

            wmove(win, table->rect.y, x);
            wprintw(win, table->headers[i]);
        }
    }

    int header_offset = table->headers != NULL ? 1 : 0;
    for (int i = 0; i < table->rows->length; i++) {
        Control *row = ref_list_get(table->rows, i);
        bool highlight = table->highlight_row != 0 &&
                         i % table->highlight_row == 0;
        if (highlight) {
            attron(A_BOLD);
        }
        for (int j = 0; j < table->column_count; j++) {
            Control *control = &row[j];
            control->rect.y = i - table->offset + table->rect.y +
                              header_offset;
            if (i >= table->offset &&
                i < table->offset + MAX_TABLE_VISIBLE_ROWS) {
                control_draw(win, &row[j], draw_time);
            }
        }

        if (highlight) {
            attroff(A_BOLD);
        }
    }

    bool offset_positive = table->offset > 0;
    bool offset_above_end = (table->offset + MAX_TABLE_VISIBLE_ROWS) <
                            table->rows->length;
    if (offset_positive) {
        wmove(win, 15, 1);
        wprintw(win, "^");
    }

    if (offset_above_end) {
        wmove(win, 15, 2);
        wprintw(win, "...");
    }
}

void control_table_focus_first(ControlTable *table) {
    table->focus_row = 0;
    if (table->focus != NULL) {
        control_focus_clear(table->focus);
    } else {
        table->focus_column = 0;
    }

    table->offset = 0;
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

    table->offset = table->rows->length - MAX_TABLE_VISIBLE_ROWS;
    if (table->offset < 0) {
        table->offset = 0;
    }
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
        Point point = (Point){.x = control->rect.x, .y = y};
        double d = rect_distance_to(&control->rect, &point);
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
        table->offset = 0;
        control_focus(control);
        *cursor = (Cursor){.x = control->rect.x, .y = control->rect.y};
        return true;
    }

    int focus_column = table->focus_column + x;
    int focus_row = table->focus_row + y;

    if (focus_column < 0) {
        if (jump) {
            *cursor = (Cursor){
                .x = table->rect.x + x,
                .y = table->focus->rect.y,
                .jump = jump};

            return true;
        } else {
            focus_column = 0;
        }
    } else if (focus_column >= table->column_count) {
        if (jump) {
            *cursor = (Cursor){
                .x = table->rect.x + table->rect.width + x,
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
                .y = table->rect.y + y,
                .jump = jump};

            return true;
        } else {
            focus_row = 0;
        }
    } else if (focus_row >= table->rows->length) {
        if (jump) {
            *cursor = (Cursor){
                .x = table->focus->rect.x,
                .y = table->rect.y + table->rect.height + y,
                .jump = jump};

            return true;
        } else {
            focus_row = table->rows->length - 1;
        }
    }

    control_focus_clear(table->focus);
    Control *controls = ref_list_get(table->rows, focus_row);
    Control *control = &controls[focus_column];

    if (focus_row - table->offset >= MAX_TABLE_VISIBLE_ROWS) {
        table->offset = focus_row - MAX_TABLE_VISIBLE_ROWS + 1;
    } else if (focus_row - table->offset < 0) {
        table->offset = focus_row;
    }

    table->focus = control;
    table->focus_row = focus_row;
    table->focus_column = focus_column;
    control_focus(control);
    *cursor = (Cursor){.x = control->rect.x, .y = control->rect.y};
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
            double d = rect_distance_to(&control->rect, &point);
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
    for (int i = 0; i < table->rows->length; i++) {
        Control *row = ref_list_get(table->rows, i);
        for (int j = 0; j < table->column_count; j++) {
            control_free(&row[j]);
        }
        free(row);
    }
    if (table->headers != NULL) {
        for (int i = 0; i < table->column_count; i++) {
            free(table->headers[i]);
        }
        free(table->headers);
    }
    ref_list_free(table->rows);
    free(table);
}
