#include "audio.h"

// Envelope

const float envelope_factor = 1.0 + (1.0 - (1.0 / ENVELOPE_CURVE));

EnvelopeGen envelope_gen_init(float attack, float decay,
                              float sustain, float release,
                              int track, int instrument) {
    return (EnvelopeGen){
        .attack = attack,
        .decay = decay,
        .sustain = sustain,
        .release = release,
        .state = ENVELOPE_IDLE,
        .start = 0,
        .last = 0,
        .value = 0,
        .track = track,
        .instrument = instrument,
        .released = false};
}

EnvelopeGen envelope_gen_for_instrument(Instrument *instrument,
                                        int tn, int in) {
    const float attack = ((float) instrument->attack - 1) / 255 *
                         (ENVELOPE_MAX_ATTACK - ENVELOPE_MIN_ATTACK) +
                         ENVELOPE_MIN_ATTACK;
    const float decay = ((float) instrument->decay - 1) / 255 *
                        (ENVELOPE_MAX_DECAY - ENVELOPE_MIN_DECAY) +
                        ENVELOPE_MIN_DECAY;

    const float sustain = ((float) instrument->sustain - 1) / 255;

    const float release = ((float) instrument->release - 1) / 255 *
                          (ENVELOPE_MAX_RELEASE - ENVELOPE_MIN_RELEASE) +
                          ENVELOPE_MIN_RELEASE;

    return envelope_gen_init(attack, decay, sustain, release, tn, in);
}

void envelope_gen_trigger(EnvelopeGen *gen, float time) {
    if (gen->attack > 0) {
        gen->state = ENVELOPE_ATTACK;
        gen->value = 0.0;
    } else if (gen->decay > 0) {
        gen->state = ENVELOPE_DECAY;
        gen->value = 1.0;
    } else {
        gen->state = ENVELOPE_SUSTAIN;
        gen->value = gen->sustain;
    }

    gen->last = time;
    gen->start = time;
}

float envelope_gen_calculate(EnvelopeGen *gen, float time) {
    if (gen->state == ENVELOPE_IDLE) {
        return 0;
    }

    const float delta = time - gen->start;
    if (gen->state == ENVELOPE_ATTACK && delta >= gen->attack) {
        if (gen->decay > 0) {
            gen->state = ENVELOPE_DECAY;
            gen->start = time;
            gen->last = time;
        } else {
            gen->value = gen->sustain;
            gen->state = ENVELOPE_SUSTAIN;
            gen->start = time;
            gen->last = time;
        }
    } else if (gen->state == ENVELOPE_DECAY && delta >= gen->decay) {
        if (!gen->released) {
            gen->state = ENVELOPE_SUSTAIN;
        } else if (gen->value > 0) {
            gen->state = ENVELOPE_RELEASE;
        } else {
            gen->state = ENVELOPE_IDLE;
        }

        gen->start = time;
        gen->last = time;
    } else if (gen->state == ENVELOPE_RELEASE && delta >= gen->release) {
        gen->state = ENVELOPE_IDLE;
        gen->start = time;
        gen->last = time;
    }

    if (gen->state == ENVELOPE_IDLE) {
        gen->value = 0.0;
        gen->last = time;
        return 0;
    }

    if (gen->state == ENVELOPE_SUSTAIN) {
        gen->last = time;
        return gen->value;
    }

    if (time - gen->last == 0) {
        return gen->value;
    }

    float interval;
    float target;
    if (gen->state == ENVELOPE_ATTACK) {
        interval = gen->attack;
        target = 1.0;
    } else if (gen->state == ENVELOPE_DECAY) {
        interval = gen->decay;
        target = gen->sustain;
    } else if (gen->state == ENVELOPE_RELEASE) {
        interval = gen->release;
        target = 0.0;
    }

    if (gen->value == target) {
        gen->last = time;
        return gen->value;
    }

    const float ry = target - gen->value;
    const float rt = (gen->start + interval - time) / interval;
    const float dt = (time - gen->last) / interval;

    if (rt <= 0.0 || dt * envelope_factor >= rt) {
        gen->value = target;
        gen->last = time;
        return gen->value;
    }

    float prev = gen->value;
    gen->value += (ry < 0 ? ENVELOPE_CURVE : (1.0 / ENVELOPE_CURVE)) * ry / rt * dt;
    gen->last = time;
    return gen->value;
}

