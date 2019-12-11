#ifndef SONG_H
#define SONG_H

#define MAX_VOICES 8
#define MAX_SONG_LENGTH 512
#define EMPTY 0

typedef struct {
    int length;
    int bpm;
    int bars[MAX_SONG_LENGTH][MAX_VOICES];
} Song;

Song song_init(int bpm);

int song_set_pattern(Song * song, int nbar, int nvoice, int pattern);

int song_get_pattern(Song const * song, int nbar, int nvoice);

#endif // SONG_H
