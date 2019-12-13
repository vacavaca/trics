#ifndef MUSIC_H
#define MUSIC_H

#include <stdbool.h> // bool

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
#define MAX_TRACKS 8
#define MAX_SONG_LENGTH 512
#define EMPTY 0
#define NONE -1

_Static_assert(sizeof(int) == 4, "Never gonna give you up");

typedef enum {
    OPERATOR_EQ = '=',
    OPERATOR_ADD = '+',
    OPERATOR_SADD = '~',
} Operator;

const char WAVETYPE_NOIZE;
const char WAVETYPE_SQUARE;
const char WAVETYPE_SAW;
const char WAVETYPE_TRI;

typedef struct {
    volatile char types;
    volatile bool ring_mod;
    volatile bool hard_sync;

    Operator pulse_width_operator;
    volatile int pulse_width;
} WaveStep;

typedef struct {
    WaveStep steps[MAX_WAVE_STEPS];
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
    FilterStep steps[MAX_FILTER_STEP];
    volatile bool repeat;
    volatile int step;
    volatile int length;
} Filter;

typedef struct {
    char const *name;
    volatile int volume;
    volatile bool hard_restart;

    volatile int amplitude_attack;
    volatile int amplitude_decay;
    volatile int amplitude_sustain;
    volatile int amplitude_release;

    Wave wave;
    Filter filter;
} Instrument;

Instrument instrument_init(char const *name);

void instrument_set_wave_step(Instrument *instrument, WaveStep step, int n);

void instrument_set_filter_step(Instrument *instrument, FilterStep step, int n);

typedef struct {
    volatile Operator pitch_operator;
    volatile int pitch;
} ArpeggioStep;

typedef struct {
    char const *name;
    ArpeggioStep steps[MAX_ARPEGGIO_STEPS];
    volatile bool repeat;
    volatile int step;
    volatile int length;
} Arpeggio;

typedef struct {
    volatile int instrument;
    volatile int note;
    volatile int arpegio;
} Step;

typedef struct {
    volatile int length;
    Step steps[MAX_PATTERN_LENGTH][MAX_PATTERN_VOICES];
} Pattern;

typedef struct {
    volatile int length;
    volatile int bpm;
    volatile int patterns[MAX_SONG_LENGTH][MAX_TRACKS];
} Song;

int song_set_pattern(Song *song, int nbar, int nvoice, int pattern);

#endif // MUSIC_H
