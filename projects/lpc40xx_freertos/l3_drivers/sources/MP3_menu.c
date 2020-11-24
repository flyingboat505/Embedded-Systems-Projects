#include "MP3_menu.h"
#include "FreeRTOS.h"
#include "MP3_song.h"
#include "delay.h"
#include "lcd.h"
#include "queue.h"
#include <string.h>

static UI_Page Page_Display;
;

static size_t cur_play_song = -1;

extern pause;

static void MP3_menu__main_menu_display_top_right(uint8_t INDEX) {
  string16_t display;
  sprintf(display, "%i", INDEX + 1);
  if (INDEX < 10)
    lcd__write_string(display, LINE_1, 15, 0);
  else
    lcd__write_string(display, LINE_1, 16, 0);
}

static void MP3_menu__main_menu_display_top_left(string16_t display) { lcd__write_string(display, LINE_1, 1, 0); }

static void MP3_menu__main_menu_display_bottom_left(string16_t display) { lcd__write_string(display, LINE_2, 1, 0); }

static void MP3_menu__refresh_main_menu(void) {
  MP3_song__set_index(0);
  string16_t song_display = {0};
  lcd__write_string("|", LINE_1, 13, 0);
  MP3_menu__main_menu_display_top_right(MP3_song__get_index());
  lcd__write_string("            ", LINE_2, 1, 0);

  strncpy(song_display, MP3_song__search_by_index(0).file_name, 12);
  MP3_menu__main_menu_display_bottom_left(song_display);

  lcd__write_string("|", LINE_2, 13, 0);
  lcd__write_string("- M E N U - ", LINE_1, 1, 0);
  lcd__write_string(">", LINE_2, 0, 0);
}

static void MP3_menu__rebooting_mp3(void) {
  lcd__write_string("REBOOTING", LINE_1, 3, 100);
  lcd__write_string("MP3 PLAYER...", LINE_2, 1, 100);
  delay__ms(1000);
}

void MP3_menu__init(void) {
  MP3_song__init();
  lcd__init();
  MP3_menu__rebooting_mp3();
  MP3_menu__refresh_main_menu();
}

void MP3_menu__main_menu_scroll_up(void) {
  uint8_t INDEX = MP3_song__get_index();
  string16_t TOP_display = {0};
  string16_t BOTTOM_display = {0};
  if (!INDEX) {
    INDEX = MP3_song__get_size() - 1;
  } else if (INDEX == 1) {
    MP3_menu__refresh_main_menu();
    return;
  } else {
    INDEX--;
  }
  strncpy(BOTTOM_display, MP3_song__search_by_index(INDEX).file_name, 12);
  MP3_menu__main_menu_display_bottom_left(BOTTOM_display);

  strncpy(TOP_display, MP3_song__search_by_index(INDEX - 1).file_name, 12);
  MP3_menu__main_menu_display_top_left(TOP_display);

  MP3_song__set_index(INDEX);
  MP3_menu__main_menu_display_top_right(INDEX);
}

void MP3_menu__main_menu_scroll_down(void) {
  uint8_t INDEX = MP3_song__get_index();
  string16_t TOP_display = {0};
  string16_t BOTTOM_display = {0};
  if (INDEX == MP3_song__get_size() - 1) {
    MP3_menu__refresh_main_menu();
    return;
  } else {
    INDEX++;
  }
  strncpy(BOTTOM_display, MP3_song__search_by_index(INDEX).file_name, 12);
  MP3_menu__main_menu_display_bottom_left(BOTTOM_display);

  strncpy(TOP_display, MP3_song__search_by_index(INDEX - 1).file_name, 12);
  MP3_menu__main_menu_display_top_left(TOP_display);

  MP3_song__set_index(INDEX);
  MP3_menu__main_menu_display_top_right(INDEX);
}

extern QueueHandle_t song_name_queue;

void MP3_menu__song_handler(void) {
  uint8_t INDEX = MP3_song__get_index();
  if (cur_play_song == -1 || cur_play_song != INDEX) {
    pause = 0;
    cur_play_song = INDEX;
    if (xQueueSend(song_name_queue, MP3_song__search_by_index(cur_play_song).file_name, 0)) {
    } else {
    }
  } else if (pause && INDEX == cur_play_song) {
    pause = 0;
  } else {
    pause = 1;
  }
}

uint8_t MP3_menu__get_cur_song_index(void) { return cur_play_song; };