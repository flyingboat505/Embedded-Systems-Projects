#include "MP3_menu.h"
#include "MP3_song.h"
#include "lcd.h"

static size_t cur_play_song;

void mp3_menu__setup(void) {
  MP3_song__init();
  lcd__init();
  lcd__write_string("|", LINE_1, 13, 0);
  lcd__write_string("|", LINE_2, 13, 0);
  lcd__write_string("- M E N U -", LINE_1, 1, 0);
  lcd__write_string(">", LINE_2, 0, 0);
}