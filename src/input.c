#include "input.h"

const char MODIFIER_KEY_CTRL = 1;
const char MODIFIER_KEY_ALT = 1 << 1;

const char *ARROW_ESC = "\x1B\x5B\0";
const char *MOD_ARROW_ESC = "\x1B\x5B\x31\x3B\0";
const char *MOUSE_ESC = "\x1B\x5B\x4D\0";
const char *DEL_ESC = "\x1B\x5B\x33\x7E\0";

Input input_init_key(char const ch) {
    return (Input){
        .type = INPUT_TYPE_KEY,
        .key.special = false,
        .key.ch = ch};
}

Input input_init_special(SpecialKey key) {
    return (Input){
        .type = INPUT_TYPE_KEY,
        .key.special = true,
        .key.special_key = key};
}

Input input_init_modified_key(char modifier, char const ch) {
    return (Input){
        .type = INPUT_TYPE_KEY,
        .key.special = false,
        .key.modifier = modifier,
        .key.ch = ch};
}

Input input_init_modified_special(char modifier, SpecialKey key) {
    return (Input){
        .type = INPUT_TYPE_KEY,
        .key.special = true,
        .key.modifier = modifier,
        .key.special_key = key};
}

bool starts_with(char const *haystack, int len, char const *needle) {
    for (int i = 0; i < len; i++) {
        const char h = haystack[i];
        const char n = needle[i];
        if (n == 0) {
            return true;
        }

        if (h != n) {
            return false;
        }
    }

    return needle[len] == 0;
}

char *read_chars(int *out_len) {
    struct pollfd fd = {
        .fd = 0,
        .events = POLLIN};
    char *ex;

    bool ready = false;
    size_t len = 0;

    *out_len = 0;
    char *keys = NULL;

    while (!ready) {
        if (POLLIN == poll(&fd, 1, 0)) {
            char tmp[INPUT_BLOCK_SIZE + 1] = { 0 };
            const size_t read_size = read(0, tmp, INPUT_BLOCK_SIZE);
            if (read_size == 0) {
                return NULL;
            }

            tmp[INPUT_BLOCK_SIZE] = 0;
            len += read_size;

            if (keys == NULL) {
                ex = malloc(read_size + 1);
                if (ex == NULL) {
                    return NULL;
                }
                keys = ex;
                keys[0] = 0;
            } else {
                ex = realloc(keys, len + 1);
                if (ex == NULL) {
                    free(keys);
                    return NULL;
                }
                keys = ex;
            }

            strcat(keys, tmp);
            keys[len] = 0;
            if (read_size < INPUT_BLOCK_SIZE) {
                *out_len = len;
                return keys;
            }
        } else
            break;
    }

    *out_len = len;
    return keys;
}

void free_chars(char *keys) {
    free(keys);
}

bool parse_single(char key, Input *input) {
    if (key == 9) {
        *input = input_init_special(SPECIAL_KEY_TAB);
        return true;
    } else if (key == 13) {
        *input = input_init_special(SPECIAL_KEY_ENTER);
        return true;
    } else if (key == 27) {
        *input = input_init_special(SPECIAL_KEY_ESC);
        return true;
    } else if ((key & 96) == 0) {
        char rev = key + 96;
        if (key == 0) {
            rev = 32;
        } else if ((rev < 48 || rev > 57) && (rev < 97 || rev > 122)) {
            rev = key + 24;
        }

        *input = input_init_modified_key(MODIFIER_KEY_CTRL, rev);
        return true;
    } else {
        *input = input_init_key(key);
        return true;
    }

    return true;
}

bool parse_double(char const *keys, Input *input) {
    if (keys[0] == 27 && parse_single(keys[1], input)) {
        input->key.modifier |= MODIFIER_KEY_ALT;
        return true;
    }

    return false;
}

