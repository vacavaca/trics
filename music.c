#include "music.h"

const char WAVETYPE_NOIZE = 1;

const char WAVETYPE_SQUARE = 1 << 1;

const char WAVETYPE_SAW = 1 << 2;

const char WAVETYPE_TRI = 1 << 3;

Instrument instrument_init(char const *name) {
    Instrument instrument = {
        .name = name,
        .volume = 256,
        .hard_restart = false,
        .amplitude_attack = 2,
        .amplitude_decay = 0,
        .amplitude_sustain = 256,
        .amplitude_release = 0,
    };

    instrument_set_wave_step(&instrument, (WaveStep){.types = WAVETYPE_SQUARE},
                             0);

    return instrument;
}

void instrument_set_wave_step(Instrument *instrument, WaveStep step, int n) {
    instrument->wave.steps[n] = step;
    instrument->wave.length = n + 1;
}

void instrument_set_fitler_step(Instrument *instrument, FilterStep step,
                                int n) {
    instrument->filter.steps[n] = step;
    instrument->filter.length = n + 1;
}

int song_set_pattern(Song *song, int nbar, int nvoice, int pattern) {
    song->length = nbar + 1;
    song->patterns[nbar][nvoice] = pattern;
    return song->length;
}