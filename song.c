#include "song.h"

Song song_init(int bpm)
{
    Song song = {.length = 0, .bpm = bpm};

    return song;
}

int song_set_pattern(Song *song, int nbar, int nvoice, int pattern)
{
    song->length = nbar + 1;
    song->bars[nbar][nvoice] = pattern;
    return song->length;
}

int song_get_pattern(Song const *song, int nbar, int nvoice)
{
    if (nbar >= song->length)
    {
        return EMPTY;
    }

    return song->bars[nbar][nvoice];
}
