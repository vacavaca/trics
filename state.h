#ifndef STATE_H
#define STATE_H

#include "pattern.h"
#include "song.h"

#define MAX_PATTERNS 256

typedef struct {
    Song song;
    Pattern patterns[MAX_PATTERNS];
} State;

#endif // STATE_H