void envelope_gen_release(EnvelopeGen *gen, float time) {
    if (gen->release > 0 && gen->value > 0) {
        if (gen->state == ENVELOPE_ATTACK || gen->state == ENVELOPE_SUSTAIN ||
            (gen->state == ENVELOPE_DECAY && time - gen->start > gen->decay)) {
            gen->state = ENVELOPE_RELEASE;
        }
    } else {
        gen->state = ENVELOPE_IDLE;
        gen->value = 0;
    }

    gen->last = time;
    gen->start = time;
    gen->released = true;
}

void envelope_gen_reset(EnvelopeGen *gen) {
    gen->last = 0;
    gen->start = 0;
    gen->value = 0;
    gen->released = false;
    gen->state = ENVELOPE_IDLE;
}

// AudioContext

void typed_audio_callback(AudioContext *ctx, short* stream, int len);

// thanks robn/tinysid for that
unsigned int frand_seed;
inline static float frand(void) {
    frand_seed = frand_seed * 1103515245 + 12345;
    return (float)(frand_seed >> 16);
}

void audio_callback(void *ctx, Uint8* stream, int bytes) {
    typed_audio_callback((AudioContext *)ctx, (short *)stream, bytes / 2);
}

AudioContext *audio_context_init(State *state) {
    RefList *envelopes = ref_list_init();
    if (envelopes == NULL) {
        return NULL;
    }

    RefList *frames = ref_list_init();
    if (frames == NULL) {
        goto cleanup_envelopes;
    }

    RefList *buffer = ref_list_init();
    if (buffer == NULL) {
        goto cleanup_frames;
    }

    RefList *queue = ref_list_init();
    if (queue == NULL) {
        goto cleanup_buffer;
    }

    AudioContext *ctx = malloc(sizeof(AudioContext));
    if (ctx == NULL) {
        goto cleanup_queue;
    }

    SDL_AudioSpec spec = (SDL_AudioSpec){
        .freq = SAMPLE_RATE,
        .format = AUDIO_S16,
        .channels = 2,
        .samples = SAMPLE_BUFFER,
        .callback = audio_callback,
        .userdata = ctx
    };

    *ctx = (AudioContext){
        .state = state,
        .envelopes = envelopes,
        .frames = frames,
        .buffer = buffer,
        .queue = queue,
        .spec = spec,
        .sample_pos = 0,
        .time = 0,
        .start_bar = 0,
        .start_time = 0,
        .playing = false,
        .update_buffer_at = 0,
        .update_frames_at = 0,
        .queue_mutex = PTHREAD_MUTEX_INITIALIZER};

    for (int i = 0; i < MAX_TRACKS * MAX_PATTERN_VOICES + 1; i ++) {
        for (int j = 0; j < WIDENING_OSCILLATORS; j++) {
            ctx->noize_values[i][j * 2] = frand();
            ctx->noize_values[i][j * 2 + 1] = 1.0;
        }
    }

    if (SDL_OpenAudio(&ctx->spec, NULL) < 0) {
        goto cleanup;
    }

    return ctx;

cleanup:
    free(ctx);
cleanup_queue:
    ref_list_free(queue);
cleanup_buffer:
    ref_list_free(buffer);
cleanup_frames:
    ref_list_free(frames);
cleanup_envelopes:
    ref_list_free(envelopes);
    return NULL;
}


bool audio_context_fill_queue(AudioContext *ctx) {
    // TODO
    return false;
}

void audio_context_play(AudioContext *ctx, int start_bar) {
    if (ctx->playing) {
        return;
    }

    ctx->sample_pos = 0;
    ctx->start_bar = start_bar;
    ctx->start_time = ctx->time;
    ctx->playing = true;

    // request to update buffers immediately
    ctx->update_buffer_at = 0;
    ctx->update_frames_at = 0;

    audio_context_fill_queue(ctx);
    SDL_PauseAudio(false);
}

