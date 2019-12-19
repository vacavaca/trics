#ifndef STATE_H
#define STATE_H

#include "reflist.h" // RefList
#include <stdbool.h> // bool
#include <stdlib.h>  // malloc
#include <string.h>  // memcpy

#define MAX_WAVE_STEPS 16
#define MIN_WAVE_STEP 4
#define MAX_WAVE_STEP 16
#define MAX_FILTER_STEPS 16
#define MIN_FILTER_STEP 4
#define MAX_FILTER_STEP 16
#define MAX_ARPEGGIO_STEPS 64
#define MIN_ARPEGGIO_STEP 1
#define MAX_ARPEGGIO_STEP 4
#define MAX_PATTERN_LENGTH 64
#define MAX_PATTERN_VOICES 2
#define MAX_PATTERNS 256
#define MAX_INSTRUMENTS 256
#define MAX_ARPEGGIOS 256
#define MAX_TRACKS 8
#define MAX_SONG_LENGTH 256
#define EMPTY 0
#define NONE -1

#define INT_PARAM(n) (n + 1)

_Static_assert(sizeof(int) == 4, "Never gonna give you up");

typedef enum {
    OPERATOR_EQ = '=',
    OPERATOR_ADD = '+',
    OPERATOR_QADD = '~',
} Operator;

const char WAVETYPE_NOIZE;
const char WAVETYPE_SQUARE;
const char WAVETYPE_SAW;
const char WAVETYPE_TRI;

typedef struct {
    volatile char types;
    volatile bool ring_mod;
    volatile bool hard_sync;

    volatile Operator pulse_width_operator;
    volatile int pulse_width;
} WaveStep;

typedef struct {
    volatile WaveStep steps[MAX_WAVE_STEPS];
    volatile bool repeat;
    volatile int step;
    volatile int length;
} Wave;

typedef enum {
    FILTERTYPE_LP,
    FILTERTYPE_BP,
    FILTERTYPE_HP,
} FilterType;

typedef struct {
    FilterType type;
    volatile Operator resonance_operator;
    volatile int resonance;
    volatile Operator cutoff_operator;
    volatile int cutoff;
} FilterStep;

typedef struct {
    volatile FilterStep steps[MAX_FILTER_STEP];
    volatile bool repeat;
    volatile int step;
    volatile int length;
} Filter;

typedef struct {
    char *name;
    volatile int volume;
    volatile int pan;
    volatile int octave;
    volatile bool hard_restart;

    volatile int amplitude_attack;
    volatile int amplitude_decay;
    volatile int amplitude_sustain;
    volatile int amplitude_release;

    Wave wave;
    Filter filter;
} Instrument;

Instrument *instrument_init(char const *name);

void instrument_set_wave_step(Instrument *instrument, int n, WaveStep step);

void instrument_set_filter_step(Instrument *instrument, int n, FilterStep step);

void instrument_free(Instrument *instrument);

typedef struct {
    volatile Operator pitch_operator;
    volatile int pitch;
} ArpeggioStep;

typedef struct {
    char *name;
    volatile ArpeggioStep steps[MAX_ARPEGGIO_STEPS];
    volatile bool repeat;
    volatile int step;
    volatile int length;
} Arpeggio;

Arpeggio *arpeggio_init(char const *name);

void arpeggio_set_step(Arpeggio *arpeggio, int n, ArpeggioStep step);

void arpeggio_free(Arpeggio *arpeggio);

typedef struct {
    volatile int instrument;
    volatile int note;
    volatile int arpeggio;
} Step;

typedef struct {
    volatile int length;
    volatile Step steps[MAX_PATTERN_LENGTH][MAX_PATTERN_VOICES];
} Pattern;

typedef struct {
    char *name;
    volatile int length;
    volatile int bpm;
    volatile int step;
    volatile int patterns[MAX_SONG_LENGTH][MAX_TRACKS];
} Song;

Song *song_init(char const *name);

int song_set_pattern(Song *song, int nbar, int nvoice, int pattern);

void song_free(Song *song);

typedef struct {
    Song *song;
    RefList *patterns;
    RefList *instruments;
    RefList *arpeggios;
} State;

State *state_init(char *song_name);

int state_create_instrument(State *state, char const *name);

Instrument *state_get_instrument(State *state, int n);

void state_del_instrument(State *state, int n);

int state_create_pattern(State *state);

Pattern *state_get_pattern(State *state, int n);

void state_del_pattern(State *state, int n);

int state_create_arpeggio(State *state, char const *name);

Arpeggio *state_get_arpeggio(State *state, int n);

void state_del_arpeggio(State *state, int n);

void state_free(State *state);

#endif // STATE_H
