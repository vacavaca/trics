#include "ui_control.h"

// ControlBool

ControlBool control_bool_init(volatile bool *const value) {
    return (ControlBool){
        .value = value,
        .edit_value = *value,
        .edit_text = NULL,
        .edit = false,
        .reseted = false};
}

char *bool_repr(bool value) {
    char *str = malloc(3);
    if (str == NULL) {
        return NULL;
    }

    sprintf(str, "%02x", value ? 1 : 0);
    return str;
}

char *control_bool_repr(ControlBool *control) {
    if (control->edit) {
        return control->edit_text;
    } else {
        return bool_repr(*control->value);
    }

    return bool_repr(*control->value);
}

bool control_bool_edit(ControlBool *control) {
    if (control->edit) {
        return false;
    }

    control->edit_value = *control->value ? 1 : 0;
    control->edit_text = bool_repr(control->edit_value);
    if (control->edit_text == NULL) {
        return false;
    }

    control->edit = true;
    control->reseted = false;
    return true;
}

bool control_bool_handle_step_input(ControlBool *control, int i) {
    if (i == 0 || !control->edit) {
        return false;
    }

    bool value = control->edit_value + i > 0 ? true : false;
    if (value != control->edit_value) {
        char* prev = control->edit_text;
        control->edit_text = bool_repr(value);
        if (control->edit_text == NULL) {
            control->edit_text = prev;
            return false;
        }
        free(prev);
        control->edit_value = value;
        control->reseted = false;
        return true;
    } else {
        return false;
    }
}

InputResult control_bool_handle_input(ControlBool *control, char key) {
    if (!control->edit) {
        return (InputResult) { .handled = false };
    }

    bool handled = false;
    if (key == 'j') {
        handled = control_bool_handle_step_input(control, -1);
    } else if (key == 'k') {
        handled = control_bool_handle_step_input(control, 1);
    }

    if (handled) {
        return (InputResult) { .handled = handled };
    }

    // non hex chars and not backspace
    if ((key < 48 || key > 57) && (key < 97 || key > 102) && key != 127) {
        return (InputResult) { .handled = false };
    }

    if (key == 127) {
        char *prev = control->edit_text;
        control->edit_text = bool_repr(false);
        if (control->edit_text == NULL) {
            control->edit_text = prev;
            return (InputResult) { .handled = false };
        }
        free(prev);
        control->edit_value = false;
        return (InputResult) { .handled = true };
    }

    if (!control->reseted) {
        control->edit_text[0] = ' ';
        control->edit_text[1] = key;

        int value = 0;
        value = strtol(control->edit_text, NULL, 16);
        if (errno == EINVAL) {
            value = 0;
        }

        if (value > 1) {
            value = 1;
        }

        control->edit_value = value > 0 ? true : false;
        control->reseted = true;
        return (InputResult) { .handled = true };
    } else {
        control->edit_text[0] = control->edit_text[1];
        control->edit_text[1] = key;
        int value = 0;
        value = strtol(control->edit_text, NULL, 16);
        if (errno == EINVAL) {
            value = 0;
        }

        if (value > 1) {
            value = 1;
        }
        control->edit_value = value > 0 ? true : false;
        return (InputResult) { .handled = true, .done = true };
    }
}

bool control_bool_empty(ControlBool *control) {
    if (control->edit) {
        char *prev = control->edit_text;
        control->edit_text = bool_repr(false);
        if (control->edit_text == NULL) {
            control->edit_text = prev;
            return false;
        }
        free(prev);
        control->edit_value = false;
    } else {
        *control->value = false;
        control->edit_value = false;
    }
    control->reseted = false;
    return true;
}

void control_bool_save_edit(ControlBool* control) {
    if (!control->edit) {
        return;
    }

    free(control->edit_text);
    control->edit_text = NULL;
    *control->value = control->edit_value;
    control->edit = false;
}

void control_bool_discard_edit(ControlBool* control) {
    if (!control->edit) {
        return;
    }

    free(control->edit_text);
    control->edit_text = NULL;
    control->edit = false;
}

void control_bool_free(ControlBool *control) {
    if (control->edit_text != NULL) {
        free(control->edit_text);
    }
}

// ControlInt