SpecialKey char_to_arrow(char key) {
    if (key == 68) {
        return SPECIAL_KEY_LEFT;
    } else if (key == 65) {
        return SPECIAL_KEY_UP;
    } else if (key == 67) {
        return SPECIAL_KEY_RIGHT;
    } else if (key == 66) {
        return SPECIAL_KEY_DOWN;
    }

    return 0;
}

bool parse_del(char const *keys, Input *input) {
    if (starts_with(keys, 4, DEL_ESC)) {
        *input = input_init_special(SPECIAL_KEY_DEL);
        return true;
    }

    return false;
}

bool parse_arrows(char const *keys, Input *input) {
    if (starts_with(keys, 3, ARROW_ESC)) {
        *input = input_init_special(char_to_arrow(keys[2]));
        return true;
    }

    return false;
}

bool parse_navigation(char const *keys, Input *input) {
    if (starts_with(keys, 6, MOD_ARROW_ESC)) {
        int modifier = 0;
        const int check = keys[4] - 33;
        if ((check & 2) == 2) {
            modifier |= MODIFIER_KEY_ALT;
        }
        if ((check & 4) == 4) {
            modifier |= MODIFIER_KEY_CTRL;
        }

        *input = input_init_modified_special(modifier, char_to_arrow(keys[5]));
        return true;
    } else if (starts_with(keys, 6, MOUSE_ESC)) {
        const char b = keys[3] - 32;
        const char x = keys[4] - 32;
        const char y = keys[5] - 32;

        if (b == 0 || b == 3) {
            MouseEvent event = b == 0
                                   ? MOUSE_EVENT_PRESS
                                   : MOUSE_EVENT_RELEASE;
            MouseButton button = MOUSE_BUTTON;
            *input = (Input){
                .type = INPUT_TYPE_MOUSE,
                .mouse.button = button,
                .mouse.event = event,
                .mouse.point = (Point){
                    .x = x,
                    .y = y}};
            return 1;
        } else if (b == 64 || b == 65) {
            MouseEvent event = MOUSE_EVENT_PRESS;
            MouseButton button = b == 64
                                     ? MOUSE_BUTTON_WHEEL_UP
                                     : MOUSE_BUTTON_WHEEL_DOWN;
            *input = (Input){
                .type = INPUT_TYPE_MOUSE,
                .mouse.button = button,
                .mouse.event = event,
                .mouse.point = (Point){
                    .x = x,
                    .y = y}};
            return true;
        }
    }

    return false;
}

int parse(char const *keys, int len, Input *input) {
    if (len >= 6 && parse_navigation(keys, input)) {
        return 6;
    }

    if (len >= 4 && parse_del(keys, input)) {
        return 4;
    }

    if (len >= 3 && parse_arrows(keys, input)) {
        return 3;
    }

    if (len >= 2 && parse_double(keys, input)) {
        return 2;
    }

    if (parse_single(keys[0], input)) {
        return 1;
    }

    return 0;
}

Input *input_read(int *out_len) {
    *out_len = 0;

    int len;
    char *keys = read_chars(&len);

    Input *input = NULL;
    int cap = INPUT_CAPACITY;
    for (int i = 0; i < len;) {
        if (input == NULL) {
            input = malloc(sizeof(Input) * cap);
        } else if (*out_len >= cap) {
            cap *= 2;
            Input *ex = realloc(input, sizeof(Input) * cap);
            if (ex == NULL) {
                free(input);
                *out_len = 0;
                return NULL;
            }

            input = ex;
        }

        int parsed = parse(keys, i > len - 6 ? len - i : 6, &input[*out_len]);
        if (parsed > 0) {
            i += parsed;
            *out_len += 1;
        }
    }

    free_chars(keys);
    return input;
}

void free_input(Input *input) {
    free(input);
}

typedef struct
{
    char *buffer;
    size_t length;
} Repr;

Repr repr_init(void) {
    return (Repr){
        .buffer = NULL,
        .length = 0};
}