bool audio_context_trigger_step(AudioContext *ctx, int instrument,
                                int arpeggio, int note, int step,
                                int step_div) {
    if (!ctx->playing) {
        return false;
    }

    PlayingNote *trigger = malloc(sizeof(PlayingNote));
    if (trigger == NULL) {
        return false;
    }

    int track = MAX_TRACKS * MAX_PATTERN_VOICES; // last solo track

    *trigger = (PlayingNote){
        .instrument = instrument,
        .arpeggio = arpeggio,
        .note = note,
        .track = track,
        .time = ctx->time,
        .state = NOTE_STATE_TRIGGER};

    PlayingNote *release = malloc(sizeof(PlayingNote));
    if (release == NULL) {
        goto cleanup_trigger;
    }

    float whole = 4.0 * 60.0 / ((float)(ctx->state->song->bpm - 1));
    float end = whole * step / step_div;
    *release = (PlayingNote){
        .instrument = instrument,
        .arpeggio = arpeggio,
        .note = note,
        .track = track,
        .song_note = false,
        .time = ctx->time + end,
        .state = NOTE_STATE_RELEASE};

    pthread_mutex_lock(&ctx->queue_mutex);

    if (!ref_list_add(ctx->queue, trigger)) {
        goto cleanup;
    }

    int i = ctx->queue->length - 2;

    // TODO bin search
    for (;i >= 0; i--) {
        PlayingNote *ex = ref_list_get(ctx->queue, i);
        if (ex->time > release->time) {
            break;
        }
    }

    if (!ref_list_insert(ctx->queue, i + 1, release)) {
        goto cleanup;
    }

    pthread_mutex_unlock(&ctx->queue_mutex);

    ctx->update_buffer_at = 0; // update immediatelly

    return true;

cleanup:
    pthread_mutex_unlock(&ctx->queue_mutex);
    free(release);
cleanup_trigger:
    free(trigger);
    return false;
}

void audio_context_stop(AudioContext *ctx) {
    if (!ctx->playing) {
        return;
    }

    // TODO pause only in audio callback to avoid buffering after pause
    SDL_PauseAudio(true);
    ctx->playing = false;
    ctx->sample_pos = 0;
    ctx->start_time = 0;

    for (int i = 0; i < ctx->buffer->length; i++) {
        PlayingNote *note = ref_list_get(ctx->buffer, i);
        if (note->song_note) {
            for (int j = 0; j < ctx->envelopes->length; j ++) {
                EnvelopeGen *gen = ref_list_get(ctx->envelopes, j);
                if (gen->track == note->track &&
                    gen->instrument == note->instrument) {
                    free(gen);
                    ref_list_del(ctx->envelopes, j);
                    break;
                }
            }

            for (int j = 0; j < ctx->frames->length; j ++) {
                Frame *frame = ref_list_get(ctx->frames, j);
                if (frame->track == note->track &&
                    frame->instrument == note->instrument) {
                    free(frame);
                    ref_list_del(ctx->frames, j);
                    break;
                }
            }

            free(note);
            ref_list_del(ctx->buffer, i);
        }
    }

    pthread_mutex_lock(&ctx->queue_mutex);
    for (int i = 0; i < ctx->queue->length; i++) {
        PlayingNote *note = ref_list_get(ctx->queue, i);
        if (note->song_note) {
            free(note);
            ref_list_del(ctx->queue, i);
        }
    }
    pthread_mutex_unlock(&ctx->queue_mutex);
}

void audio_context_free(AudioContext *ctx) {
    SDL_CloseAudio();
    for (int i = 0; i < ctx->envelopes->length; i++) {
        EnvelopeGen *e = ref_list_get(ctx->envelopes, i);
        if (e != NULL) {
            free(e);
        }
    }
    ref_list_free(ctx->envelopes);

    for (int i = 0; i < ctx->frames->length; i++) {
        Frame *f = ref_list_get(ctx->frames, i);
        if (f != NULL) {
            free(f);
        }
    }
    ref_list_free(ctx->frames);

    for (int i = 0; i < ctx->buffer->length; i++) {
        PlayingNote *n = ref_list_get(ctx->buffer, i);
        if (n != NULL) {
            free(n);
        }
    }
    ref_list_free(ctx->buffer);

    for (int i = 0; i < ctx->queue->length; i++) {
        PlayingNote *n = ref_list_get(ctx->queue, i);
        if (n != NULL) {
            free(n);
        }
    }
    ref_list_free(ctx->queue);

    free(ctx);
}

