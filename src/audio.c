#include "audio.h"

// Envelope

const float envelope_factor = 1.0 + (1.0 - (1.0 / ENVELOPE_CURVE));

EnvelopeGen envelope_gen_init(float attack, float decay,
                              float sustain, float release) {
    return (EnvelopeGen){
        .attack = attack,
        .decay = decay,
        .sustain = sustain,
        .release = release,
        .state = ENVELOPE_IDLE,
        .start = 0,
        .last = 0,
        .value = 0,
        .released = false};
}

EnvelopeGen envelope_gen_for_instrument(Instrument *instrument) {
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

    return envelope_gen_init(attack, decay, sustain, release);
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

// Playinh note

PlayingNote *playing_note_init(State *state, int instrument, int track,
                               int arpeggio, int note, bool song_note,
                               float time, bool trigger, int ndx) {
    PlayingNote *playing_note = malloc(sizeof(PlayingNote));
    if (playing_note == NULL) {
        return NULL;
    }

    Instrument *instrument_ref = ref_list_get(state->instruments, instrument);

    *playing_note = (PlayingNote){
        .instrument = instrument,
        .track = track,
        .arpeggio = arpeggio,
        .note = note,
        .song_note = song_note,
        .time = time,
        .state = trigger ? NOTE_STATE_TRIGGER : NOTE_STATE_RELEASE,
        .envelope = NULL,
        .frame = NULL,
        .instrument_ref = instrument_ref,
        .ndx = ndx};

    return playing_note;
}

void playing_note_free(PlayingNote *note) {
    if (note->envelope != NULL) {
        free(note->envelope);
    }

    if (note->frame != NULL) {
        free(note->frame);
    }

    free(note);
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
    RefList *buffer = ref_list_init();
    if (buffer == NULL) {
        return NULL;
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
        .buffer = buffer,
        .queue = queue,
        .spec = spec,
        .sample_pos = 0,
        .time = 0,
        .start_bar = 0,
        .start_time = 0,
        .playing = false,
        .update_buffer_at = -1,
        .update_frames_at = -1,
        .update_request_mutex = PTHREAD_MUTEX_INITIALIZER,
        .queue_mutex = PTHREAD_MUTEX_INITIALIZER,
        .frames_update_count = 0,
        .buffer_update_count = 0,
        .note_ndx = 0};

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
    return NULL;
}

inline static void audio_context_request_buffer_update(AudioContext *ctx, int at) {
    if (at == ctx->update_buffer_at) {
        return;
    }

    pthread_mutex_lock(&ctx->update_request_mutex);
    if (ctx->update_buffer_at != -1) {
        ctx->update_buffer_at = MIN(ctx->update_buffer_at, at);
    } else {
        ctx->update_buffer_at = at;
    }
    pthread_mutex_unlock(&ctx->update_request_mutex);
}

inline static void audio_context_request_frames_update(AudioContext *ctx, int at) {
    if (at == ctx->update_frames_at) {
        return;
    }

    pthread_mutex_lock(&ctx->update_request_mutex);
    if (ctx->update_frames_at != -1) {
        ctx->update_frames_at = MIN(ctx->update_frames_at, at);
    } else {
        ctx->update_frames_at = at;
    }
    pthread_mutex_unlock(&ctx->update_request_mutex);
}

inline static bool audio_context_need_buffer_update(AudioContext *ctx) {
    int at = ctx->update_buffer_at;
    return at != -1 && ctx->sample_pos >= at;
}

inline static bool audio_context_need_frames_update(AudioContext *ctx) {
    int at = ctx->update_frames_at;
    return at != -1 && ctx->sample_pos >= at;
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

    audio_context_request_buffer_update(ctx, 0);

    audio_context_fill_queue(ctx);
    SDL_PauseAudio(false);
}

int track = 0;

bool audio_context_trigger_step(AudioContext *ctx, int instrument,
                                int arpeggio, int note, int step,
                                int step_div) {
    if (!ctx->playing) {
        return false;
    }

    int track = MAX_TRACKS * MAX_PATTERN_VOICES; // last solo track

    arpeggio = arpeggio == EMPTY ? EMPTY : arpeggio - 1;

    PlayingNote *trigger = playing_note_init(ctx->state, instrument - 1, track, arpeggio, note - 1,
                                             false, ctx->time, true, ++ctx->note_ndx);
    if (trigger == NULL) {
        return false;
    }

    float whole = 4.0 * 60.0 / ((float)(ctx->state->song->bpm - 1));
    float end = ctx->time + whole * step / step_div;
    PlayingNote *release = playing_note_init(ctx->state, instrument - 1, track, arpeggio, note - 1,
                                             false, end, false, ++ctx->note_ndx);
    if (release == NULL) {
        goto cleanup_trigger;
    }

    pthread_mutex_lock(&ctx->queue_mutex);

    if (!ref_list_add(ctx->queue, trigger)) {
        goto cleanup;
    }

    // TODO bin search
    int i = ctx->queue->length - 2;
    for (;i >= 0; i--) {
        PlayingNote *ex = ref_list_get(ctx->queue, i);
        if (ex->time > release->time) {
            break;
        }
    }

    if (!ref_list_insert(ctx->queue, i + 1, release)) {
        goto cleanup;
    }

    // update immediatelly
    audio_context_request_buffer_update(ctx, ctx->sample_pos);

    pthread_mutex_unlock(&ctx->queue_mutex);

    return true;

cleanup:
    pthread_mutex_unlock(&ctx->queue_mutex);
    playing_note_free(release);
cleanup_trigger:
    playing_note_free(trigger);
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

    // never update
    audio_context_request_buffer_update(ctx, -1);
    audio_context_request_frames_update(ctx, -1);

    for (int i = 0; i < ctx->buffer->length; i++) {
        PlayingNote *note = ref_list_get(ctx->buffer, i);
        if (note->song_note) {
            playing_note_free(note);
            ref_list_del(ctx->buffer, i);
            i -= 1;
        }
    }

    pthread_mutex_lock(&ctx->queue_mutex);

    for (int i = 0; i < ctx->queue->length; i++) {
        PlayingNote *note = ref_list_get(ctx->queue, i);
        if (note->song_note) {
            playing_note_free(note);
            ref_list_del(ctx->queue, i);
            i -= 1;
        }
    }

    pthread_mutex_unlock(&ctx->queue_mutex);
}

void audio_context_free(AudioContext *ctx) {
    SDL_CloseAudio();
    for (int i = 0; i < ctx->buffer->length; i++) {
        PlayingNote *n = ref_list_get(ctx->buffer, i);
        if (n != NULL) {
            playing_note_free(n);
        }
    }
    ref_list_free(ctx->buffer);

    for (int i = 0; i < ctx->queue->length; i++) {
        PlayingNote *n = ref_list_get(ctx->queue, i);
        if (n != NULL) {
            playing_note_free(n);
        }
    }
    ref_list_free(ctx->queue);

    free(ctx);
}

void audio_context_offset_time(AudioContext *ctx, float offset) {
    if (ctx->time - offset <= 0) {
        return;
    }

    ctx->start_time -= offset;
    ctx->time -= offset;

    for(int i = 0; i < ctx->buffer->length; i ++) {
        PlayingNote *note = ref_list_get(ctx->buffer, i);
        note->time -= offset;
        note->envelope->start -= offset;
        note->envelope->last -= offset;
    }

    pthread_mutex_lock(&ctx->queue_mutex);

    for(int i = 0; i < ctx->queue->length; i ++) {
        PlayingNote *note = ref_list_get(ctx->queue, i);
        note->time -= offset;
    }

    pthread_mutex_unlock(&ctx->queue_mutex);
}

void audio_context_release_same_track_note(AudioContext *ctx, PlayingNote *note) {
    int tn = note->track;
    int in = note->instrument;

    for (int i = 0; i < ctx->buffer->length; i ++) {
        PlayingNote *ex = ref_list_get(ctx->buffer, i);
        if (ex->track == tn && ex->instrument == in) {
            playing_note_free(ex);
            ref_list_del(ctx->buffer, i);
            break;
        }
    }
}

bool audio_context_fill_buffer(AudioContext *ctx) {
    ctx->buffer_update_count += 1;
    audio_context_request_buffer_update(ctx, -1);
    pthread_mutex_lock(&ctx->queue_mutex);
    if (ctx->queue->length > 0) {
        PlayingNote *last = ref_list_get(ctx->queue, ctx->queue->length - 1);
        if (last->time > ctx->time) {
            int dst = ceil((last->time - ctx->time) * SAMPLE_RATE + 2);
            audio_context_request_buffer_update(ctx, ctx->sample_pos + dst);
            pthread_mutex_unlock(&ctx->queue_mutex);
            return false;
        }
    }


    bool updated = false;

    while(ctx->queue->length > 0) {
        PlayingNote *note = ref_list_pop(ctx->queue);
        if (note->time <= ctx->time) {
            if (note->state == NOTE_STATE_TRIGGER) {
                audio_context_release_same_track_note(ctx, note);

                EnvelopeGen *envelope = malloc(sizeof(EnvelopeGen));
                if (envelope == NULL) {
                    pthread_mutex_unlock(&ctx->queue_mutex);
                    return false;
                }

                Instrument *instrument = ref_list_get(ctx->state->instruments, note->instrument);
               *envelope = envelope_gen_for_instrument(instrument);

                note->envelope = envelope;
                note->state = NOTE_STATE_PLAY;
                ref_list_add(ctx->buffer, note);

                envelope_gen_trigger(envelope, note->time);

                updated = true;
            } else if (note->state == NOTE_STATE_RELEASE) {
                for (int i = 0; i < ctx->buffer->length; i ++) {
                    PlayingNote *ex = ref_list_get(ctx->buffer, i);
                    if (ex->track == note->track && ex->instrument == note->instrument) {
                        envelope_gen_release(ex->envelope, note->time);
                    }
                }
                updated = true;
            }
        } else {
            ref_list_add(ctx->queue, note);
            int dst = ceil((note->time - ctx->time) * SAMPLE_RATE + 2);
            audio_context_request_buffer_update(ctx, ctx->sample_pos + dst);
            break;
        }
    }

    pthread_mutex_unlock(&ctx->queue_mutex);
    return updated;
}

inline static float calc_op(Operator op, float prev, float value) {
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
                                          note->instrument);
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
    ring_mod = CLAMP(ring_mod, MIN_RING_MOD, MAX_RING_MOD) - 1;

    float prev_ring_mod_amount = previous != NULL
                             ? previous->ring_mod_amount
                             : INITIAL_RING_MOD_AMOUNT;
    float ring_mod_amount = calc_op(step->ring_mod_amount_operator,
                                prev_ring_mod_amount,
                                step->ring_mod_amount);
    ring_mod_amount = NORM(ring_mod_amount, MIN_RING_MOD_AMOUNT, MAX_RING_MOD_AMOUNT);

    float prev_hard_sync = previous != NULL
                             ? previous->hard_sync
                             : INITIAL_HARD_SYNC;
    float hard_sync = calc_op(step->hard_sync_operator,
                                prev_hard_sync,
                                step->hard_sync);
    hard_sync = CLAMP(hard_sync, MIN_HARD_SYNC, MAX_HARD_SYNC) - 1;

    float prev_pulse_width = previous != NULL
                             ? previous->pulse_width
                             : INITIAL_PULSE_WIDTH;
    float pulse_width = calc_op(step->pulse_width_operator,
                                prev_pulse_width,
                                step->pulse_width);
    pulse_width = NORM(pulse_width, MIN_PULSE_WIDTH, MAX_PULSE_WIDTH);

    float note_duration = ctx->time - note->time;
    int note_sample_duration = SAMPLE_RATE * note_duration;
    float duration = (float)SAMPLE_RATE * 240.0 / ((wave->step - 1) * (song->bpm - 1));
    int frames_past = floor(note_sample_duration / duration);

//    int part_frame_passed = floor(note_duration * (wave->step - 1) *
                              //    (song->bpm - 1) / 240.0);
    int pos = ctx->sample_pos - note_sample_duration + frames_past * duration;

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
                                          note->instrument);
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
    resonance = NORM(resonance, MIN_RESONANCE, MAX_RESONANCE);

    float prev_cutoff = previous != NULL
                             ? previous->cutoff
                             : INITIAL_CUTOFF;
    float cutoff = calc_op(step->cutoff_operator,
                                prev_cutoff,
                                step->cutoff);
    cutoff = NORM(cutoff, MIN_CUTOFF, MAX_CUTOFF);

    float note_duration = ctx->time - note->time;
    int note_sample_duration = SAMPLE_RATE * note_duration;
    float duration = (float)SAMPLE_RATE * 240.0 / ((filter->step - 1) *
                   (song->bpm - 1));
    int frames_past = floor(note_sample_duration / duration);
    int pos = ctx->sample_pos - note_sample_duration + frames_past * duration;

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
                                      note->arpeggio);
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
    pitch = CLAMP(pitch, MIN_PITCH, MAX_PITCH) - 1;

    float note_duration = ctx->time - note->time;
    int note_sample_duration = SAMPLE_RATE * note_duration;
    float duration = (float)SAMPLE_RATE * 240.0 / ((arpeggio->step - 1) *
                                          (song->bpm - 1));
    int frames_past = floor(note_sample_duration / duration);
    int pos = ctx->sample_pos - note_sample_duration + frames_past * duration;

    return (ArpeggioFrame){
        .sample_pos = pos,
        .duration = duration,
        .step_n = step_n,
        .note = pitch};
}

