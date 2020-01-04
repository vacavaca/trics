#include "input.h"
#include "state.h"
#include "ui_interface.h"
#include "audio.h"
#include "render.h"
#include <ncurses.h> // ncurses functions
#include <signal.h>  // signal
#include <stdbool.h>  // bool
#include <termios.h> // TC constants
#include <time.h>    // nanosleep
#include <unistd.h>  // STDIN_FILENO
#include <stdio.h>  // fprintf
#include <stdlib.h>  // exit

static WINDOW *win;

struct termios orig_termios;

void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON | ISIG);
    raw.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enable_mouse_tracking(void) {
    printf("\x1B[?1000h");
}

void disable_mouse_tracking(void) {
    printf("\x1B[?1000l");
}

void handle_resize(int sig) {
    endwin();
    wclear(win);
    winsertln(win);
}

void handle_exit(void) {
    disable_raw_mode();
    renderer_teardown();
    //endwin();
    disable_mouse_tracking();
}

void sleep_msec(int ms) {
    struct timespec ts;
    ts.tv_sec = ms / 1000;
    ts.tv_nsec = (ms % 1000) * 1000000;
    struct timespec r;
    nanosleep(&ts, &r);
}

int main(int argc, char *argv[]) {
    renderer_setup();
    Widget *table = widget_init_container(NULL, NULL, (Rect){ .x = 1, .y = 5, .width = 10, .height = 10 });

    for (int i = 0; i < 40; i += 2) {
        char *str = malloc(10);
        sprintf(str, "test %d\0", i);
        widget_init_text(NULL, table, (Rect){ .x = 1, .y = i, .width = 10, .height = 1 }, str);
    }

    widget_refresh(table);

    int i = 0;
    while (i < 100) {
        i += 1;

        container_scroll(table->container, 1);
        widget_refresh(table);

        SDL_Delay(300.0);
    }

    renderer_teardown();


    return 0;
    State *state = state_init((char *)"Song Title");
    if (state == NULL) {
        fprintf(stderr, "Failed to initialize state\n");
        exit(1);
    }

    AudioContext *ctx = audio_context_init(state);
    audio_context_play(ctx, 0);
    /*

    int start = SDL_GetTicks();

    int max = SAMPLE_RATE * 30;
    int e = 0;
    int fu = 0;
    int bu = 0;
    while (e < max) {
        int prev = e;
        e += 1024;
        if (prev / (SAMPLE_RATE / 2) != e / (SAMPLE_RATE / 2)) {
            if(audio_context_trigger_step(ctx, 1, 1, 59 -12 * 3  , 1, 1)) {
        int dbu = ctx->buffer_update_count - bu;
        int dfu = ctx->frames_update_count - fu;
        bu = ctx->buffer_update_count;
        fu = ctx->frames_update_count;
        printf("buffer: %d, queue: %d time: %f base_rate: %f  bu/s: %f  fu/s: %f\n", ctx->buffer->length, ctx->queue->length, ctx->time, 1 / 0.5, dbu / 0.5, dfu / 0.5);
            }
        }

        short * stream = malloc(sizeof(short) * 1024);
        typed_audio_callback(ctx, stream, 1024);
        free(stream);
    }

    int elapsed = SDL_GetTicks() - start;
    float total = ((float)(max) / 2 / (float)SAMPLE_RATE);
    float load = (float)elapsed / 1000.0 / total;
    printf("%d %d %f of %f\n", max, elapsed, load, total);

    return 0;


    int i = 0;
    int fu = 0;
    int bu = 0;
    float delay = 60.0 / 128. / 2;
    while (true) {
        i += 1;

        if(!audio_context_trigger_step(ctx, 1, 1, 59 -12 * 3  , 1, 15)) {
            printf("FAILED NOTE \n");
        }
        int dbu = ctx->buffer_update_count - bu;
        int dfu = ctx->frames_update_count - fu;
        bu = ctx->buffer_update_count;
        fu = ctx->frames_update_count;
        printf("buffer: %d, queue: %d time: %f base_rate: %f  bu/s: %f  fu/s: %f\n", ctx->buffer->length, ctx->queue->length, ctx->time, 1 / delay, dbu / delay, dfu / delay);
        SDL_Delay(1000.0 * delay);

        if (i > 400) {
            break;
        }

        printf("%d of 400\n", i);

       
    }

    printf("b %d  f %d\n", ctx->buffer_update_count, ctx->frames_update_count);

    return 0;

    */


    Interface *interface = interface_init(state);

    if (interface == NULL) {
        fprintf(stderr, "Failed to initialize interface\n");
        exit(1);
    }

    atexit(handle_exit);
    signal(SIGWINCH, handle_resize);

    enable_mouse_tracking();

    win = initscr();
    enable_raw_mode();
    curs_set(0);
    init_colors();

    interface_draw(win, interface, 0);

    int t = 0;
    Input *inputs = NULL;
    while (true) {
        sleep_msec(REFRESH_RATE_MSEC);

        int len = 0;
        inputs = input_read(&len);

        for (int i = 0; i < len; i++) {
            Input input = inputs[i];
            Input test = input_init_modified_key(MODIFIER_KEY_CTRL, 'c');
            if (input_eq(&input, &test)) {
                goto cleanup;
            }

            interface_handle_input(interface, &input);
        }

        free_input(inputs);
        inputs = NULL;
        //interface_update(interface, 0);
        wrefresh(win);
        t += 1;
    }

cleanup:
    if (inputs != NULL) {
        free_input(inputs);
    }
    interface_free(interface);
    state_free(state);
    return 0;
}
