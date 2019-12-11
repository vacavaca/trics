#include "sys.h"
#include "song.h"

const int empty_bar[MAX_VOICES] = {};

struct {
    int length;
    int bpm;
    int** bars;
} song = { 0, 120, (int*[MAX_SONG_LENGTH]) {} };

void init_song(int bpm) {
    set_bpm(bpm);
    size_t bar_size = sizeof(int) * MAX_VOICES;

    for (int i = 0; i < MAX_SONG_LENGTH; i ++) {
        int *new_bar = (int*) malloc(bar_size); // no need to free it
        memcpy(new_bar, empty_bar, bar_size);

        song.bars[i] = new_bar;
    }
}

int get_song_length() {
    return song.length;
}

void set_bpm(int bpm) {
    song.bpm = bpm;
}

int get_bpm() {
    return song.bpm;
}

int set_song_pattern(int nbar, int nvoice, int pattern) {
    song.length = nbar + 1;
    song.bars[nbar][nvoice] = pattern;
    return song.length;
}

int get_song_pattern(int nbar, int nvoice) {
    if (nbar >= song.length)
        return EMPTY;

    return song.bars[nbar][nvoice];
}