Frame *audio_context_calculate_frame(AudioContext *ctx, PlayingNote *note) {
    Frame *previous = note->frame;

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

    Frame *frame = malloc(sizeof(Frame));
    if (frame == NULL) {
        return NULL;
    }

    *frame = (Frame){
        .wave = wave,
        .filter = filter,
        .play_arpeggio = note->arpeggio != EMPTY
    };

    if (!frame->play_arpeggio) {
        frame->note = note->note;
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

    frame->arpeggio = arpeggio;
    return frame;
}

inline static float get_min_frame_end(Frame *frame) {
    WaveFrame *wave = &frame->wave;
    FilterFrame *filter = &frame->filter;
    ArpeggioFrame *arp = &frame->arpeggio;
    int wave_end = wave->sample_pos + wave->duration;
    int filter_end = filter->sample_pos + filter->duration;
    if (frame->play_arpeggio) {
        int arp_end = arp->sample_pos + arp->duration;
        return MIN(wave_end, MIN(filter_end, arp_end));
    } else {
        return MIN(wave_end, filter_end);
    }
}

bool audio_context_fill_frames(AudioContext *ctx) {
            //    printf("check %d\n", ctx->sample_pos);
    ctx->frames_update_count += 1;
    audio_context_request_frames_update(ctx, -1);
    bool updated = false;
    for (int i = 0; i < ctx->buffer->length; i ++) {
        PlayingNote *note = ref_list_get(ctx->buffer, i);
        Frame *prev = note->frame;
        if (prev != NULL) {
            int end = get_min_frame_end(prev);
            if (end > ctx->sample_pos) {
                int end = get_min_frame_end(prev);
            //    printf("requet %d\n", end + 2);
                audio_context_request_frames_update(ctx, end + 2);
                continue; // no need for update
            }
        }

        updated = true;

        Frame *frame = audio_context_calculate_frame(ctx, note);
        if (frame == NULL) {
            return false;
        }

        int end = get_min_frame_end(frame);
//                printf("requet 2 %d %d\n", end + 2, frame->filter.duration);
        audio_context_request_frames_update(ctx, end + 2);

        if (prev != NULL) {
            free(prev);
        }
        note->frame = frame;
    }

    return updated;
}

bool audio_context_update_play_buffers(AudioContext *ctx) {
    bool update_buffer = audio_context_need_buffer_update(ctx);
    bool buffer_updated = false;
    if (update_buffer) {
        buffer_updated = audio_context_fill_buffer(ctx);
    }

    bool update_frames = audio_context_need_frames_update(ctx);

    // fill frames if buffer update or if frames requested update
    if (buffer_updated || update_frames) {
        if (!audio_context_fill_frames(ctx)) {
            return false;
        }
    }

    return true;
}



void audio_context_release_note(AudioContext *ctx, PlayingNote *note) {
    for (int i = 0; i < ctx->buffer->length; i ++) {
        PlayingNote *ex = ref_list_get(ctx->buffer, i);
        if (ex == note) {
            playing_note_free(ex);
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
    float ref_note = 58; // A4 440
    float ref_freq = 440;
    float dn = note - ref_note;
    return ref_freq * pow(tett, dn);
}

inline static Output single_voice(AudioContext *ctx, PlayingNote *note, int nv,
                                  float detune, float separation, float offset) {
    Instrument *instrument = note->instrument_ref;;
    EnvelopeGen *envelope = note->envelope;
    Frame *frame = note->frame;

    // parameters
    float pitch = frame->play_arpeggio ? frame->arpeggio.note : frame->note;
    float freq = note_freq(pitch);
    float vol = NORM((float)instrument->volume, MIN_PARAM, MAX_PARAM);
    float nvl = nv * 2;
    float nvr = nv * 2 + 1;
    float pan = NORM((float)instrument->pan, MIN_PARAM, MAX_PARAM);

    // phases
    float cycle_len_l = (float)SAMPLE_RATE / (freq - detune);
    float cycle_len_r = (float)SAMPLE_RATE / (freq + detune);
    float tl = (float)ctx->sample_pos / cycle_len_l + separation + offset;
    float tr = (float)ctx->sample_pos / cycle_len_r - separation + offset;
    float xl = tl - floor(tl);
    float xr = tr - floor(tr);

    // TODO hard sync

    // wave
    float yl = wave(ctx, note->track, nvl, frame->wave.form,
                    frame->wave.pulse_width, xl);
    float yr = wave(ctx, note->track, nvr, frame->wave.form,
                    frame->wave.pulse_width, xr);

    // TODO ring mod

    // envelope
    float e = envelope_gen_calculate(envelope, ctx->time);

    yl *= vol * e;
    yr *= vol * e;

    float pd = abs(pan - 0.5);
    yl *= (1 - pan) * (-pd + 1) * 2;
    yr *= pan * (-pd + 1) * 2;

    // output

    // TODO filter

    return (Output){ .left = yl, .right = yr };
}

inline static Output instrument_voice(AudioContext *ctx, PlayingNote *note) {

    Output first = single_voice(ctx, note, 0, 0,
                                WIDENING_OFFSET / 2, -0.224 + (float)(note->ndx % 23) / 23);
    Output second = single_voice(ctx, note, 1, WIDENING_DETUNE,
                                 WIDENING_OFFSET, 0.0 + (float)(note->ndx % 23) / 23);

    EnvelopeGen *envelope = note->envelope;
    if (envelope->state == ENVELOPE_IDLE) {
        audio_context_release_note(ctx, note);
    }

    // TODO FX

    return (Output) {
        .left = (first.left + second.left) / 2.0,
        .right = (first.right + second.right) / 2.0};
}

void typed_audio_callback(AudioContext *ctx, short* stream, int len) {

    float dt = 1.0 / SAMPLE_RATE;
//;    int start = SDL_GetTicks();
    for (int i = 0; i < len / 2; i ++) {
        ctx->time += dt;
        ctx->sample_pos += 1;

        audio_context_update_play_buffers(ctx);

        Output mix = (Output){ .left = 0, .right = 0 };
        float fmix = MAX(1.0, (float)ctx->buffer->length / 1.0);
        for (int j = 0; j < ctx->buffer->length; j ++) {
            PlayingNote *note = ref_list_get(ctx->buffer, j);
            Output output = instrument_voice(ctx, note);
            mix.left += output.left / fmix;
            mix.right += output.right / fmix;
        }

        stream[i * 2] = mix.left;
        stream[i * 2 + 1] = mix.right;
    }
    /*
    int elapsed = SDL_GetTicks() - start;
    float total = ((float)len / (float)SAMPLE_RATE);
    float load = (float)elapsed / 1000.0 / total;
    printf("%d %d %f of %f\n", len, elapsed, load, total); 
    */

    if (ctx->time > 6) {
        audio_context_offset_time(ctx, 6);
    }
}