void repr_concat(Repr *repr, char const *value) {
    const size_t len = strlen(value);
    if (repr->length > 0) {
        repr->buffer = realloc(repr->buffer, repr->length + len + 1);
        strcat(repr->buffer, value);
    } else {
        repr->buffer = malloc(len + 1);
        memcpy(repr->buffer, value, len);
        repr->buffer[len] = 0;
    }

    repr->length += len;
}

void repr_add(Repr *repr, char value) {
    if (repr->length > 0) {
        repr->buffer = realloc(repr->buffer, repr->length + 2);
    } else {
        repr->buffer = malloc(2);
    }

    repr->buffer[repr->length] = value;
    repr->buffer[repr->length + 1] = 0;
    repr->length += 1;
}

char *input_repr(Input const *input) {
    Repr repr = repr_init();
    if (input->type == INPUT_TYPE_KEY) {
        if ((input->key.modifier & MODIFIER_KEY_CTRL) == MODIFIER_KEY_CTRL) {
            repr_concat(&repr, "CTRL + ");
        }
        if ((input->key.modifier & MODIFIER_KEY_ALT) == MODIFIER_KEY_ALT) {
            repr_concat(&repr, "ALT + ");
        }

        if (input->key.special) {
            if (input->key.special_key == SPECIAL_KEY_ESC) {
                repr_concat(&repr, "ESC");
            } else if (input->key.special_key == SPECIAL_KEY_TAB) {
                repr_concat(&repr, "TAB");
            } else if (input->key.special_key == SPECIAL_KEY_ENTER) {
                repr_concat(&repr, "ENTER");
            } else if (input->key.special_key == SPECIAL_KEY_UP) {
                repr_concat(&repr, "UP");
            } else if (input->key.special_key == SPECIAL_KEY_DOWN) {
                repr_concat(&repr, "DOWN");
            } else if (input->key.special_key == SPECIAL_KEY_LEFT) {
                repr_concat(&repr, "LEFT");
            } else if (input->key.special_key == SPECIAL_KEY_RIGHT) {
                repr_concat(&repr, "RIGHT");
            }
        } else {
            if (input->key.ch == ' ') {
                repr_concat(&repr, "SPACE");
            } else if (input->key.ch == 127) {
                repr_concat(&repr, "BACKSPACE");
            } else {
                repr_add(&repr, input->key.ch);
            }
        }
    } else if (input->mouse.event == MOUSE_EVENT_PRESS) {
        repr_concat(&repr, "mouse: ");
        if (input->mouse.button == MOUSE_BUTTON) {
            repr_concat(&repr, "LEFT ");
        } else if (input->mouse.button == MOUSE_BUTTON_WHEEL_UP) {
            repr_concat(&repr, "UP ");
        } else if (input->mouse.button == MOUSE_BUTTON_WHEEL_DOWN) {
            repr_concat(&repr, "DOWN ");
        }

        char pos[8];
        sprintf(pos, "%d %d", input->mouse.point.x, input->mouse.point.y);
        repr_concat(&repr, pos);
    }

    return repr.buffer;
}

void input_repr_free(char * repr) {
    free(repr);
}

bool input_eq(Input const *a, Input const *b) {
    InputType type = a->type;
    if (type != b->type) {
        return false;
    }

    if (type == INPUT_TYPE_KEY) {
        bool special = a->key.special;

        if (special != b->key.special) {
            return false;
        }

        if (a->key.modifier != b->key.modifier) {
            return false;
        }

        if (special) {
            return a->key.special_key == b->key.special_key;
        } else {
            return a->key.ch == b->key.ch;
        }
    } else {
        if (a->mouse.event != b->mouse.event) {
            return false;
        }

        if (a->mouse.button != b->mouse.button) {
            return false;
        }

        return a->mouse.point.x == b->mouse.point.x &&
               a->mouse.point.y == b->mouse.point.y;
    }
}

bool input_mouse_event_eq(Input const *a, MouseEvent event, MouseButton button) {
    if (a->type != INPUT_TYPE_MOUSE) {
        return false;
    }

    return a->mouse.event == event && a->mouse.button == button;
}
