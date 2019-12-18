#include "state.h"

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
        .pan = 128,
        .octave = 4,
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

    WaveStep wave_step = (WaveStep){
        .types = WAVETYPE_SQUARE};

    instrument_set_wave_step(instrument, 0, wave_step);

    return instrument;
}

void instrument_set_wave_step(Instrument *instrument, int n, WaveStep step) {
    instrument->wave.steps[n] = step;
    instrument->wave.length = n + 1;
}

void instrument_set_fitler_step(Instrument *instrument, int n,
                                FilterStep step) {
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

    ArpeggioStep step = (ArpeggioStep){
        .pitch_operator = OPERATOR_ADD,
        .pitch = 0};

    arpeggio_set_step(arpeggio, 0, step);

    return arpeggio;
}

void arpeggio_set_step(Arpeggio *arpeggio, int n, ArpeggioStep step) {
    arpeggio->steps[n] = step;
    arpeggio->length = n + 1;
}

void arpeggio_free(Arpeggio *arpeggio) {
    free(arpeggio->name);
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

bool state_init_defaults(State *state) {
    if (!state_create_instrument(state, "Default")) {
        return false;
    }

    if (!state_create_pattern(state)) {
        return false;
    }

    if (!state_create_arpeggio(state, "Default")) {
        return false;
    }

    return true;
}

State *state_init(char *song_name) {
    Song *song = song_init(song_name);
    if (song == NULL) {
        return NULL;
    }

    RefList *instruments = ref_list_init();
    if (instruments == NULL) {
        goto cleanup_song;
    }

    RefList *patterns = ref_list_init();
    if (patterns == NULL) {
        goto cleanup_instruments;
    }

    RefList *arpeggios = ref_list_init();
    if (arpeggios == NULL) {
        goto cleanup_patterns;
    }

    State *state = malloc(sizeof(State));
    if (state == NULL) {
        goto cleanup;
    }

    *state = (State){
        .song = song,
        .instruments = instruments,
        .patterns = patterns,
        .arpeggios = arpeggios};

    if (!state_init_defaults(state)) {
        goto cleanup;
    }

    return state;

cleanup:
    free(arpeggios);
cleanup_patterns:
    free(patterns);
cleanup_instruments:
    free(instruments);
cleanup_song:
    free(song);
    return NULL;
}

int state_create_instrument(State *state, char const *name) {
    if (state->instruments->length >= MAX_INSTRUMENTS) {
        return -1;
    }

    Instrument *instrument = instrument_init(name);
    if (instrument == NULL) {
        return -1;
    }

    if (!ref_list_add(state->instruments, instrument)) {
        instrument_free(instrument);
        return -1;
    }

    return state->instruments->length - 1;
}

Instrument *state_get_instrument(State *state, int n) {
    return ref_list_get(state->instruments, n);
}

void state_del_instrument(State *state, int n) {
    if (ref_list_has(state->instruments, n)) {
        ref_list_set(state->instruments, n, NULL);
    }
}

int state_create_pattern(State *state) {
    if (state->patterns->length >= MAX_PATTERNS) {
        return -1;
    }

    Pattern *pattern = malloc(sizeof(Pattern));
    *pattern = (Pattern){ .length = state->song->step };

    if (!ref_list_add(state->patterns, pattern)) {
        return -1;
    }

    return state->patterns->length - 1;
}

Pattern *state_get_pattern(State *state, int n) {
    return ref_list_get(state->patterns, n);
}

void state_del_pattern(State *state, int n) {
    if (ref_list_has(state->patterns, n)) {
        ref_list_set(state->patterns, n, NULL);
    }
}

int state_create_arpeggio(State *state, char const *name) {
    if (state->arpeggios->length >= MAX_ARPEGGIOS) {
        return -1;
    }

    Arpeggio *arpeggio = arpeggio_init(name);
    if (arpeggio == NULL) {
        return -1;
    }

    if (!ref_list_add(state->arpeggios, arpeggio)) {
        arpeggio_free(arpeggio);
        return -1;
    }

    return state->arpeggios->length - 1;
}

Arpeggio *state_get_arpeggio(State *state, int n) {
    return ref_list_get(state->arpeggios, n);
}

void state_del_arpeggio(State *state, int n) {
    if (ref_list_has(state->arpeggios, n)) {
        ref_list_set(state->arpeggios, n, NULL);
    }
}

void state_free(State *state) {
    song_free(state->song);

    for (int i = 0; i < state->instruments->length; i++) {
        instrument_free(ref_list_get(state->instruments, i));
    }
    ref_list_free(state->instruments);

    for (int i = 0; i < state->patterns->length; i++) {
        free(ref_list_get(state->patterns, i));
    }
    ref_list_free(state->patterns);

    for (int i = 0; i < state->arpeggios->length; i++) {
        arpeggio_free(ref_list_get(state->arpeggios, i));
    }
    ref_list_free(state->arpeggios);

    free(state);
}
