#include "MP3_menu.h"
#include "FreeRTOS.h"
#include "MP3_song.h"
#include "delay.h"
#include "lcd.h"
#include "queue.h"
#include <string.h>

static UI_Page Page_Display;

static size_t cur_play_song = -1;

extern uint8_t pause;

//==================================================================
//               T A B L E   O F    C O N T E N T
//==================================================================
// Note: Unused Functions. Use for redirecting under this source file
// Right click function name and go to definition;
static void Redirect_to_reboot_code();
static void Redirect_to_login_code();

//==================================================================

//==================================================================
//                      G E N E R I C
//==================================================================
#define SCROLL_UP '^'
#define SCROLL_DOWN 'V'
#define ENTER 'E'

//==================================================================
//                         R E B O O T
//==================================================================
static void Redirect_to_reboot_code() {}
//-----------------------------------

static void MP3_menu__rebooting_mp3(void) {
  lcd__write_string("REBOOTING", LINE_1, 3, 100);
  lcd__write_string("MP3 PLAYER...", LINE_2, 1, 100);
  delay__ms(1000);
}

//==================================================================
//                          L O G I N
//==================================================================
static void Redirect_to_login_code() {}
//-----------------------------------

#define LOGIN_number_of_digits 4
#define LOGIN_clear_key 'B'

#define LOGIN_passcode 1234

static uint8_t pin_count = 0;
static uint16_t entered_pin = 0;
static LOGIN_stats LOG_stats = IN_PROCESS;

static void MP3_menu__LOGIN_lock_menu_page(void) {
  vTaskSuspend(xTaskGetHandle("reader"));
  vTaskSuspend(xTaskGetHandle("player"));
  vTaskSuspend(xTaskGetHandle("volume"));
}
static void MP3_menu__LOGIN_unlock_menu_page(void) {
  vTaskResume(xTaskGetHandle("reader"));
  vTaskResume(xTaskGetHandle("player"));
  vTaskResume(xTaskGetHandle("volume"));
}

static void MP3_menu__LOGIN_setup_passcode_UI(void) {
  lcd__write_string(">             ", LINE_2, 0, 0);
  Page_Display = login;
}

static void MP3_menu__LOGIN_refresh_passcode(void) {
  LOG_stats = IN_PROCESS;
  pin_count = 0;
  entered_pin = 0;
  lcd__write_string("           ", LINE_2, 1, 0);
}

static void MP3_menu__LOGIN_handle_incorrect_passcode(void) {
  LOG_stats = INCORRECT;
  delay__ms(500);
  lcd__write_string("INCORRECT", LINE_2, 1, 0);
}

static void MP3_menu__MAIN_MENU_refresh(void); // Need to declare this function here
static void MP3_menu__LOGIN_handle_correct_passcode(void) {
  LOG_stats = SUCCESS;
  delay__ms(500);
  lcd__write_string("               ", LINE_1, 1, 0);
  lcd__write_string("               ", LINE_2, 0, 0);
  lcd__write_string("WELCOME!", LINE_1, 4, 100);
  lcd__write_string("      ...    ", LINE_2, 0, 100);
  delay__ms(1000);
  Page_Display = main_menu;
  MP3_menu__MAIN_MENU_refresh();
  MP3_menu__LOGIN_unlock_menu_page();
}

static void MP3_menu__LOGIN_handler(const char key) {
  if (key >= '0' && key <= '9') {
    if (MP3_menu__LOGIN_get_LOGIN_status() == INCORRECT) {
      MP3_menu__LOGIN_refresh_passcode();
    }
    uint8_t num = key - 0x30;
    entered_pin = entered_pin * 10 + num;

    delay__ms(200);

    lcd__write_string("*", LINE_2, 2 + pin_count, 0);

    if (++pin_count >= LOGIN_number_of_digits) {
      if (entered_pin == LOGIN_passcode)
        MP3_menu__LOGIN_handle_correct_passcode();
      else
        MP3_menu__LOGIN_handle_incorrect_passcode();
    }
  } else if (key == LOGIN_clear_key) {
    MP3_menu__LOGIN_refresh_passcode();
  }
}

LOGIN_stats MP3_menu__LOGIN_get_LOGIN_status(void) { return LOG_stats; }

//==================================================================
//                    M A I N   M E N U
//==================================================================

uint8_t MAIN_MENU_cur_index = 0;
string16_t MAIN_MENU_options[2] = {"Setting        ", "Song list      "};

static void MP3_menu__MAIN_MENU_refresh(void) {
  MAIN_MENU_cur_index = 0;
  lcd__write_string(">", LINE_2, 0, 0);
  lcd__write_string("--- MENU ----", LINE_1, 1, 0);
  lcd__write_string(MAIN_MENU_options[0], LINE_2, 1, 0);
}

static void MP3_menu__refresh_main_menu(void);
static void MP3_menu__MAIN_MENU_handle_select_song_list(void) {
  delay__ms(100);
  Page_Display = song_list;
  MP3_menu__refresh_main_menu();
}

static void MP3_menu__MAIN_MENU_handler(key_press) {
  if (key_press == ENTER) {
    if (MAIN_MENU_cur_index) {
      MP3_menu__MAIN_MENU_handle_select_song_list();
    }
  } else if (!MAIN_MENU_cur_index && key_press == SCROLL_DOWN) {
    MAIN_MENU_cur_index++;
    lcd__write_string(MAIN_MENU_options[0], LINE_1, 1, 0);
    lcd__write_string(MAIN_MENU_options[1], LINE_2, 1, 0);
  } else if (MAIN_MENU_cur_index && key_press == SCROLL_UP)
    MP3_menu__MAIN_MENU_refresh();
}

//==================================================================

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

void MP3_menu__init(void) {
  MP3_song__init();
  lcd__init();
  MP3_menu__rebooting_mp3();
  Page_Display = reboot;
}

static void MP3_menu__main_menu_scroll_up(void) {
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

static void MP3_menu__main_menu_scroll_down(void) {
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

static void MP3_menu__song_handler(void) {
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
void MP3_menu__UI_handler(const unsigned char key_press) {
  switch (Page_Display) {
  case reboot:
    MP3_menu__LOGIN_lock_menu_page();
    MP3_menu__LOGIN_setup_passcode_UI();
    break;
  case login:
    // vTaskSuspend(xTaskGetHandle("mp3_login")); // Prevent Context Switching
    MP3_menu__LOGIN_handler(key_press);
    // vTaskResume(xTaskGetHandle("mp3_login"));
    break;
  case main_menu:
    MP3_menu__MAIN_MENU_handler(key_press);
    break;
  case song_list:
    if (key_press == '^') {
      MP3_menu__main_menu_scroll_up();
    } else if (key_press == 'V') {
      MP3_menu__main_menu_scroll_down();
    } else if (key_press == 'E') {
      MP3_menu__song_handler();
    }
    break;
  default:
    break;
  }
}

uint8_t MP3_menu__get_cur_song_index(void) { return cur_play_song; };