EnvelopeGen *audio_context_get_envelope(AudioContext *ctx, int tn, int in) {
    // TODO optimize me
    for (int i = 0; i < ctx->envelopes->length; i --) {
        EnvelopeGen *e = ref_list_get(ctx->envelopes, i);
        if (e->track == tn && e->instrument == in) {
            return e;
        }
    }

    Instrument *instrument = ref_list_get(ctx->state->instruments, in - 1);
    EnvelopeGen *e = malloc(sizeof(EnvelopeGen));
    if (e == NULL) {
        return NULL;
    }
    *e = envelope_gen_for_instrument(instrument, tn, in);
    if (!ref_list_add(ctx->envelopes, e)) {
        free(e);
        return NULL;
    }

    return e;
}

void audio_context_offset_time(AudioContext *ctx, float offset) {
    if (ctx->time - offset <= 0) {
        return;
    }

    ctx->start_time -= offset;
    ctx->time -= offset;
    for(int i = 0; i < ctx->envelopes->length; i ++) {
        EnvelopeGen *gen = ref_list_get(ctx->envelopes, i);
        gen->start -= offset;
        gen->last -= offset;
    }

    for(int i = 0; i < ctx->buffer->length; i ++) {
        PlayingNote *note = ref_list_get(ctx->buffer, i);
        note->time -= offset;
    }

    pthread_mutex_lock(&ctx->queue_mutex);
    for(int i = 0; i < ctx->queue->length; i ++) {
        PlayingNote *note = ref_list_get(ctx->queue, i);
        note->time -= offset;
    }
    pthread_mutex_unlock(&ctx->queue_mutex);
}

bool audio_context_fill_buffer(AudioContext *ctx) {
    pthread_mutex_lock(&ctx->queue_mutex);
    if (ctx->queue->length > 0) {
        PlayingNote *last = ref_list_get(ctx->queue, ctx->queue->length - 1);
        if (last->time > ctx->time) {
            int dst = round((last->time - ctx->time) * SAMPLE_RATE);
            ctx->update_buffer_at = ctx->sample_pos + dst - 1;
            pthread_mutex_unlock(&ctx->queue_mutex);
            return false;
        }
    }

    bool updated = false;
    while(ctx->queue->length > 0) {
        PlayingNote *note = ref_list_pop(ctx->queue);
        EnvelopeGen *gen = audio_context_get_envelope(ctx, note->track,
                                                      note->instrument);
        if (note->time <= ctx->time) {
            if (note->state == NOTE_STATE_TRIGGER) {
                audio_context_release_note_buffer(ctx, note->track, note->instrument, false);
                note->state = NOTE_STATE_PLAY;
                ref_list_add(ctx->buffer, note);
                envelope_gen_reset(gen);
                envelope_gen_trigger(gen, note->time);
                updated = true;
            } else if (note->state == NOTE_STATE_RELEASE) {
                envelope_gen_release(gen, note->time);
                updated = true;
            }
        } else {
            ref_list_add(ctx->queue, note);
            int dst = round((note->time - ctx->time) * SAMPLE_RATE);
            ctx->update_buffer_at = ctx->sample_pos + dst - 1;
            break;
        }
    }

    pthread_mutex_unlock(&ctx->queue_mutex);
    return updated;
}

float calc_op(Operator op, float prev, float value) {
    if (op == OPERATOR_EQ) {
        return value;
    } else if (op == OPERATOR_ADD) {
        return prev + value;
    } else if (op == OPERATOR_QADD) {
        return prev + value / 4.;
    } else if (op == OPERATOR_SUB) {
        return prev - value;
    } else if (op == OPERATOR_QSUB) {
        return prev - value / 4.;
    }
    return value;
}

