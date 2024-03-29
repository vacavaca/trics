#ifndef AUDIO_H
#define AUDIO_H

#include "state.h" // State
#include "reflist.h" // RefList
#include "util.h" // MAX, MIN
#include "filter.h" // LadderFilter
#include <SDL2/SDL.h>
#include <math.h> // floor
#include <string.h> // memcpy
#include <stdbool.h> // bool
#include <pthread.h> // pthread_mutex_

#define SAMPLE_BUFFER 1024
#define SAMPLE_RATE 44100
#define ENVELOPE_CURVE 3
#define ENVELOPE_MIN_ATTACK 0.002
#define ENVELOPE_MAX_ATTACK 8
#define ENVELOPE_MIN_DECAY 0.006
#define ENVELOPE_MAX_DECAY 12
#define ENVELOPE_MIN_RELEASE 0.006
#define ENVELOPE_MAX_RELEASE 24
#define MAX_VALUE 32767
#define WIDENING_DETUNE 0.24
#define WIDENING_OFFSET -0.4
#define WIDENING_OSCILLATORS 4
#define TETT 1.0594630943592953  // 2 ^ (1 / 12)

typedef enum {
    ENVELOPE_IDLE = 0,
    ENVELOPE_ATTACK,
    ENVELOPE_DECAY,
    ENVELOPE_SUSTAIN,
    ENVELOPE_RELEASE,
} EnvelopeState;

typedef struct {
    float attack;
    float decay;
    float sustain;
    float release;
    EnvelopeState state;
    float start;
    float last;
    float value;
    bool released;
} EnvelopeGen;

typedef enum {
    NOTE_STATE_TRIGGER,
    NOTE_STATE_PLAY,
    NOTE_STATE_RELEASE,
} NoteState;

typedef struct {
    int sample_pos;
    int duration;

    int step_n;
    char form;
    float ring_mod; // 1 - 256
    float ring_mod_amount; // 1 - 256
    float hard_sync; // 1 - 256
    float pulse_width; // 1 - 256
} WaveFrame;

typedef struct {
    int sample_pos;
    int duration;

    int step_n;
    float resonance;
    float cutoff;
} FilterFrame;

typedef struct {
    int sample_pos;
    int duration;

    int step_n;
    float note;
} ArpeggioFrame;

typedef struct {
    WaveFrame wave;
    FilterFrame filter;
    bool play_arpeggio;
    union {
        ArpeggioFrame arpeggio;
        int note;
    };
} Frame;

typedef struct {
    int instrument;
    int track;
    int arpeggio;
    int note;
    int ndx;
    bool song_note;
    float time;
    NoteState state;
    EnvelopeGen *envelope;
    Frame *frame;
    Instrument *instrument_ref;
    Arpeggio *arpeggio_ref;
    float random;
    int sample_pos;
    LadderFilter *filters[WIDENING_OSCILLATORS];
} PlayingNote;

// queue  - queue with future note trigger and release events
//          updates on song start, and on keyboard presses
//          updates are pushed from play function
//
// buffer - play buffer, contains notes which are currently playing
//          updates when notes are pressed or playing in the pattern
//          (up to ~ 34 Gz)
//          updates are pulled from the audio_callback
//          clears after notes envelopes go idle
//
// ui/song -(note|event)-> queue -> buffer => ~~~-> samples
typedef struct {
    State *state;
    RefList *buffer;
    RefList *queue;
    float noize_values
        [MAX_TRACKS * MAX_PATTERN_VOICES + 1] // 8 * 2 + solo
        [WIDENING_OSCILLATORS * 2];
    SDL_AudioSpec spec;
    int sample_pos;
    float time;
    int start_bar;
    float start_time;
    volatile bool playing;
    volatile int update_buffer_at;
    volatile int update_frames_at;
    pthread_mutex_t update_request_mutex;
    pthread_mutex_t queue_mutex;
    volatile int frames_update_count;
    volatile int buffer_update_count;
    int note_ndx;
} AudioContext;

AudioContext *audio_context_init(State *state);

void audio_context_play(AudioContext *ctx, int start_bar);

bool audio_context_trigger_step(AudioContext *ctx, int instrument, int arpeggio,
                           int note, int step, int step_div);

void audio_context_stop(AudioContext *ctx);

void audio_context_free(AudioContext *ctx);

#endif // AUDIO_H