ControlInt control_int_init(volatile int *const value, bool allow_empty) {
    return (ControlInt){
        .value = value,
        .edit_value = *value,
        .edit_text = NULL,
        .allow_empty = allow_empty,
        .edit = false,
        .reseted = false};
}

char *int_repr(int value) {
    char *str = malloc(3);
    if (str == NULL) {
        return NULL;
    }

    if (value != EMPTY) {
        sprintf(str, "%02x", value - 1);
    } else {
        sprintf(str, "--");
    }
    return str;
}

char *control_int_repr(ControlInt *control) {
    if (control->edit) {
        return control->edit_text;
    } else {
        return int_repr(*control->value);
    }
}

bool control_int_edit(ControlInt *control) {
    if (control->edit) {
        return false;
    }

    control->edit_value = *control->value;
    control->edit_text = int_repr(control->edit_value);
    if (control->edit_text == NULL) {
        return false;
    }
    control->edit = true;
    control->reseted = false;

    return true;
}

int clamp_int(int value, bool allow_empty) {
    if (value < 0) {
        value = EMPTY;
    }

    if (!allow_empty && value == EMPTY) {
        value += 1;
    }

    if (value > 256) {
        value = 256;
    }

    return value;
}

bool control_int_handle_step_input(ControlInt *control, int i) {
    if (!control->edit || i == 0) {
        return false;
    }

    int value = clamp_int(control->edit_value + i, control->allow_empty);

    if (value != control->edit_value) {
        char *prev = control->edit_text;
        control->edit_text = int_repr(value);
        if (control->edit_text == NULL) {
            control->edit_text = prev;
            return false;
        }
        free(prev);
        control->edit_value = value;
        control->reseted = false;
        return true;
    } else {
        return false;
    }
}

bool control_int_handle_multiplier_input(ControlInt *control, double i) {
    if (!control->edit || i == 1.0) {
        return false;
    }

    int value = (int)round((double)control->edit_value * i);
    value = clamp_int(value, control->allow_empty);
    if (value != control->edit_value) {
        char *prev = control->edit_text;
        control->edit_text = int_repr(value);
        if (control->edit_text == NULL) {
            control->edit_text = prev;
            return false;
        }
        free(prev);
        control->edit_value = value;
        control->reseted = false;
        return true;
    } else {
        return false;
    }
}

InputResult control_int_handle_input(ControlInt *control, char key) {
    if (!control->edit) {
        return (InputResult) { .handled = false };
    }

    if (key == 'j') {
        control_int_handle_step_input(control, -1);
    } else if (key == 'k') {
        control_int_handle_step_input(control, 1);
    }

    // non hex chars and not backspace
    if ((key < 48 || key > 57) && (key < 97 || key > 102) && key != 127) {
        return (InputResult) { .handled = false };
    }

    if (key == 127) {
        if (!control->allow_empty) {
            return (InputResult) { .handled = false };
        }
        char *prev =control->edit_text;
        control->edit_text = int_repr(EMPTY);
        if (control->edit_text == NULL) {
            control->edit_text = prev;
            return (InputResult) { .handled = false };
        }
        free(prev);
        control->edit_value = EMPTY;
        return (InputResult) { .handled = true };
    }

    if (!control->reseted) {
        control->edit_text[0] = ' ';
        control->edit_text[1] = key;

        int value = 0;
        value = strtol(control->edit_text, NULL, 16);
        if (errno == EINVAL) {
            return (InputResult) { .handled = false };
        }

        value = clamp_int(value + 1, control->allow_empty);
        control->edit_value = value;
        control->reseted = true;
        return (InputResult) { .handled = true };
    } else {
        control->edit_text[0] = control->edit_text[1];
        control->edit_text[1] = key;
        int value = 0;
        value = strtol(control->edit_text, NULL, 16);
        if (errno == EINVAL) {
            return (InputResult) { .handled = false };
        }

        value = clamp_int(value + 1, control->allow_empty);
        control->edit_value = value;
        return (InputResult) { .handled = true, .done = true };
    }
}

bool control_int_empty(ControlInt *control) {
    if (!control->allow_empty) {
        return false;
    }

    if (control->edit) {
        char *prev = control->edit_text;
        control->edit_text = int_repr(EMPTY);
        if (control->edit_text != NULL) {
            control->edit_text = prev;
            return false;
        }
        free(prev);
        control->edit_value = EMPTY;
    } else {
        control->edit_value = EMPTY;
        *control->value = EMPTY;
    }

    control->reseted = false;
    return true;
}

