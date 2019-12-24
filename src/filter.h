#ifndef FILTER_H
#define FILTER_H

#include "util.h" // PI
#include <math.h> // sin
#include <stdlib.h>  // malloc

typedef struct {
    float s[4];
    float d[4];
    float p;
    float k;
    float t1;
    float t2;
    float resonance;
    float cutoff;
    int sample_rate;
} LadderFilter;

LadderFilter *filter_init(int sample_rate);

// 0 - 1
void filter_set_resonance(LadderFilter *filter, float r);

// 0 - 20'000
void filter_set_cutoff(LadderFilter *filter, float f);

float filter_process(LadderFilter *filter, float s);

void filter_free(LadderFilter *filter);

#endif // FILTER_H
