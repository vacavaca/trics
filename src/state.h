#ifndef STATE_H
#define STATE_H

#include "reflist.h" // RefList
#include <stdbool.h> // bool
#include <stdlib.h>  // malloc
#include <string.h>  // memcpy

#define MAX_WAVE_STEPS 16
#define MIN_WAVE_STEP 16
#define MAX_WAVE_STEP 128
#define MAX_FILTER_STEPS 16
#define MIN_FILTER_STEP 16
#define MAX_FILTER_STEP 128
#define MAX_ARPEGGIO_STEPS 16
#define MIN_ARPEGGIO_STEP 16
#define MAX_ARPEGGIO_STEP 128
#define MAX_PATTERN_VOICES 2
#define MAX_PATTERNS 256
#define MAX_INSTRUMENTS 256
#define MAX_ARPEGGIOS 256
#define MIN_PATTERN_STEP 4
#define MAX_PATTERN_STEP 32
#define MAX_TRACKS 8
#define MAX_SONG_LENGTH 256
#define EMPTY 0
#define NONE -1
#define STATE_VARS_COUNT 8
#define STATE_VAR_PATTERN 0
#define STATE_VAR_INSTRUMENT 1
#define STATE_VAR_ARPEGGIO 2
#define STATE_VAR_TRANSPOSE 3
#define INITIAL_PW 128
#define INITIAL_RING_MOD 13
#define INITIAL_RING_MOD_AMOUNT 1
#define INITIAL_HARD_SYNC 2
#define INITIAL_RESONANCE 1
#define INITIAL_CUTOFF 256

#define INT_PARAM(n) (n + 1)

_Static_assert(sizeof(int) == 4, "Never gonna give you up");

typedef enum {
    OPERATOR_EQ = '=',
    OPERATOR_ADD = '+', // +semi = +1
    OPERATOR_QADD = '<', // +semi/4 = +0.25
    OPERATOR_SUB = '-', // -semi = -1
    OPERATOR_QSUB = '>', // -semi/4 = -0.25
} Operator;

const char WAVE_FORM_NOIZE;
const char WAVE_FORM_SQUARE;
const char WAVE_FORM_SAW;
const char WAVE_FORM_TRI;

typedef struct {
    volatile char form;

    volatile Operator ring_mod_operator;
    volatile int ring_mod;

    volatile Operator ring_mod_amount_operator;
    volatile int ring_mod_amount;

    volatile Operator hard_sync_operator;
    volatile int hard_sync;

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
    FILTER_TYPE_LP,
    FILTER_TYPE_BP,
    FILTER_TYPE_HP,
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

    volatile int attack;
    volatile int decay;
    volatile int sustain;
    volatile int release;

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
    volatile Step steps[MAX_PATTERN_STEP][MAX_PATTERN_VOICES];
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
    int vars[STATE_VARS_COUNT];
} State;

State *state_init(char *song_name);

int state_create_instrument(State *state, char const *name);

int state_create_pattern(State *state);

int state_create_arpeggio(State *state, char const *name);

void state_free(State *state);

#endif // STATE_H
