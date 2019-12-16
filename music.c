#include "music.h"

const char WAVETYPE_NOIZE = 1;

const char WAVETYPE_SQUARE = 1 << 1;

const char WAVETYPE_SAW = 1 << 2;

const char WAVETYPE_TRI = 1 << 3;

Instrument *instrument_init(char const *name) {
    Instrument *instrument = malloc(sizeof(Instrument));
    if (instrument == NULL) {
        return NULL;
    }

    *instrument = (Instrument){
        .volume = 256,
        .hard_restart = false,
        .amplitude_attack = 2,
        .amplitude_decay = 0,
        .amplitude_sustain = 256,
        .amplitude_release = 0,
        .wave = (Wave){.repeat = true, .step = 16},
        .filter = (Filter){.repeat = true, .step = 16}};

    int len = strlen(name);

    instrument->name = malloc(len + 1);
    if (instrument->name == NULL) {
        free(instrument);
        return NULL;
    }

    memcpy(instrument->name, name, len + 1);

    instrument_set_wave_step(instrument, (WaveStep){.types = WAVETYPE_SQUARE},
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

void instrument_free(Instrument *instrument) {
    free(instrument->name);
    free(instrument);
}

Arpeggio *arpeggio_init(char const *name) {
    Arpeggio *arpeggio = malloc(sizeof(Arpeggio));
    if (arpeggio == NULL) {
        return NULL;
    }

    *arpeggio = (Arpeggio){.repeat = true, .step = 16};

    int len = strlen(name);
    arpeggio->name = malloc(len + 1);
    if (arpeggio->name == NULL) {
        free(arpeggio);
        return NULL;
    }

    memcpy(arpeggio->name, name, len + 1);

    return arpeggio;
}

void arpeggio_free(Arpeggio *arpeggio) {
    free(*arpeggio->name);
    free(arpeggio);
}

Song *song_init(char const *name) {
    Song *song = malloc(sizeof(Song));
    if (song == NULL) {
        return NULL;
    }

    *song = (Song){
        .bpm = INT_PARAM(128),
        .name = NULL,
        .step = INT_PARAM(16)};

    int len = strlen(name);
    song->name = malloc(len + 1);
    if (song->name == NULL) {
        free(song);
        return NULL;
    }

    memcpy(song->name, name, len + 1);

    return song;
}

int song_set_pattern(Song *song, int nbar, int nvoice, int pattern) {
    song->length = nbar + 1;
    song->patterns[nbar][nvoice] = pattern;
    return song->length;
}

void song_free(Song *song) {
    free(song->name);
    free(song);
}