void control_int_save_edit(ControlInt* control) {
    if (!control->edit) {
        return;
    }

    free(control->edit_text);
    control->edit_text = NULL;
    *control->value = control->edit_value;
    control->edit = false;
}

void control_int_discard_edit(ControlInt* control) {
    if (!control->edit) {
        return;
    }

    free(control->edit_text);
    control->edit_text = NULL;
    control->edit_value = *control->value;
    control->edit = false;
}

void control_int_free(ControlInt *control) {
    if (control->edit_text != NULL) {
        free(control->edit_text);
        control->edit_text = NULL;
    }
}

// ControlText

ControlText control_text_init(char **value, bool allow_empty,
                             int width) {
    return (ControlText){
        .value = value,
        .edit_value = NULL,
        .allow_empty = allow_empty,
        .edit = false,
        .reseted = false,
        .width = width,
        .start = 0};
}

char *text_cut(char *str, int start, int width, bool ellipsis) {
    const int len = strlen(str);
    char *result = malloc(width + 1);

    int rs = len - start;
    if (rs > width) {
        rs = width;
    }
    memcpy(result, str + start, rs + 1);

    result[width] = '\0';
    if (ellipsis && len - start >= width) {
        result[width - 1] = '.';
        result[width - 2] = '.';
        result[width - 3] = '.';
    }

    return result;
}

char *control_text_repr(ControlText *control) {
    if (control->edit) {
        return text_cut(control->edit_value, control->start,
                        control->width, false);
    } else {
        int len = strlen(*control->value);
        if (len == 0) {
            char *str = malloc(2);
            if (str == NULL) {
                return NULL;
            }
            str[0] = ' ';
            str[1] = '\0';
            return str;
        }

        return text_cut(*control->value, 0,
                        control->width, true);
    }
}

bool control_text_edit(ControlText *control) {
    if (control->edit) {
        return false;
    }

    int len = strlen(*control->value);
    control->edit_value = malloc(len + 1);
    if (control->edit_value == NULL) {
        return false;
    }
    memcpy(control->edit_value, *control->value, len + 1);
    control->edit = true;
    control->reseted = false;
    control->start = 0;
    return true;
}

InputResult control_text_handle_input(ControlText *control, unsigned char key) {
    if (!control->edit) {
        return (InputResult){ .handled = false };
    }

    // printable chars + backspace
    if (key < 32 || key > 127) {
        return (InputResult) { .handled = false };
    }

    // TODO backspace

    int len = strlen(control->edit_value);

    if (!control->reseted) {
        if (key != 127) {
            char *next = malloc(2);
            if (next == NULL) {
                return (InputResult) { .handled = false };
            }
            free(control->edit_value);
            sprintf(next, "%c", key);
            control->edit_value = next;
            control->reseted = true;
            return (InputResult) { .handled = true };
        } else {
            char *next = malloc(1);
            if (next == NULL) {
                return (InputResult) { .handled = false };
            }
            free(control->edit_value);
            next[0] = 0;
            control->edit_value = next;
            control->reseted = true;
            return (InputResult) { .handled = true };
        }
    } else {
        if (key != 127) {
            char *ex = realloc(control->edit_value, len + 2);
            if (ex == NULL) {
                return (InputResult) { .handled = false };
            }

            if (len >= control->width) {
                control->start = len - control->width + 1;
            }

            control->edit_value = ex;
            control->edit_value[len] = key;
            control->edit_value[len + 1] = '\0';
            return (InputResult) { .handled = true };
        } else {
            if (len > 0) {
                control->edit_value[len - 1] = '\0';
                return (InputResult) { .handled = true };
            }
        }
    }

    return (InputResult) { .handled = false };
}

bool control_text_empty(ControlText *control) {
    if (!control->edit) {
        return false;
    }

    char *next = malloc(1);
    if (next == NULL) {
        return false;
    }
    next[0] = 0;
    free(control->edit_value);
    control->edit_value = next;
    control->start = 0;
    control->reseted = false;
    return true;
}

