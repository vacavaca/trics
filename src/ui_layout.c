#include "ui_layout.h"

Layout *layout_init(State *const state) {
    RefList *tables = ref_list_init();
    if (tables == NULL) {
        return NULL;
    }

    Layout *layout = malloc(sizeof(Layout));
    *layout = (Layout){
        .tables = tables,
        .state = state,
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

void layout_refresh(Layout *layout) {
    for (int i = 0; i < layout->tables->length; i++) {
        control_table_refresh(ref_list_get(layout->tables, i));
    }
}

void layout_update(Layout *layout, int draw_time) {
    for (int i = 0; i < layout->tables->length; i++) {
        control_table_update(ref_list_get(layout->tables, i), draw_time);
    }
}

void layout_focus_clear(Layout *layout) {
    ControlTable *table = layout->focus;
    layout->focus = NULL;

    if (table != NULL) {
        control_table_focus_clear(table);
    }
}

void layout_focus_first(Layout *layout) {
    ControlTable *focus = layout->focus;
    if (focus == NULL) {
        bool find = false;
        int min_x;
        int min_y;

        for (int i = 0; i < layout->tables->length; i++) {
            ControlTable *table = ref_list_get(layout->tables, i);
            if (!find || table->widget->rect.x < min_x || table->widget->rect.y < min_y) {
                min_x = table->widget->rect.x;
                min_y = table->widget->rect.y;
                focus = table;
                find = true;
            }
        }
    }

    layout->focus = focus;
    control_table_focus_first(focus);
}

void layout_focus_last(Layout *layout) {
    ControlTable *focus = layout->focus;
    if (focus == NULL) {
        bool find = false;
        int min_x;
        int min_y;

        for (int i = 0; i < layout->tables->length; i++) {
            ControlTable *table = ref_list_get(layout->tables, i);
            if (!find || table->widget->rect.x < min_x || table->widget->rect.y < min_y) {
                min_x = table->widget->rect.x;
                min_y = table->widget->rect.y;
                focus = table;
                find = true;
            }
        }
    }

    layout->focus = focus;
    control_table_focus_last(focus);
}

void layout_focux_first_col_or_same(Layout *layout, int x, int y) {
    ControlTable *focus = NULL;
    for (int i = 0; i < layout->tables->length; i++) {
        ControlTable *table = ref_list_get(layout->tables, i);
        if (x >= table->widget->rect.x && x <= table->widget->rect.x + table->widget->rect.width &&
            y >= table->widget->rect.y && y <= table->widget->rect.y + table->widget->rect.height) {
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
    Point point = (Point){ .x = x, .y = y };
    for (int i = 0; i < layout->tables->length; i++) {
        ControlTable *table = ref_list_get(layout->tables, i);
        if (rect_contains(&table->widget->rect, &point)) {
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
            if (!find || table->widget->rect.x < min_x || table->widget->rect.y < min_y) {
                min_x = table->widget->rect.x;
                min_y = table->widget->rect.y;
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
                    !rect_ray_intersection(&table->widget->rect, &origin,
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
