#define MAX_VOICES 8
#define MAX_SONG_LENGTH 512
#define EMPTY 0

void init_song(int bpm);

int get_song_length();

void set_bpm(int bpm);

int get_bpm();

int set_song_pattern(int nbar, int nvoice, int pattern);

int get_song_pattern(int nbar, int nvoice);