void control_text_save_edit(ControlText* control) {
    if (!control->edit) {
        return;
    }

    free(*control->value);
    *control->value = control->edit_value;
    control->edit_value = NULL;
    control->edit = false;
    control->start = 0;
    control->reseted = false;
}

void control_text_discard_edit(ControlText* control) {
    if (!control->edit) {
        return;
    }

    free(control->edit_value);
    control->edit_value = NULL;
    control->edit = false;
    control->start = 0;
    control->reseted = false;
}

void control_text_free(ControlText *control) {
    if (control->edit_value != NULL) {
        free(control->edit_value);
    }
}

// ControlNote

ControlNote control_note_init(volatile int *const value,
                              int base_octave, bool allow_empty);

char *control_note_repr(ControlNote *control);

bool control_note_edit(ControlNote *control);

bool control_note_handle_step_input(ControlNote *control, int i);

bool control_note_handle_multiplier_input(ControlNote *control, double i);

InputResult control_note_handle_input(ControlNote *control, char key);

bool control_note_empty(Control *control);

void control_note_save_edit(ControlNote* control);

void control_note_discard_edit(ControlNote* control);

void control_note_free(ControlNote *control);

// Control

Control control_init_bool(volatile bool *value, bool allow_empty,
                          void (*on_change)(void *), void *layout) {
    return (Control){
        .type = CONTROL_TYPE_BOOL,
        .control_bool = control_bool_init(value),
        .rect = (Rect){ .x=0, .y=0, .width=2, .height=1 },
        .focus = false,
        .edit = false,
        .focused_at = -1,
        .on_change = on_change,
        .layout = layout};
}

Control control_init_int(volatile int *value, bool allow_empty,
                         void (*on_change)(void *), void *layout) {
    return (Control){
        .type = CONTROL_TYPE_INT,
        .control_int = control_int_init(value, allow_empty),
        .rect = (Rect){ .x=0, .y=0, .width=2, .height=1 },
        .focus = false,
        .edit = false,
        .focused_at = -1,
        .on_change = on_change,
        .layout = layout};
}

Control control_init_text(char **value, int width, bool allow_empty,
                          void (*on_change)(void *), void *layout) {
    return (Control){
        .type = CONTROL_TYPE_TEXT,
        .control_text = control_text_init(value, allow_empty, width),
        .rect = (Rect){ .x=0, .y=0, .width=width, .height=1 },
        .focus = false,
        .edit = false,
        .focused_at = -1,
        .on_change = on_change,
        .layout = layout};
}

Control control_init_note(volatile int *value, int base_octave,
                          bool allow_empty, void (*on_change)(void *),
                          void *layout);

