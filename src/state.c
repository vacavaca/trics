#include "state.h"

const char WAVE_FORM_NOIZE = 1;

const char WAVE_FORM_SQUARE = 1 << 1;

const char WAVE_FORM_SAW = 1 << 2;

const char WAVE_FORM_TRI = 1 << 3;

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
        .attack = 1,
        .decay = 12,
        .sustain = 256,
        .release = 17,
        .wave = (Wave){.repeat = true, .step = 32},
        .filter = (Filter){.repeat = true, .step = 32}};

    int len = strlen(name);

    instrument->name = malloc(len + 1);
    if (instrument->name == NULL) {
        free(instrument);
        return NULL;
    }

    memcpy(instrument->name, name, len + 1);

    WaveStep wave_step = (WaveStep){
        .form = WAVE_FORM_SQUARE,
        .ring_mod_operator = OPERATOR_EQ,
        .ring_mod = 13,
        .ring_mod_amount_operator = OPERATOR_EQ,
        .ring_mod_amount = 1,
        .hard_sync_operator = OPERATOR_EQ,
        .hard_sync = 1,
        .pulse_width_operator = OPERATOR_EQ,
        .pulse_width = 128};

    instrument_set_wave_step(instrument, 0, wave_step);

    /*
     // TODO
    wave_step = (WaveStep){
        .form = WAVE_FORM_SQUARE,
        .ring_mod_operator = OPERATOR_EQ,
        .ring_mod = 13,
        .ring_mod_amount_operator = OPERATOR_EQ,
        .ring_mod_amount = 1,
        .hard_sync_operator = OPERATOR_EQ,
        .hard_sync = 1,
        .pulse_width_operator = OPERATOR_EQ,
        .pulse_width = 128};

    instrument_set_wave_step(instrument, 1, wave_step);
    */

    FilterStep filter_step = (FilterStep){
        .type = FILTER_TYPE_LP,
        .resonance_operator = OPERATOR_EQ,
        .resonance = 1,
        .cutoff_operator = OPERATOR_EQ,
        .cutoff = 256};

    instrument_set_filter_step(instrument, 0, filter_step);

    return instrument;
}

void instrument_set_wave_step(Instrument *instrument, int n, WaveStep step) {
    instrument->wave.steps[n] = step;
    instrument->wave.length = n + 1;
}

void instrument_set_filter_step(Instrument *instrument, int n,
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

    *arpeggio = (Arpeggio){.repeat = true, .step = 32};

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
    if (state_create_instrument(state, "default") == -1) {
        return false;
    }

    state->vars[STATE_VAR_INSTRUMENT] = 1;

    if (state_create_pattern(state) == -1) {
        return false;
    }

    state->vars[STATE_VAR_PATTERN] = 1;
    for (int i = 0; i < MAX_PATTERN_VOICES; i++) {
        state->vars[STATE_VAR_TRANSPOSE + i] = 5;
    }

    if (state_create_arpeggio(state, "default") == -1) {
        return false;
    }

    state->vars[STATE_VAR_ARPEGGIO] = 1;

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
        goto cleanup_arpeggios;
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
    free(state);
cleanup_arpeggios:
    for (int i = 0; i < arpeggios->length; i++) {
        arpeggio_free(ref_list_get(arpeggios, i));
    }
    ref_list_free(arpeggios);
cleanup_patterns:
    ref_list_free(patterns);
cleanup_instruments:
    for (int i = 0; i < instruments->length; i++) {
        instrument_free(ref_list_get(instruments, i));
    }
    ref_list_free(instruments);
cleanup_song:
    song_free(song);
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