WaveFrame audio_context_calculate_wave_frame(AudioContext *ctx,
                                             PlayingNote *note,
                                             WaveFrame *previous) {
    Song *song = ctx->state->song;
    Instrument *instrument = ref_list_get(ctx->state->instruments,
                                          note->instrument - 1);
    Wave *wave = &instrument->wave;
    int step_n = previous != NULL ? previous->step_n : 0;
    if (step_n >= wave->length) {
        if (wave->repeat) {
            step_n %= wave->length;
        } else {
            step_n = wave->length - 1;
        }
    }

    volatile WaveStep *step = &wave->steps[step_n];

    float prev_ring_mod = previous != NULL
                             ? previous->ring_mod
                             : INITIAL_RING_MOD;
    float ring_mod = calc_op(step->ring_mod_operator,
                                prev_ring_mod,
                                step->ring_mod);
    ring_mod = CLAMP(ring_mod, 1, 256);

    float prev_ring_mod_amount = previous != NULL
                             ? previous->ring_mod_amount
                             : INITIAL_RING_MOD_AMOUNT;
    float ring_mod_amount = calc_op(step->ring_mod_amount_operator,
                                prev_ring_mod_amount,
                                step->ring_mod_amount);
    ring_mod_amount = CLAMP(ring_mod_amount, 1, 256);

    float prev_hard_sync = previous != NULL
                             ? previous->hard_sync
                             : INITIAL_HARD_SYNC;
    float hard_sync = calc_op(step->hard_sync_operator,
                                prev_hard_sync,
                                step->hard_sync);
    hard_sync = CLAMP(hard_sync, 1, 256);

    float prev_pulse_width = previous != NULL
                             ? previous->pulse_width
                             : INITIAL_PW;
    float pulse_width = calc_op(step->pulse_width_operator,
                                prev_pulse_width,
                                step->pulse_width);
    pulse_width = CLAMP(pulse_width, 1, 256);

    float note_duration = ctx->time - note->time;
    int duration = SAMPLE_RATE * 240.0 / ((wave->step - 1) * (song->bpm - 1));
    int psn = floor(note_duration * (wave->step - 1) *
                   (song->bpm - 1) / 240.0);
    int pos = duration * psn;

    return (WaveFrame){
        .sample_pos = pos,
        .duration = duration,
        .step_n = step_n,
        .form = step->form,
        .ring_mod = ring_mod,
        .ring_mod_amount = ring_mod_amount,
        .hard_sync = hard_sync,
        .pulse_width = pulse_width};
}

FilterFrame audio_context_calculate_filter_frame(AudioContext *ctx,
                                                 PlayingNote *note,
                                                 FilterFrame *previous) {
    Song *song = ctx->state->song;
    Instrument *instrument = ref_list_get(ctx->state->instruments,
                                          note->instrument - 1);
    Filter *filter = &instrument->filter;
    int step_n = previous != NULL ? previous->step_n : 0;
    if (step_n >= filter->length) {
        if (filter->repeat) {
            step_n %= filter->length;
        } else {
            step_n = filter->length - 1;
        }
    }

    volatile FilterStep *step = &filter->steps[step_n];

    float prev_resonance = previous != NULL
                             ? previous->resonance
                             : INITIAL_RESONANCE;
    float resonance = calc_op(step->resonance_operator,
                                prev_resonance,
                                step->resonance);
    resonance = CLAMP(resonance, 1, 256);

    float prev_cutoff = previous != NULL
                             ? previous->cutoff
                             : INITIAL_CUTOFF;
    float cutoff = calc_op(step->cutoff_operator,
                                prev_cutoff,
                                step->cutoff);
    cutoff = CLAMP(cutoff, 1, 256);

    float note_duration = ctx->time - note->time;
    int duration = SAMPLE_RATE * 240.0 / ((filter->step - 1) *
                   (song->bpm - 1));
    int psn = floor(note_duration * (filter->step - 1) *
                    (song->bpm - 1) / 240.0);
    int pos = duration * psn;

    return (FilterFrame){
        .sample_pos = pos,
        .duration = duration,
        .step_n = step_n,
        .type = step->type,
        .resonance = resonance,
        .cutoff = cutoff};
}

