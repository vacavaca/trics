#include "input.h"

const char MODIFIER_KEY_CTRL = 1;
const char MODIFIER_KEY_ALT = 1 << 1;

const char *ARROW_ESC = "\x1B\x5B\0";
const char *MOD_ARROW_ESC = "\x1B\x5B\x31\x3B\0";
const char *MOUSE_ESC = "\x1B\x5B\x4D\0";

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

int get_chars(char **keys) {
    struct pollfd fd = {
        .fd = 0,
        .events = POLLIN};
    char *ex;

    bool ready = false;
    size_t len = 0;

    *keys = NULL;

    while (!ready) {
        if (POLLIN == poll(&fd, 1, 0)) {
            char tmp[INPUT_BLOCK_SIZE + 1];
            const size_t read_size = read(0, tmp, INPUT_BLOCK_SIZE);
            if (read_size == 0) {
                return 0;
            }

            tmp[INPUT_BLOCK_SIZE] = 0;
            len += read_size;

            if (*keys == NULL) {
                ex = malloc(read_size + 1);
                if (ex == NULL) {
                    return 0;
                }
                *keys = ex;
                *keys[0] = 0;
            } else {
                ex = realloc(*keys, len + 1);
                if (ex == NULL) {
                    free(*keys);
                    return 0;
                }
                *keys = ex;
            }

            strcat(*keys, tmp);
            (*keys)[len] = 0;
            if (read_size < INPUT_BLOCK_SIZE) {
                return len;
            }
        } else
            break;
    }

    return len;
}

int parse_single(char key, Input *input) {
    if (key == 9) {
        *input = (Input){
            .type = INPUT_TYPE_KEY,
            .key.special = true,
            .key.special_key = SPECIAL_KEY_TAB};
        return 1;
    } else if (key == 13) {
        *input = (Input){
            .type = INPUT_TYPE_KEY,
            .key.special = true,
            .key.special_key = SPECIAL_KEY_ENTER};
        return 1;
    } else if (key == 27) {
        *input = (Input){
            .type = INPUT_TYPE_KEY,
            .key.special = true,
            .key.special_key = SPECIAL_KEY_ESC};
        return 1;
    } else if ((key & 96) == 0) {
        char rev = key + 96;
        if (key == 0) {
            rev = 32;
        } else if ((rev < 48 || rev > 57) && (rev < 97 || rev > 122)) {
            rev = key + 24;
        }

        *input = (Input){
            .type = INPUT_TYPE_KEY,
            .key.special = false,
            .key.modifier = MODIFIER_KEY_CTRL,
            .key.ch = rev};
        return 1;
    } else {
        *input = (Input){
            .type = INPUT_TYPE_KEY,
            .key.special = false,
            .key.ch = key};
        return 1;
    }

    return 0;
}

int parse_double(char const *keys, Input *input) {
    if (keys[0] == 27 && parse_single(keys[1], input)) {
        input->key.modifier |= MODIFIER_KEY_ALT;
        return 1;
    }

    return 0;
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

int parse_arrows(char const *keys, Input *input) {
    if (starts_with(keys, 3, ARROW_ESC)) {
        *input = (Input){
            .type = INPUT_TYPE_KEY,
            .key.special = true,
            .key.special_key = char_to_arrow(keys[2])};
        return 1;
    }

    return 0;
}

int parse_navigation(char const *keys, Input *input) {
    if (starts_with(keys, 6, MOD_ARROW_ESC)) {
        int modifier = 0;
        const int check = keys[4] - 33;
        if ((check & 2) == 2) {
            modifier |= MODIFIER_KEY_ALT;
        }
        if ((check & 4) == 4) {
            modifier |= MODIFIER_KEY_CTRL;
        }

        *input = (Input){
            .type = INPUT_TYPE_KEY,
            .key.special = true,
            .key.modifier = modifier,
            .key.special_key = char_to_arrow(keys[5])};
        return 1;
    } else if (starts_with(keys, 6, MOUSE_ESC)) {
        char b = keys[3] - 32;
        char x = keys[4] - 32;
        char y = keys[5] - 32;

        if (b == 0 || b == 3) {
            MouseEvent event = b == 0
                                   ? MOUSE_EVENT_PRESS
                                   : MOUSE_EVENT_RELEASE;
            MouseButton button = MOUSE_BUTTON;
            *input = (Input){
                .type = INPUT_TYPE_MOUSE,
                .mouse.button = button,
                .mouse.event = event,
                .mouse.x = x,
                .mouse.y = y};
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
                .mouse.x = x,
                .mouse.y = y};
            return 1;
        }
    }

    return 0;
}

int parse(char const *keys, int len, Input *input) {
    if (len == 1) {
        return parse_single(keys[0], input);
    } else if (len == 2) {
        return parse_double(keys, input);
    } else if (len == 3) {
        return parse_arrows(keys, input);
    } else if (len == 6) {
        return parse_navigation(keys, input);
    }

    return 0;
}

int input_read(Input *input) {
    char *keys;
    int len;
    if ((len = get_chars(&keys)) > 0) {
        return parse(keys, len, input);
    }

    return 0;
}

typedef struct
{
    char *buffer;
    size_t length;
} Repr;

Repr repr_init(void) {
    return (Repr){
        .buffer = (char[1]){0},
        .length = 0};
}

void repr_concat(Repr *repr, char const *value) {
    size_t len = strlen(value);
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
                repr_concat(&repr, "WS");
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
        sprintf(pos, "%d %d", input->mouse.x, input->mouse.y);
        repr_concat(&repr, pos);
    }

    return repr.buffer;
}