char *control_repr(Control *control) {
    if (control->type == CONTROL_TYPE_BOOL) {
        return control_bool_repr(&control->control_bool);
    } else if (control->type == CONTROL_TYPE_INT) {
        return control_int_repr(&control->control_int);
    } else if (control->type == CONTROL_TYPE_TEXT) {
        return control_text_repr(&control->control_text);
    }

    char *str = malloc(3);
    if (str == NULL) {
        return NULL;
    }

    sprintf(str, "xx");
    return str;
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
    char *repr = control_repr(control);
    if (repr != NULL) {
        wprintw(win, repr);

        // only allocated
        if (!control->edit || control->type == CONTROL_TYPE_TEXT) {
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

void control_discard_edit(Control *control) {
    if (control->type == CONTROL_TYPE_BOOL) {
        control_bool_discard_edit(&control->control_bool);
    } else if (control->type == CONTROL_TYPE_INT) {
        control_int_discard_edit(&control->control_int);
    } else if (control->type == CONTROL_TYPE_TEXT) {
        control_text_discard_edit(&control->control_text);
    }

    control->edit = false;
}

void control_focus_clear(Control *control) {
    control->focus = false;
    if (control->edit) {
        control_discard_edit(control);
    }
    control->edit = false;
}

bool control_edit(Control *control) {
    if (control->edit) {
        return true;
    }

    bool result = false;
    if (control->type == CONTROL_TYPE_BOOL) {
        result = control_bool_edit(&control->control_bool);
    } else if (control->type == CONTROL_TYPE_INT) {
        result = control_int_edit(&control->control_int);
    } else if (control->type == CONTROL_TYPE_TEXT) {
        result = control_text_edit(&control->control_text);
    }

    if (result) {
        control->edit = true;
        return true;
    }

    return false;
}

bool control_handle_step_input(Control *control, int i) {
    if (control->type == CONTROL_TYPE_BOOL) {
        return control_bool_handle_step_input(&control->control_bool, i);
    } else if (control->type == CONTROL_TYPE_INT) {
        return control_int_handle_step_input(&control->control_int, i);
    }

    return false;
}

bool control_handle_multiplier_input(Control *control, double i) {
    if (control->type == CONTROL_TYPE_INT) {
        return control_int_handle_multiplier_input(&control->control_int, i);
    }

    return false;
}

InputResult control_handle_input(Control *control, char key) {
    if (control->type == CONTROL_TYPE_BOOL) {
        return control_bool_handle_input(&control->control_bool, key);
    } else if (control->type == CONTROL_TYPE_INT) {
        return control_int_handle_input(&control->control_int, key);
    } else if (control->type == CONTROL_TYPE_TEXT) {
        return control_text_handle_input(&control->control_text, key);
    }

    return (InputResult){ .handled = false };
}

bool control_handle_wheel_input(Control *control, Point const *point, int i) {
    if (control->type == CONTROL_TYPE_BOOL) {
        return control_bool_handle_step_input(&control->control_bool, i);
    } else if (control->type == CONTROL_TYPE_INT) {
        return control_int_handle_step_input(&control->control_int, i);
    }

    return false;
}

bool control_empty(Control* control) {
    if (control->type == CONTROL_TYPE_BOOL) {
        return control_bool_empty(&control->control_bool);
    } else if (control->type == CONTROL_TYPE_INT) {
        return control_int_empty(&control->control_int);
    } else if (control->type == CONTROL_TYPE_TEXT) {
        return control_text_empty(&control->control_text);
    }

    return false;
}

void control_save_edit(Control *control) {
    if (control->type == CONTROL_TYPE_BOOL) {
        control_bool_save_edit(&control->control_bool);
    } else if (control->type == CONTROL_TYPE_INT) {
        control_int_save_edit(&control->control_int);
    } else if (control->type == CONTROL_TYPE_TEXT) {
        control_text_save_edit(&control->control_text);
    }

    control->edit = false;
}

void control_free(Control *control) {
    if (control->type == CONTROL_TYPE_BOOL) {
        control_bool_free(&control->control_bool);
    } else if (control->type == CONTROL_TYPE_INT) {
        control_int_free(&control->control_int);
    } else if (control->type == CONTROL_TYPE_TEXT) {
        control_text_free(&control->control_text);
    }

    control->edit = false;
}

/*

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
        .base_octave = 0,
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
        .base_octave = 0,
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
        .base_octave = 0,
        .rect = (Rect){
            .x = 0,
            .y = 0,
            .width = width,
            .height = 1}};
}

Control control_init_note(volatile int *const value, int base_octave,
                          bool allow_empty) {
    return (Control){
        .type = CONTROL_TYPE_NOTE,
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
        .base_octave = base_octave,
        .rect = (Rect){
            .x = 0,
            .y = 0,
            .width = 4,
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
        .base_octave = 0,
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

const char *notes[12] = {
    "C ",
    "C#",
    "D ",
    "D#",
    "E ",
    "F ",
    "F#",
    "G ",
    "G#",
    "A ",
    "A#",
    "B ",
};

char *control_repr_note(Control const *control) {
    if (*control->int_value == EMPTY) {
        return "----";
    }
    const int value = *control->int_value - 1;
    const int octave = value / 12;
    const int d = octave - control->base_octave;
    const int note = value % 12;
    char *result = malloc(4);
    if (d != 0) {
        sprintf(result, "%s%s%d", notes[note], d > 0 ? " " : "", d);
    } else {
        sprintf(result, "%s  ", notes[note]);
    }

    return result;
}

char *control_repr(Control* control, bool cut, bool ellipsis) {
    if (control->type == CONTROL_TYPE_BOOL) {
        return control_repr_bool(control);
    } else if (control->type == CONTROL_TYPE_INT ||
               control->type == CONTROL_TYPE_SELF_INT) {
        return control_repr_int(control);
    } else if (control->type == CONTROL_TYPE_NOTE) {
        return control_repr_note(control);
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
*/
