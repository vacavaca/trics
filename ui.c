#include "ui.h"

typedef struct{
    bool handled;
    bool done;
} InputResult;

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

char *text_cut(char *str, int width, bool cut, bool ellipsis) {
    const int len = strlen(str);
    char *result = malloc(len + 1);

    memcpy(result, str, len + 1);
    if (cut && len > width) {
        result[width] = '\0';
        if (ellipsis) {
            result[width - 1] = '.';
            result[width - 2] = '.';
            result[width - 3] = '.';
        }
    }

    return result;
}

Control control_init_bool(volatile bool *const value, bool allow_empty) {
    return (Control){
        .type = CONTROL_TYPE_BOOL,
        .bool_value = value,
        .focus = false,
        .focused_at = -1,
        .edit = false,
        .text_edit_reseted = false,
        .num_edit_reseted = false,
        .allow_empty = allow_empty,
        .edit_value = NULL,
        .rect = (Rect){
            .x = 0,
            .y = 0,
            .width = 2,
            .height = 1,
        }};
}

Control control_init_int(volatile int *const value, bool allow_empty) {
    return (Control){
        .type = CONTROL_TYPE_INT,
        .int_value = value,
        .focus = false,
        .focused_at = -1,
        .edit = false,
        .text_edit_reseted = false,
        .num_edit_reseted = false,
        .allow_empty = allow_empty,
        .edit_value = NULL,
        .rect = (Rect){
            .x = 0,
            .y = 0,
            .width = 2,
            .height = 1}};
}

Control control_init_text(char **const value, int width, bool allow_empty) {
    return (Control){
        .type = CONTROL_TYPE_TEXT,
        .text_value = value,
        .focus = false,
        .focused_at = -1,
        .edit = false,
        .text_edit_reseted = false,
        .num_edit_reseted = false,
        .allow_empty = allow_empty,
        .edit_value = NULL,
        .rect = (Rect){
            .x = 0,
            .y = 0,
            .width = width,
            .height = 1}};
}

char *control_repr_bool(Control const *control) {
    char *str = malloc(3);
    sprintf(str, "%02x", *control->bool_value);
    return str;
}

char *control_repr_int(Control const *control) {
    char *str = malloc(3);
    int value = *control->int_value;
    if (value != EMPTY) {
        sprintf(str, "%02x", value - 1);
    } else {
        sprintf(str, "--");
    }
    return str;
}

char *control_repr_text(Control const *control, bool cut, bool ellipsis) {
    int len = strlen(*control->text_value);
    if (len == 0) {
        char *str = malloc(2);
        sprintf(str, " ");
        return str;
    }

    return text_cut(*control->text_value, control->rect.width, cut, ellipsis);
}

char *control_repr(Control* control, bool cut, bool ellipsis) {
    if (control->type == CONTROL_TYPE_BOOL) {
        return control_repr_bool(control);
    } else if (control->type == CONTROL_TYPE_INT) {
        return control_repr_int(control);
    } else if (control->type == CONTROL_TYPE_TEXT) {
        return control_repr_text(control, cut, ellipsis);
    }

    return NULL;
}

void control_draw(WINDOW *win, Control *control, int draw_time) {
    int color;
    if (control->focus || control->edit) {
        if (control->focused_at == -1) {
            control->focused_at = draw_time;
        }

        const int double_refresh_rate = CURSOR_BLINK_RATE_MSEC * 2;
        if (control->edit ||
            draw_time - control->focused_at < CURSOR_BLINK_RATE_MSEC) {
            color = UI_COLOR_INVERSE;
        } else if (draw_time - control->focused_at > double_refresh_rate) {
            control->focused_at = draw_time;
            color = UI_COLOR_INVERSE;
        } else {
            color = UI_COLOR_BRIGHT;
        }
    } else {
        color = UI_COLOR_BRIGHT;
    }

    if (control->edit) {
        attron(A_BOLD);
    }

    attron(COLOR_PAIR(color));
    wmove(win, control->rect.y, control->rect.x);

    if (!control->edit) {
        char *repr = control_repr(control, true, true);
        wprintw(win, repr);
        free(repr);
    } else {
        if (control->edit_value != NULL) {
            char *repr = text_cut(control->edit_value, control->rect.width,
                                  true, false);
            wprintw(win, "%s", repr);
            free(repr);
        }
    }

    attroff(COLOR_PAIR(color));

    if (control->edit) {
        attroff(A_BOLD);
    }
}

void control_focus(Control *control) {
    control->focus = true;
    control->focused_at = -1;
}

