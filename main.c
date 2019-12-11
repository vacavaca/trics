#include <stdio.h>
#include "song.h"
#include <signal.h>
#include <ncurses.h>

static WINDOW *win;

void *resizeHandler(int);

void *resizeHandler(int sig)
{
  endwin();
  wclear(win);
  winsertln(win);
  signal(SIGWINCH, resizeHandler);
}

int main()
{
  Song song = song_init(128);

  signal(SIGWINCH, resizeHandler);

  win = initscr();
  cbreak();
  noecho();
  keypad(stdscr, TRUE);
  curs_set(0);
  while (true)
  {
    wmove(win, 1, 1);
    wprintw(win, "*SONG");
    wrefresh(win);
    // wprintw(win, "len: %d\n", song.length);
    // wprintw(win, "len: %d\n", song_set_pattern(&song, 1, 1, 7));
    // wprintw(win, "len: %d\n", song.length);
    // wprintw(win, "1: %d\n", song_get_pattern(&song, 42, 1));
    // wprintw(win, "2: %d\n", song_get_pattern(&song, 0, 1));
    // wprintw(win, "3: %d\n", song_get_pattern(&song, 1, 0));
    // wprintw(win, "4: %d\n", song_get_pattern(&song, 1, 1));
  }

  endwin();
  return 0;
}
