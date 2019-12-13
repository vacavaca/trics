#ifndef INPUT_H
#define INPUT_H

#include <poll.h>    // poll
#include <stdbool.h> // bool
#include <stdio.h>   //sprintf
#include <stdlib.h>  // malloc realloc free
#include <string.h>  // strcat
#include <unistd.h>  // read

#define INPUT_BLOCK_SIZE 4

const char MODIFIER_KEY_CTRL;
const char MODIFIER_KEY_ALT;

typedef enum {
    INPUT_TYPE_KEY,
    INPUT_TYPE_MOUSE
} InputType;

typedef enum {
    SPECIAL_KEY_ESC,
    SPECIAL_KEY_TAB,
    SPECIAL_KEY_ENTER,
    SPECIAL_KEY_UP,
    SPECIAL_KEY_DOWN,
    SPECIAL_KEY_LEFT,
    SPECIAL_KEY_RIGHT,
} SpecialKey;

typedef enum {
    MOUSE_EVENT_PRESS,
    MOUSE_EVENT_RELEASE,
} MouseEvent;

typedef enum {
    MOUSE_BUTTON,
    MOUSE_BUTTON_WHEEL_UP,
    MOUSE_BUTTON_WHEEL_DOWN,
} MouseButton;

typedef struct {
    InputType type;
    union {
        struct {
            bool special;
            char modifier;
            union {
                SpecialKey special_key;
                char ch;
            };
        } key;
        struct {
            MouseEvent event;
            MouseButton button;
            int x;
            int y;
        } mouse;
    };
} Input;

int input_read(Input *input);

char * input_repr(Input const *input);

#endif // INPUT_H