void control_focus_clear(Control *control) {
    control->focus = false;
    if (control->edit && control->edit_value != NULL) {
        free(control->edit_value);
        control->edit_value = NULL;
    }
    control->edit = false;
}

bool control_edit(Control *control) {
    if (control->edit) {
        return true;
    }

    control->edit_value = control_repr(control, false, false);

    control->edit = true;
    return true;
}

InputResult control_handle_num_input(Control *control, unsigned char key) {
    // non hex chars and not backspace
    if ((key < 48 || key > 57) && (key < 97 || key > 102) && key != 127) {
        return (InputResult) { .handled = false };
    }

    if (key == 127) {
        if (control->allow_empty) {
            control->edit_value[0] = '-';
            control->edit_value[1] = '-';
            return (InputResult) { .handled = true, .done = true };
        } else {
            return (InputResult) { .handled = false };
        }
    }

    if (!control->num_edit_reseted) {
        control->edit_value[0] = ' ';
        control->edit_value[1] = key;

        control->num_edit_reseted = true;
        return (InputResult) { .handled = true };
    } else {
        control->edit_value[0] = control->edit_value[1];
        control->edit_value[1] = key;
        return (InputResult) { .handled = true, .done = true };
    }
}

InputResult control_handle_text_input(Control *control, unsigned char key) {
    // printable chars + backspace
    if (key < 32 || key > 127) {
        return (InputResult) { .handled = false };
    }

    int len = strlen(control->edit_value);
    if (key == 127) {
        int bsp = control->rect.width < len ? control->rect.width : len;
        control->edit_value[bsp - 1] = 0;
        if (!control->text_edit_reseted) {
            control->text_edit_reseted = true;
        }
        return (InputResult) { .handled = true };
    }

    if (!control->text_edit_reseted) {
        if (key == 127) {
            int bsp = control->rect.width < len ? control->rect.width : len;
            control->edit_value[bsp - 1] = 0;
        } else {
            free(control->edit_value);
            control->edit_value = malloc(2);
            sprintf(control->edit_value, "%c", key);
        }
        control->text_edit_reseted = true;
    } else {
        if (len >= control->rect.width) {
            return (InputResult) { .handled = true, .done = true };
        }

        char *ex = realloc(control->edit_value, len + 2);
        if (ex == NULL) {
            return (InputResult) { .handled = false };
        }
        control->edit_value = ex;
        control->edit_value[len] = key;
        control->edit_value[len + 1] = 0;
    }

    return (InputResult) { .handled = true };
}

InputResult control_handle_input(Control *control, char key) {
    if (control->type == CONTROL_TYPE_BOOL ||
        control->type == CONTROL_TYPE_INT) {
        return control_handle_num_input(control, key);
    } else if (control->type == CONTROL_TYPE_TEXT) {
        return control_handle_text_input(control, key);
    }

    return (InputResult) { .handled = false };
}

void control_save_edit(Control *control) {
    if (!control->edit) {
        return;
    }

    if (control->edit_value != NULL) {
        bool control_bool = control->type == CONTROL_TYPE_BOOL;
        bool control_int = control->type == CONTROL_TYPE_INT;
        if (control_bool || control_int) {
            if (strcmp(control->edit_value,"--")) {
                int value = strtol(control->edit_value, NULL, 16);
                if (errno != EINVAL) {
                    if (control_bool) {
                        *control->bool_value = value > 1 ? 2 : 1;
                    } else {
                        *control->int_value = value + 1;
                    }
                }
            } else {
                if (control_bool) {
                    *control->bool_value = 0;
                } else {
                   *control->int_value = EMPTY;
               }
            }

            free(control->edit_value);
        } else if (control->type == CONTROL_TYPE_TEXT) {
            free(*control->text_value);
            *control->text_value = control->edit_value;
        }

        control->edit_value = NULL;
    }

    control->edit = false;
    control->text_edit_reseted = false;
    control->num_edit_reseted = false;
}

void control_discard_edit(Control *control) {
    if (!control->edit) {
        return;
    }

    if (control->type == CONTROL_TYPE_TEXT) {
        if (control->edit_value != NULL) {
            free(control->edit_value);
        }
        control->edit_value = NULL;
    }

    control->edit = false;
    control->text_edit_reseted = false;
    control->num_edit_reseted = false;
}

void control_free(Control *control) {
    if (control->edit_value != NULL) {
        free(control->edit_value);
        control->edit_value = NULL;
    }
}

