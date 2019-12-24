#include "filter.h"


LadderFilter *filter_init(int sample_rate) {
    LadderFilter *filter = malloc(sizeof(LadderFilter));
    if (filter == NULL) {
        return NULL;
    }

    *filter = (LadderFilter){
        .resonance = 0.8,
        .sample_rate = sample_rate
    };

    for (int i = 0; i < 4; i ++) {
        filter->s[i] = 0;
        filter->d[i] = 0;
    }

    filter_set_cutoff(filter, 1000.0);

    return filter;
}

// 0 - 1
void filter_set_resonance(LadderFilter *filter, float r) {
    filter->resonance = 0.8 * r * (filter->t2 + 6.0 * filter->t1) / (filter->t2 - 6.0 * filter->t1);
}

// 0 - 20'000
void filter_set_cutoff(LadderFilter *filter, float f) {
    if (filter->cutoff != f) {
        filter->cutoff = f;
        f = 2.0 * f / filter->sample_rate;
        filter->p = f * (1.8 - 0.8 * f);
        filter->k = 2.0 * sin(f * PI * 0.5) - 1.0;
        filter->t1 = (1.0 - filter->p) * 1.386249;
        filter->t2 = 12.0 + filter->t1 * filter->t1;
    }
}

float filter_process(LadderFilter *filter, float s) {
   float x = s - MIN(1.0, filter->cutoff / 2000) * filter->resonance * filter->s[3];

    filter->s[0] = x * filter->p + filter->d[0]  * filter->p - filter->k * filter->s[0];
    filter->s[1] = filter->s[0] * filter->p + filter->d[1] * filter->p - filter->k * filter->s[1];
    filter->s[2] = filter->s[1] * filter->p + filter->d[2] * filter->p - filter->k * filter->s[2];
    filter->s[3] = filter->s[2] * filter->p + filter->d[3] * filter->p - filter->k * filter->s[3];

    filter->s[3] -= (filter->s[3] * filter->s[3] * filter->s[3]) / 6.0;

    filter->d[0] = x;
    filter->d[1] = filter->s[0];
    filter->d[2] = filter->s[1];
    filter->d[3] = filter->s[2];

    return filter->s[3];
}

void filter_free(LadderFilter *filter) {
    free(filter);
}