ArpeggioFrame audio_context_calculate_arpeggio_frame(AudioContext *ctx,
                                                     PlayingNote *note,
                                                     ArpeggioFrame *previous) {
    Song *song = ctx->state->song;
    Arpeggio *arpeggio = ref_list_get(ctx->state->arpeggios,
                                      note->arpeggio - 1);
    int step_n = previous != NULL ? previous->step_n : 0;
    if (step_n >= arpeggio->length) {
        if (arpeggio->repeat) {
            step_n %= arpeggio->length;
        } else {
            step_n = arpeggio->length - 1;
        }
    }

    volatile ArpeggioStep *step = &arpeggio->steps[step_n];

    float prev_pitch = previous != NULL
                             ? previous->note
                             : note->note;
    float pitch = calc_op(step->pitch_operator,
                                prev_pitch,
                                step->pitch);
    pitch = CLAMP(pitch, 1, 109);

    float note_duration = ctx->time - note->time;
    int duration = SAMPLE_RATE * 240.0 / ((arpeggio->step - 1) *
                                          (song->bpm - 1));
    int psn = floor(note_duration * (arpeggio->step - 1) *
                    (song->bpm - 1) / 240.0);
    int pos = duration * psn;

    return (ArpeggioFrame){
        .sample_pos = pos,
        .duration = duration,
        .step_n = step_n,
        .note = pitch};
}

Frame audio_context_calculate_frame(AudioContext *ctx, PlayingNote *note,
                                    Frame *previous) {
    WaveFrame wave;
    if (previous != NULL) {
        WaveFrame *prev = &previous->wave;
        if (prev->sample_pos + prev->duration > ctx->sample_pos) {
            wave = *prev;
        } else {
            wave = audio_context_calculate_wave_frame(ctx, note, prev);
        }
    } else {
        wave = audio_context_calculate_wave_frame(ctx, note, NULL);
    }

    FilterFrame filter;
    if (previous != NULL) {
        FilterFrame *prev = &previous->filter;
        if (prev->sample_pos + prev->duration > ctx->sample_pos) {
            filter = *prev;
        } else {
            filter = audio_context_calculate_filter_frame(ctx, note, prev);
        }
    } else {
        filter = audio_context_calculate_filter_frame(ctx, note, NULL);
    }

    Frame frame = (Frame){
        .track = note->track,
        .instrument = note->instrument,
        .prev = previous,
        .wave = wave,
        .filter = filter,
        .play_arpeggio = note->arpeggio != EMPTY
    };

    if (!frame.play_arpeggio) {
        frame.note = note->note;
        return frame;
    }

    ArpeggioFrame arpeggio;
    if (previous != NULL) {
        ArpeggioFrame *prev = &previous->arpeggio;
        if (prev->sample_pos + prev->duration > ctx->sample_pos) {
            arpeggio = *prev;
        } else {
            arpeggio = audio_context_calculate_arpeggio_frame(ctx, note, prev);
        }
    } else {
        arpeggio = audio_context_calculate_arpeggio_frame(ctx, note, NULL);
    }

    frame.arpeggio = arpeggio;
    return frame;
}

float get_min_frame_end(Frame *frame) {
    WaveFrame *wave = &frame->wave;
    FilterFrame *filter = &frame->filter;
    ArpeggioFrame *arp = &frame->arpeggio;
    int wave_end = wave->sample_pos + wave->duration;
    int filter_end = filter->sample_pos + filter->duration;
    int arp_end = arp->sample_pos + filter->duration;
    return MIN(wave_end, MIN(filter_end, arp_end));
}

bool audio_context_fill_frames(AudioContext *ctx) {
    int update_at = ctx->update_buffer_at;
    bool updated = false;
    for (int i = 0; i < ctx->buffer->length; i ++) {
        PlayingNote *note = ref_list_get(ctx->buffer, i);
        int prev_frame_ndx = -1;
        Frame *prev = NULL;
        for (int j = 0; j < ctx->frames->length; j ++) {
            Frame *frame = ref_list_get(ctx->frames, j);
            if (frame->track == note->track &&
                frame->instrument == note->instrument) {
                prev = frame;
                prev_frame_ndx = j;
                break;
            }
        }

        if (prev != NULL) {
            int end = get_min_frame_end(prev);
            if (end > ctx->sample_pos) {
                continue; // no need for update
            }
        }

        updated = true;

        Frame *frame = malloc(sizeof(Frame));
        if (frame == NULL) {
            return false;
        }

        *frame = audio_context_calculate_frame(ctx, note, prev);
        update_at = MIN(update_at, get_min_frame_end(frame));

        if (prev != NULL) {
            free(prev);
            ref_list_set(ctx->frames, prev_frame_ndx, frame);
        } else {
            ref_list_add(ctx->frames, frame);
        }
    }

    ctx->update_frames_at = update_at;
    return updated;
}