bool control_empty(Control* control) {
    if (!control->allow_empty) {
        return false;
    }

    control_discard_edit(control);
    bool control_bool = control->type == CONTROL_TYPE_BOOL;
    bool control_int = control->type == CONTROL_TYPE_INT;
    if (control_bool || control_int) {
        if (control_bool) {
            *control->bool_value = EMPTY;
        } else {
            *control->int_value = EMPTY;
        }
    } else {
        free(*control->text_value);
        char *val = malloc(1);
        val[0] = 0;
        *control->text_value = val;
    }

    return true;
}

typedef struct {
    int x;
    int y;
    bool jump;
} Cursor;

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
            .height = (table_headers != NULL ? 1 : 0),
        },
        .focus = NULL,
        .focus_row = 0,
        .focus_column = 0,
        .offset = 0,
    };

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

    table->rect.height += 1;

    return true;
}

void control_table_draw(WINDOW *win, ControlTable const *table, int draw_time) {
    if (table->headers != NULL) {
        int offset = 0;
        Control *first_row = ref_list_get(table->rows, 0);
        for (int i = 0; i < table->column_count; i++) {
            int x = table->rect.x + offset;
            offset += first_row[i].rect.width + 1;

            wmove(win, table->rect.y, x);
            wprintw(win, table->headers[i]);
        }
    }

    int header_offset = table->headers != NULL ? 1 : 0;
    for (int i = 0; i < table->rows->length; i++) {
        Control *row = ref_list_get(table->rows, i);
        for (int j = 0; j < table->column_count; j++) {
            Control *control = &row[j];
            control->rect.y = i - table->offset + table->rect.y +
                              header_offset;
            if (i >= table->offset &&
                i < table->offset + MAX_TABLE_VISIBLE_ROWS) {
                control_draw(win, &row[j], draw_time);
            }
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

    table->offset = table->rows->length - MAX_TABLE_VISIBLE_ROWS - 1;
    table->offset = table->rows->length - 1 - MAX_TABLE_VISIBLE_ROWS;
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

Layout *layout_init(void) {
    RefList *tables = ref_list_init();
    if (tables == NULL) {
        return NULL;
    }

    Layout *layout = malloc(sizeof(Layout));

    *layout = (Layout){
        .tables = tables,
        .focus = NULL};

    return layout;
}

bool layout_add_table(Layout *layout, ControlTable *table) {
    return ref_list_add(layout->tables, table);
}

void layout_draw(WINDOW *win, Layout const *layout, int draw_time) {
    for (int i = 0; i < layout->tables->length; i++) {
        control_table_draw(win, ref_list_get(layout->tables, i), draw_time);
    }
}

void layout_focus_clear(Layout *layout) {
    ControlTable *table = layout->focus;
    if (table != NULL) {
        control_table_focus_clear(table);
    }

    layout->focus = NULL;
}

void layout_focus_first(Layout *layout) {
    ControlTable *focus = layout->focus;
    if (focus == NULL) {
        bool find = false;
        int min_x;
        int min_y;

        for (int i = 0; i < layout->tables->length; i++) {
            ControlTable *table = ref_list_get(layout->tables, i);
            if (!find || table->rect.x < min_x || table->rect.y < min_y) {
                min_x = table->rect.x;
                min_y = table->rect.y;
                focus = table;
                find = true;
            }
        }
    }

    control_table_focus_first(focus);
    layout->focus = focus;
}

void layout_focus_last(Layout *layout) {
    ControlTable *focus = layout->focus;
    if (focus == NULL) {
        bool find = false;
        int min_x;
        int min_y;

        for (int i = 0; i < layout->tables->length; i++) {
            ControlTable *table = ref_list_get(layout->tables, i);
            if (!find || table->rect.x < min_x || table->rect.y < min_y) {
                min_x = table->rect.x;
                min_y = table->rect.y;
                focus = table;
                find = true;
            }
        }
    }

    control_table_focus_last(focus);
    layout->focus = focus;
}

void layout_focux_first_col_or_same(Layout *layout, int x, int y) {
    ControlTable *focus = NULL;
    for (int i = 0; i < layout->tables->length; i++) {
        ControlTable *table = ref_list_get(layout->tables, i);
        if (x >= table->rect.x && x <= table->rect.x + table->rect.width &&
            y >= table->rect.y && y <= table->rect.y + table->rect.height) {
            focus = table;
            break;
        }
    }

    if (layout->focus != NULL && layout->focus != focus) {
        layout_focus_clear(layout);
    }

    if (layout->focus == NULL && focus != NULL) {
        if (control_table_focus_first_col(focus, y)) {
            layout->focus = focus;
        }
    }
}

// if not exact find closest
void layout_focus_point(Layout *layout, int x, int y, bool exact) {
    ControlTable *focus = NULL;
    for (int i = 0; i < layout->tables->length; i++) {
        ControlTable *table = ref_list_get(layout->tables, i);
        if (x >= table->rect.x && x <= table->rect.x + table->rect.width &&
            y >= table->rect.y && y <= table->rect.y + table->rect.height) {
            focus = table;
            break;
        }
    }

    if (layout->focus != NULL && layout->focus != focus) {
        layout_focus_clear(layout);
    }

    if (focus != NULL) {
        if (control_table_focus_point(focus, x, y, false)) {
            layout->focus = focus;
        }
    }
}

void layout_focus_add(Layout *layout, int x, int y, bool jump) {
    if (layout->focus == NULL) {
        bool find = false;
        int min_x;
        int min_y;
        ControlTable *focus = NULL;

        for (int i = 0; i < layout->tables->length; i++) {
            ControlTable *table = ref_list_get(layout->tables, i);
            if (!find || table->rect.x < min_x || table->rect.y < min_y) {
                min_x = table->rect.x;
                min_y = table->rect.y;
                focus = table;
                find = true;
            }
        }

        Cursor _;
        if (control_table_focus_add(focus, x, y, false, &_)) {
            layout->focus = focus;
        }
    } else {
        Cursor cursor;
        bool can_focus = control_table_focus_add(layout->focus, x, y,
                                                 jump, &cursor);
        if (can_focus && cursor.jump) {
            bool find = false;
            int min_d;
            ControlTable *focus = NULL;
            Point origin = (Point){.x = cursor.x - x, .y = cursor.y - y};
            Point ray = (Point){.x = x, .y = y};
            Point inter;

            for (int i = 0; i < layout->tables->length; i++) {
                ControlTable *table = ref_list_get(layout->tables, i);
                if (table == layout->focus ||
                    !rect_ray_intersection(&table->rect, &origin,
                                           &ray, &inter)) {
                    continue;
                }

                Point local_inter = (Point){
                    .x = inter.x - origin.x,
                    .y = inter.y - origin.y};
                double d = length(&local_inter);
                if (!find || d < min_d) {
                    min_d = d;
                    focus = table;
                    find = true;
                }
            }

            if (focus != NULL && layout->focus != focus) {
                control_table_focus_clear(layout->focus);
                layout->focus = focus;
                control_table_focus_point(focus, cursor.x, cursor.y, false);
            }
        }
    }
}

void layout_free(Layout *layout) {
    for (int i = 0; i < layout->tables->length; i++) {
        control_table_free(ref_list_get(layout->tables, i));
    }
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
        control_init_int(&song->bpm, false),
        control_init_int(&song->step, false),
        control_init_text(&song->name, 17, true),
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

    for (int i = 0; i < 16; i++) {
        Control arrangement_row[8] = {
            control_init_int(&song->patterns[i][0], true),
            control_init_int(&song->patterns[i][1], true),
            control_init_int(&song->patterns[i][2], true),
            control_init_int(&song->patterns[i][3], true),
            control_init_int(&song->patterns[i][4], true),
            control_init_int(&song->patterns[i][5], true),
            control_init_int(&song->patterns[i][6], true),
            control_init_int(&song->patterns[i][7], true),
        };

        if (!control_table_add(arrangement_table, arrangement_row)) {
            control_table_free(arrangement_table);
            return NULL;
        }
    }

    return arrangement_table;
}

Layout *init_song_layout(Song *const song) {
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
        .input_repr_printed_at = -1,
        .focus = NULL,
        .focus_control = NULL};

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

bool interface_is_editing(Interface const *interface) {
    return interface->focus_control != NULL && interface->focus_control->edit;
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
        return control_empty(interface->focus_control);
    }

    return false;
}

bool handle_edit_input(Interface *interface, Input const *input) {
    if (!interface_is_editing(interface)) {
        return false;
    }

    if (input->type != INPUT_TYPE_KEY) {
        return false;
    }

    if (!input->key.special) {
        InputResult result = control_handle_input(interface->focus_control,
                                                  input->key.ch);
        if (result.done) {
            interface_save_edit(interface);
        }

        return result.handled;
    } else {
        return false;
    }
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

    if (handle_focus(interface, input)) {
        return true;
    }

    if (handle_edit(interface, input)) {
        return true;
    }

    if (handle_edit_input(interface, input)) {
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
