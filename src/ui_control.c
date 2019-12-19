#include "ui_control.h"

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
        .on_edit = NULL,
        .layout = NULL,
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
        .on_edit = NULL,
        .layout = NULL,
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
        .on_edit = NULL,
        .layout = NULL,
        .rect = (Rect){
            .x = 0,
            .y = 0,
            .width = width,
            .height = 1}};
}

Control control_init_self_int(int value, bool allow_empty, void *layout,
                              void (*on_edit)(void *)) {
    return (Control){
        .type = CONTROL_TYPE_SELF_INT,
        .self_int_value = value,
        .focus = false,
        .focused_at = -1,
        .edit = false,
        .text_edit_reseted = false,
        .num_edit_reseted = false,
        .allow_empty = allow_empty,
        .edit_value = NULL,
        .on_edit = on_edit,
        .layout = layout,
        .rect = (Rect){
            .x = 0,
            .y = 0,
            .width = 2,
            .height = 1}};
}

char *repr_bool(bool value) {
    char *str = malloc(3);
    sprintf(str, "%02x", value);
    return str;
}

char *repr_int(int value) {
    char *str = malloc(3);
    if (value != EMPTY) {
        sprintf(str, "%02x", value - 1);
    } else {
        sprintf(str, "--");
    }
    return str;
}

char *control_repr_bool(Control const *control) {
    return repr_bool(*control->bool_value);
}

char *control_repr_int(Control const *control) {
    if (control->type == CONTROL_TYPE_INT) {
        return repr_int(*control->int_value);
    } else if (control->type == CONTROL_TYPE_SELF_INT) {
        return repr_int(control->self_int_value);
    }

    return NULL;
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
    } else if (control->type == CONTROL_TYPE_INT ||
               control->type == CONTROL_TYPE_SELF_INT) {
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
        if (repr != NULL) {
            wprintw(win, repr);
            free(repr);
        }
    } else {
        if (control->edit_value != NULL) {
            char *repr = text_cut(control->edit_value, control->rect.width,
                                  true, false);
            if (repr != NULL) {
                wprintw(win, "%s", repr);
                free(repr);
            }
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

void control_handle_step_input(Control *control, int i) {
    if (i == 0) {
        return;
    }

    bool control_bool = control->type == CONTROL_TYPE_BOOL;
    bool control_int = control->type == CONTROL_TYPE_INT;
    bool control_self_int = control->type == CONTROL_TYPE_SELF_INT;
    if (control_bool || control_int || control_self_int) {
        int value = EMPTY;
        if (strcmp(control->edit_value,"--")) {
            value = strtol(control->edit_value, NULL, 16);
            if (errno != EINVAL) {
                value += 1;
            } else {
                value = EMPTY;
            }
        }

        if (control_bool) {
            value = value + i > 1 ? 2 : 1;
        } else {
            value += i;
        }

        if (value <= EMPTY) {
            value = EMPTY;
            if (!control->allow_empty) {
                value = 1;
            }
        } else if (value >= 256) {
            value = 256;
        }

        free(control->edit_value);
        if (control_bool) {
            control->edit_value = repr_bool(value);
        } else {
            control->edit_value = repr_int(value);
        }
    }
}

void control_handle_multiplier_input(Control *control, double i) {
    if (i == 1) {
        return;
    }

    bool control_bool = control->type == CONTROL_TYPE_BOOL;
    bool control_int = control->type == CONTROL_TYPE_INT;
    bool control_self_int = control->type == CONTROL_TYPE_SELF_INT;
    if (control_bool || control_int || control_self_int) {
        int value = EMPTY;
        if (strcmp(control->edit_value,"--")) {
            value = strtol(control->edit_value, NULL, 16);
            if (errno == EINVAL) {
                value = EMPTY;
            }
        }

        if (control_bool) {
            value = (int)round((double)value * i) > 1 ? 2 : 1;
        } else {
            value = (int)round((double)value * i);
        }

        value += 1;

        if (value <= EMPTY) {
            value = EMPTY;
            if (!control->allow_empty) {
                value = 1;
            }
        } else if (value >= 256) {
            value = 256;
        }

        free(control->edit_value);
        if (control_bool) {
            control->edit_value = repr_bool(value);
        } else {
            control->edit_value = repr_int(value);
        }
    }
}

InputResult control_handle_num_input(Control *control, unsigned char key) {
    if (key == 'j') {
        control_handle_step_input(control, -1);
    } else if (key == 'k') {
        control_handle_step_input(control, 1);
    }

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
        control->type == CONTROL_TYPE_INT ||
        control->type == CONTROL_TYPE_SELF_INT) {
        return control_handle_num_input(control, key);
    } else if (control->type == CONTROL_TYPE_TEXT) {
        return control_handle_text_input(control, key);
    }

    return (InputResult) { .handled = false };
}

bool control_handle_wheel_input(Control *control, Point const *point, int i) {
    if (!rect_contains(&control->rect, point)) {
        return false;
    }

    control_handle_step_input(control, i);
    return true;
}

void control_save_edit(Control *control) {
    if (!control->edit) {
        return;
    }

    if (control->edit_value != NULL) {
        bool control_bool = control->type == CONTROL_TYPE_BOOL;
        bool control_int = control->type == CONTROL_TYPE_INT;
        bool control_self_int = control->type == CONTROL_TYPE_SELF_INT;
        if (control_bool || control_int || control_self_int) {
            if (strcmp(control->edit_value,"--")) {
                int value = strtol(control->edit_value, NULL, 16);
                if (errno != EINVAL) {
                    if (control_bool) {
                        *control->bool_value = value > 1 ? 2 : 1;
                    } else if (control_int) {
                        *control->int_value = value + 1;
                    } else if (control_self_int) {
                        control->self_int_value = value + 1;
                    }
                }
            } else {
                if (control_bool) {
                    *control->bool_value = 0;
                } else if (control_int) {
                   *control->int_value = EMPTY;
                } else if (control_self_int) {
                   control->self_int_value = EMPTY;
                }
            }

            free(control->edit_value);
        } else if (control->type == CONTROL_TYPE_TEXT) {
            free(*control->text_value);
            *control->text_value = control->edit_value;
        } else {
            free(control->edit_value);
        }

        control->edit_value = NULL;
    }

    control->edit = false;
    control->text_edit_reseted = false;
    control->num_edit_reseted = false;

    if (control->on_edit != NULL) {
        control->on_edit(control);
    }
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

bool control_empty(Control* control) {
    if (!control->allow_empty) {
        return false;
    }

    control_discard_edit(control);
    bool control_bool = control->type == CONTROL_TYPE_BOOL;
    bool control_int = control->type == CONTROL_TYPE_INT;
    bool control_self_int = control->type == CONTROL_TYPE_SELF_INT;
    if (control_bool || control_int || control_self_int) {
        if (control_bool) {
            *control->bool_value = EMPTY;
        } else if (control_int) {
            *control->int_value = EMPTY;
        } else if (control_self_int) {
            control->self_int_value = EMPTY;
        }
    } else {
        free(*control->text_value);
        char *val = malloc(1);
        val[0] = 0;
        *control->text_value = val;
    }

    if (control->on_edit != NULL) {
        control->on_edit(control);
    }

    return true;
}

void control_free(Control *control) {
    if (control->edit_value != NULL) {
        free(control->edit_value);
        control->edit_value = NULL;
    }
}