bool audio_context_update_play_buffers(AudioContext *ctx) {
    bool update_buffer = ctx->sample_pos >= ctx->update_buffer_at;
    if (update_buffer && !audio_context_fill_buffer(ctx)) {
        return false;
    }

    // fill frames if buffer update or if frames requested update
    if ((update_buffer || ctx->sample_pos >= ctx->update_frames_at) &&
        !audio_context_fill_frames(ctx)) {
        return false;
    }

    return true;
}


void audio_context_release_note_buffer(AudioContext *ctx, int tn, int in, bool del_envelope) {
    if (del_envelope) {
        for (int i = 0; i < ctx->envelopes->length; i ++) {
            EnvelopeGen *gen = ref_list_get(ctx->envelopes, i);
            if (gen->track == tn && gen->instrument == in) {
                free(gen);
                ref_list_del(ctx->envelopes, i);
                break;
            }
        }
    }

    for (int i = 0; i < ctx->frames->length; i ++) {
        Frame *frame = ref_list_get(ctx->frames, i);
        if (frame->track == tn && frame->instrument == in) {
            free(frame);
            ref_list_del(ctx->frames, i);
            break;
        }
    }

    for (int i = 0; i < ctx->buffer->length; i ++) {
        PlayingNote *note = ref_list_get(ctx->buffer, i);
        if (note->track == tn && note->instrument == in) {
            free(note);
            ref_list_del(ctx->buffer, i);
            break;
        }
    }
}

// Sound engine

inline static float noize_wave(AudioContext *ctx, int ndx, int osc, float x) {
    float xf = x * 2;
    xf = xf - round(xf);
    if (xf < ctx->noize_values[ndx][osc * 2 + 1]) {
        ctx->noize_values[ndx][osc * 2] = frand();
    }
    ctx->noize_values[ndx][osc * 2 + 1] = xf;
    return ctx->noize_values[ndx][osc * 2];
}

inline static float saw_wave(float x) {
    return (float) MAX_VALUE - 2.0 * x * MAX_VALUE;
}

inline static float square_wave(float pw, float x) {
    const float o = 0.205026489;
    float pws = o;
    if (pws > 1.0) {
        pws -= 1;
    }
    float pwe = o + pw;
    if (pwe > 1.0) {
        pwe -= 1;
    }
    if (pwe < pws) {
        pwe += pws;
        pws = pwe - pws;
        pwe = pwe - pws;
    }
    return x >= pws && x < pwe ? 0.0 : MAX_VALUE;
}

inline static float tri_wave(float x) {
    if (x < 0.25) {
        return 0 + (x / 0.25) * MAX_VALUE;
    } else if (x > 0.75) {
        return -MAX_VALUE + (x - 0.75) / 0.25 * MAX_VALUE;
    } else {
        return MAX_VALUE - 2 * (x - 0.25) / 0.5 * MAX_VALUE;
    }
}

inline static float and_wave(float a, float b) {
    return (float)((short)round(a) & (short)round(b));
}

inline static float wave(AudioContext *ctx, int ndx, int osc,
                         char form, float pw, float x) {
    float result = 0.0;
    if ((form & WAVE_FORM_NOIZE) == WAVE_FORM_NOIZE) {
        if (result == 0.0) {
            result += noize_wave(ctx, ndx, osc, x);
        } else {
            result = and_wave(result, noize_wave(ctx, ndx, osc, x));
        }
    }

    if ((form & WAVE_FORM_SQUARE) == WAVE_FORM_SQUARE) {
        if (result == 0.0) {
            result += square_wave(pw, x);
        } else {
            result = and_wave(result, square_wave(pw, x));
        }
    }

    if ((form & WAVE_FORM_SAW) == WAVE_FORM_SAW) {
        if (result == 0.0) {
            result += saw_wave(x);
        } else {
            result = and_wave(result, saw_wave(x));
        }
    }

    if ((form & WAVE_FORM_TRI) == WAVE_FORM_TRI) {
        if (result == 0.0) {
            result += tri_wave(x);
        } else {
            result = and_wave(result, tri_wave(x));
        }
    }

    return result;
}

