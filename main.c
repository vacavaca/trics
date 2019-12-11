#include "sys.h"
#include "song.h"

int main(char* argv) {
    init_song(120);
    printf("len: %d\n", get_song_length());
    printf("len: %d\n", set_song_pattern(1, 1, 7));
    printf("len: %d\n", get_song_length());
    printf("1: %d\n", get_song_pattern(42, 1));
    printf("2: %d\n", get_song_pattern(0, 1));
    printf("3: %d\n", get_song_pattern(1, 0));
    printf("4: %d\n", get_song_pattern(1, 1));
    return 0;
}