float const tett = 1.0594630943592953; // 2 ^ (1/12)

typedef struct {
    float left;
    float right;
} Output;

inline static float note_freq(float note) {
    float ref_note = 59; // A4 440
    float ref_freq = 440;
    float dn = note - ref_note;
    return ref_freq * pow(tett, dn);
}

inline static Output single_voice(AudioContext *ctx, Frame *frame, int nv,
                                  float detune, float separation, float offset) {
    int tn = frame->track;
    int in = frame->instrument;

    Instrument *instrument = ref_list_get(ctx->state->instruments, in - 1);
    EnvelopeGen *envelope = audio_context_get_envelope(ctx, tn, in);

    // parameters
    float note = frame->play_arpeggio ? frame->arpeggio.note : frame->note;
    float freq = note_freq(note);
    float vol = (float)instrument->volume / 256;
    float nvl = nv * 2;
    float nvr = nv * 2 + 1;
    float pan = ((float)instrument->pan - 128) / 256;

    // phases
    float cycle_len_l = (float)SAMPLE_RATE / (freq - detune);
    float cycle_len_r = (float)SAMPLE_RATE / (freq + detune);
    float tl = (float)ctx->sample_pos / cycle_len_l + separation + offset;
    float tr = (float)ctx->sample_pos / cycle_len_r - separation + offset;
    float xl = tl - floor(tl);
    float xr = tr - floor(tr);

    // wave
    float yl = wave(ctx, frame->track, nvl, frame->wave.form,
                    (frame->wave.pulse_width - 1) / 255, xl);
    float yr = wave(ctx, frame->track, nvr, frame->wave.form,
                    (frame->wave.pulse_width - 1) / 255, xr);

    // envelope
    float e = envelope_gen_calculate(envelope, ctx->time);

    yl *= vol * e;
    yr *= vol * e;

    yl *= (-pan + 1.0) / 2;
    yr *= (pan + 1.0) / 2;

    // output

    // TODO hard sync
    // TODO ring mod
    // TODO filter

    return (Output){ .left = yl, .right = yr };
}

inline static Output instrument_voice(AudioContext *ctx, Frame *frame) {

    Output first = single_voice(ctx, frame, 0, 0,
                                WIDENING_OFFSET / 2, -0.224);
    Output second = single_voice(ctx, frame, 1, WIDENING_DETUNE,
                                 WIDENING_OFFSET, 0.0);

    int tn = frame->track;
    int in = frame->instrument;
    EnvelopeGen *envelope = audio_context_get_envelope(ctx, tn, in);

    if (envelope->state == ENVELOPE_IDLE) {
        audio_context_release_note_buffer(ctx, tn, in, true);
    }

    // TODO FX

    return (Output) {
        .left = (first.left + second.left) / 2.0,
        .right = (first.right + second.right) / 2.0};
}

void typed_audio_callback(AudioContext *ctx, short* stream, int len) {

    float dt = 1.0 / SAMPLE_RATE;
    for (int i = 0; i < len / 2; i ++) {
        ctx->time += dt;
        ctx->sample_pos += 1;

        audio_context_update_play_buffers(ctx);

        Output mix = (Output){ .left = 0, .right = 0 };
        for (int j = 0; j < ctx->frames->length; j ++) {
           Frame *frame = ref_list_get(ctx->frames, j);
           Output output = instrument_voice(ctx, frame);
           mix.left += output.left / ctx->frames->length;
           mix.right += output.right / ctx->frames->length;
        }

        stream[i * 2] = mix.left;
        stream[i * 2 + 1] = mix.right;
    }

    if (ctx->time > 6) {
        audio_context_offset_time(ctx, 6);
    }
}
