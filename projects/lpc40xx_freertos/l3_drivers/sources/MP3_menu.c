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
#define SCROLL_RIGHT '>'
#define SCROLL_LEFT '<'
#define ENTER 'E'
#define RETURN 'B'

static void MP3_menu__rebooting_mp3(void);

void MP3_menu__init(void) {
  MP3_song__init();
  lcd__init();
  MP3_menu__rebooting_mp3();
  Page_Display = reboot;
}
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
  vTaskSuspend(xTaskGetHandle("rotate_str"));
}
static void MP3_menu__LOGIN_unlock_menu_page(void) {
  vTaskResume(xTaskGetHandle("reader"));
  vTaskResume(xTaskGetHandle("player"));
  vTaskResume(xTaskGetHandle("volume"));
}

static void MP3_menu__LOGIN_setup_passcode_UI(void) {
  lcd__write_string("}             ", LINE_2, 0, 0);
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
  lcd__write_string("}", LINE_2, 0, 0);
  lcd__write_string("--- MENU ----", LINE_1, 1, 0);
  lcd__write_string(MAIN_MENU_options[0], LINE_2, 1, 0);
}

static void MP3_menu__SONG_LIST_reset(void);
static void MP3_menu__MAIN_MENU_handle_select_song_list(void) {
  delay__ms(100);
  Page_Display = song_list;
  vTaskResume(xTaskGetHandle("rotate_str"));
  MP3_menu__SONG_LIST_reset();
}

static void MP3_menu__MAIN_MENU_handler(key_press) {
  if (key_press == ENTER) {
    if (MAIN_MENU_cur_index) {
      MP3_menu__MAIN_MENU_handle_select_song_list();
    } else {
    }

  } else if (!MAIN_MENU_cur_index && key_press == SCROLL_DOWN) {
    MAIN_MENU_cur_index++;
    lcd__write_string(MAIN_MENU_options[0], LINE_1, 1, 0);
    lcd__write_string(MAIN_MENU_options[1], LINE_2, 1, 0);
  } else if (MAIN_MENU_cur_index && key_press == SCROLL_UP)
    MP3_menu__MAIN_MENU_refresh();
}

//==================================================================
//                    S O N G   L I S T
//==================================================================
#define select_criteria -1
static const SONGS *SONG_LIST;
static int8_t cur_playing_song = -1;
static int8_t cur_song_cursor_index = select_criteria;
static string64_t cur_selected_song = {0};
static MP3_song_payload request_payload;

static string48_t cur_criteria = {0};

//=========== Retrieve Current Criteria Display Cases ===========
static void MP3_menu__SONG_LIST__case_start_year_NULL(void) {
  string16_t end_year = {0};
  sprintf(end_year, "%d", *request_payload.end_year);
  strcat(cur_criteria, "<");
  strncat(cur_criteria, end_year, sizeof(string16_t));
}

static void MP3_menu__SONG_LIST__case_end_year_NULL(void) {
  string16_t start_year = {0};
  sprintf(start_year, "%d", *request_payload.start_year);
  strcat(cur_criteria, ">");
  strncat(cur_criteria, start_year, sizeof(string16_t));
}

static void MP3_menu__SONG_LIST__case_start_and_end_year_NOT_NULL(void) {
  string16_t start_year = {0}, end_year = {0};
  sprintf(start_year, "%d", *request_payload.start_year);
  sprintf(end_year, "%d", *request_payload.end_year);
  if (*request_payload.start_year == *request_payload.end_year) {
    strncat(cur_criteria, start_year, sizeof(string16_t));
  } else {
    strncat(cur_criteria, start_year, sizeof(string16_t));
    strcat(cur_criteria, "-");
    strncat(cur_criteria, end_year, sizeof(string16_t));
  }
}
//==============================================================

static void MP3_menu__SONG_LIST_retrieve_cur_crtieria_display(void) {
  memset(cur_criteria, 0, sizeof(string48_t));
  snprintf(cur_criteria, sizeof(string16_t), "Genre: %s", request_payload.genre);
  if (request_payload.start_year != NULL || request_payload.end_year != NULL) {
    string16_t start_year = {0}, end_year = {0};
    strcat(cur_criteria, " & Year: ");
    if (request_payload.start_year == NULL) {
      MP3_menu__SONG_LIST__case_start_year_NULL();
    } else if (request_payload.end_year == NULL) {
      MP3_menu__SONG_LIST__case_end_year_NULL();
    } else {
      MP3_menu__SONG_LIST__case_start_and_end_year_NOT_NULL();
    }
  }
  strcat(cur_criteria, "    ");
}

static MP3_menu__SONG_LIST_concat_songname_artist(char *song_display, uint8_t INDEX) {
  snprintf(song_display, sizeof(string64_t) - 1, "%s - %s   ", SONG_LIST[INDEX].song_name, SONG_LIST[INDEX].artist);
}

//==================== UI Display functions =====================
static void MP3_menu__SONG_LIST_display_top_right(int8_t INDEX) {
  string16_t display = {0};
  if (INDEX == -1)
    sprintf(display, "CR");
  else {
    if (INDEX + 1 < 10)
      sprintf(display, " %d", INDEX + 1);
    else
      sprintf(display, "%d", INDEX + 1);
  }
  lcd__write_string(display, LINE_1, 14, 0);
}

static void MP3_menu__SONG_LIST_display_top_left(const char *display_input) {
  string16_t display = {0};
  strncpy(display, display_input, 12);
  lcd__write_string(display, LINE_1, 1, 0);
}
static void MP3_menu__SONG_LIST_display_bottom_left(const char *display_input) {
  string16_t display = {0};
  strncpy(display, display_input, 12);
  lcd__write_string(display, LINE_2, 1, 0);
}

static void MP3_menu__SONG_LIST_refresh(void) {
  cur_song_cursor_index = -1;
  lcd__write_string("}", LINE_1, 0, 0);
  lcd__write_string(" ", LINE_2, 0, 0);
  MP3_menu__SONG_LIST_display_top_left(cur_criteria);
  MP3_menu__SONG_LIST_display_top_right(cur_song_cursor_index);
  string64_t song_display = {0};
  if (MP3_song__get_response_size()) {
    MP3_menu__SONG_LIST_concat_songname_artist(song_display, cur_song_cursor_index + 1);
    MP3_menu__SONG_LIST_display_bottom_left(song_display);
  } else {
    string16_t NO_RESULT = " NO RESULT  ";
    MP3_menu__SONG_LIST_display_bottom_left(NO_RESULT);
  }
}

static void MP3_menu__SONG_LIST_reset(void) {
  request_payload = (MP3_song_payload){"All", NULL, NULL};
  SONG_LIST = MP3_song__query_by_genre_and_year(&request_payload);
  MP3_menu__SONG_LIST_retrieve_cur_crtieria_display();
  MP3_menu__SONG_LIST_refresh();
}

//================== KEYPAD INTEGRATION ========================
static void MP3_menu__FILTER_refresh(void);
static void MP3_menu__FILTER_clear_year(void);

static void MP3_menu_SONG_LIST_enter_handler(void) {
  if (cur_song_cursor_index == select_criteria) {
    vTaskSuspend(xTaskGetHandle("rotate_str"));
    Page_Display = filter;
    MP3_menu__FILTER_refresh();
    MP3_menu__FILTER_clear_year();
  }
}

static void MP3_menu_SONG_LIST_return_handler(void) {
  Page_Display = main_menu;
  vTaskSuspend(xTaskGetHandle("rotate_str"));
  MP3_menu__MAIN_MENU_refresh();
}

static void MP3_menu_SONG_LIST_scroll_down(void) {
  if (cur_song_cursor_index++ == MP3_song__get_response_size() - 1)
    MP3_menu__SONG_LIST_refresh();
  else {
    string64_t next_song = {0};
    MP3_menu__SONG_LIST_concat_songname_artist(cur_selected_song, cur_song_cursor_index);
    if (cur_song_cursor_index < MP3_song__get_response_size() - 1)
      MP3_menu__SONG_LIST_concat_songname_artist(next_song, cur_song_cursor_index + 1);
    else
      strcat(next_song, "            ");
    MP3_menu__SONG_LIST_display_top_left(cur_selected_song);
    MP3_menu__SONG_LIST_display_bottom_left(next_song);
    MP3_menu__SONG_LIST_display_top_right(cur_song_cursor_index);
  }
}

static void MP3_menu_SONG_LIST_scroll_up(void) {
  if (cur_song_cursor_index-- == 0 || !MP3_song__get_response_size())
    MP3_menu__SONG_LIST_refresh();
  else {
    string64_t next_song = {0};
    if (cur_song_cursor_index >= 0)
      MP3_menu__SONG_LIST_concat_songname_artist(next_song, cur_song_cursor_index + 1);
    else {
      strcat(next_song, "            ");
      cur_song_cursor_index = MP3_song__get_response_size() - 1;
    }
    MP3_menu__SONG_LIST_display_top_left(cur_selected_song);
    MP3_menu__SONG_LIST_display_bottom_left(next_song);
    MP3_menu__SONG_LIST_display_top_right(cur_song_cursor_index);
  }
}

static void MP3_menu__SONG_LIST_handler(const char key) {
  if (key == ENTER) {
    MP3_menu_SONG_LIST_enter_handler();
  } else if (key == RETURN)
    MP3_menu_SONG_LIST_return_handler();
  else if (key == SCROLL_UP) {
    MP3_menu_SONG_LIST_scroll_up();
  } else if (key == SCROLL_DOWN) {
    MP3_menu_SONG_LIST_scroll_down();
  }
}

//==================================================================
//                      F I L T E R
//==================================================================
#define FILTER_genre 0
#define FILTER_year 1
#define FILTER_submit 2
static uint8_t FILTER_cursor;

//==========GENRE================
static uint8_t FILTER_cur_genre_option = 0;
static const string16_t *FILTER_genre_options;
static bool FILTER_is_genre_option_change;

static void MP3_menu__FILTER_display_genre_text(const char *display_input) {
  lcd__write_string("               ", LINE_2, 1, 0);
  string16_t display = {0};
  snprintf(display, 14, "%s", display_input);
  lcd__write_string(display, LINE_2, 1, 0);
}

//==========YEAR=================
#define FILTER_start_year_cursor 0
#define FILTER_end_year_cursor 1
static uint16_t FILTER_start_year = 0;
static uint16_t FILTER_end_year = 0;
static uint16_t FILTER_year_cursor;

static void MP3_menu__FILTER_display_year_text(uint16_t start_input_year, uint16_t end_input_year) {
  string16_t display_start_year = {0};
  string16_t display_end_year = {0};
  lcd__write_string("     -          ", LINE_2, 1, 0);
  if (start_input_year)
    sprintf(display_start_year, "%d", start_input_year);
  if (end_input_year)
    sprintf(display_end_year, "%d", end_input_year);
  lcd__write_string(display_start_year, LINE_2, 1, 0);
  lcd__write_string(display_end_year, LINE_2, 6 + 3, 0);
}

static void MP3_menu__FILTER_year_move_cursor_to_left(void) {
  lcd__write_string("}", LINE_2, 0, 0);
  lcd__write_string(" ", LINE_2, 8, 0);
}
static void MP3_menu__FILTER_year_move_cursor_to_right(void) {
  lcd__write_string(" ", LINE_2, 0, 0);
  lcd__write_string("}", LINE_2, 8, 0);
}

static void MP3_menu__FILTER_clear_year(void) { FILTER_start_year = FILTER_end_year = 0; }

//================== KEYPAD INTEGRATION ========================
static void MP3_menu__FILTER_input_number_handler(const char key) {
  if (FILTER_cursor == FILTER_year) {
    uint8_t input_number = key - 0x30;
    if (FILTER_year_cursor == FILTER_start_year_cursor) {
      FILTER_start_year = FILTER_start_year * 10 + input_number;
      if (FILTER_start_year > 9999)
        FILTER_start_year = input_number;
      MP3_menu__FILTER_display_year_text(FILTER_start_year, FILTER_end_year);
      MP3_menu__FILTER_year_move_cursor_to_left();
    } else {
      FILTER_end_year = FILTER_end_year * 10 + input_number;
      if (FILTER_end_year > 9999)
        FILTER_end_year = input_number;
      MP3_menu__FILTER_display_year_text(FILTER_start_year, FILTER_end_year);
      MP3_menu__FILTER_year_move_cursor_to_right();
    }
  }
}

static void MP3_menu__FILTER_scroll_left(void) {
  if (FILTER_cursor == FILTER_genre) {
    if (FILTER_cur_genre_option-- == 0)
      FILTER_cur_genre_option = MP3_song__get_num_of_genre_options() - 1;
    MP3_menu__FILTER_display_genre_text(FILTER_genre_options[FILTER_cur_genre_option]);
    FILTER_is_genre_option_change = true;
  } else if (FILTER_cursor == FILTER_year) {
    if (FILTER_year_cursor == FILTER_end_year_cursor) {
      FILTER_year_cursor = FILTER_start_year_cursor;
      MP3_menu__FILTER_year_move_cursor_to_left();
    }
  }
}

static void MP3_menu_FILTER_scroll_right(void) {
  if (FILTER_cursor == FILTER_genre) {
    if (FILTER_cur_genre_option++ >= MP3_song__get_num_of_genre_options() - 1)
      FILTER_cur_genre_option = 0;
    MP3_menu__FILTER_display_genre_text(FILTER_genre_options[FILTER_cur_genre_option]);
    FILTER_is_genre_option_change = true;
  } else if (FILTER_cursor == FILTER_year) {
    if (FILTER_year_cursor == FILTER_start_year_cursor) {
      FILTER_year_cursor = FILTER_end_year_cursor;
      MP3_menu__FILTER_year_move_cursor_to_right();
    }
  }
}

static void MP3_menu_FILTER_at_genre(void);
static void MP3_menu_FILTER_at_year(void);
static void MP3_menu_FILTER_at_submit(void);

static void MP3_menu_FILTER_scroll_up(void) {
  if (FILTER_cursor == FILTER_year)
    MP3_menu_FILTER_at_genre();
  else if (FILTER_cursor == FILTER_submit)
    MP3_menu_FILTER_at_year();
}

static void MP3_menu_FILTER_scroll_down(void) {
  if (FILTER_cursor == FILTER_genre)
    MP3_menu_FILTER_at_year();
  else if (FILTER_cursor == FILTER_year)
    MP3_menu_FILTER_at_submit();
}
//====================================================================================

static void MP3_menu_FILTER_at_submit(void) {
  FILTER_cursor = FILTER_submit;
  lcd__write_string("                ", LINE_1, 0, 0);
  lcd__write_string("}SUBMIT         ", LINE_2, 0, 0);
  lcd__write_string("^", LINE_1, 15, 0);
}

static void MP3_menu_FILTER_at_year(void) {
  FILTER_cursor = FILTER_year;
  FILTER_year_cursor = FILTER_start_year_cursor;
  lcd__write_string(" YEAR             ", LINE_1, 0, 0);
  lcd__write_string("}", LINE_2, 0, 0);
  MP3_menu__FILTER_display_year_text(FILTER_start_year, FILTER_end_year);
  lcd__write_string("^", LINE_1, 15, 0);
  lcd__write_string("V", LINE_2, 15, 0);
}

static void MP3_menu_FILTER_at_genre(void) {
  FILTER_cursor = FILTER_genre;
  lcd__write_string(" GENRE            ", LINE_1, 0, 0);
  lcd__write_string("}", LINE_2, 0, 0);
  if (FILTER_is_genre_option_change) {
    MP3_menu__FILTER_display_genre_text(FILTER_genre_options[FILTER_cur_genre_option]);
  } else {
    string16_t display_genre = {0};
    sprintf(display_genre, "%s", request_payload.genre);
    MP3_menu__FILTER_display_genre_text(display_genre);
  }
  lcd__write_string("V", LINE_2, 15, 0);
}
static void MP3_menu__FILTER_refresh(void) {
  FILTER_is_genre_option_change = false;
  FILTER_genre_options = MP3_song_get_genre_options();
  uint16_t FILTER_start_year = (request_payload.start_year == NULL) ? 0 : *request_payload.start_year;
  uint16_t FILTER_end_year = (request_payload.end_year == NULL) ? 0 : *request_payload.end_year;
  MP3_menu_FILTER_at_genre();
}

static void MP3_menu__FILTER_handler(const char key) {
  if (key == ENTER) {

  } else if (key >= '0' && key <= '9') {
    MP3_menu__FILTER_input_number_handler(key);
  } else if (key == SCROLL_LEFT) {
    MP3_menu__FILTER_scroll_left();
  } else if (key == SCROLL_RIGHT) {
    MP3_menu_FILTER_scroll_right();
  } else if (key == SCROLL_UP) {
    MP3_menu_FILTER_scroll_up();
  } else if (key == SCROLL_DOWN) {
    MP3_menu_FILTER_scroll_down();
  }
}
/*
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
}*/

void MP3_menu_SONG_LIST_rotate_string(void) {
  switch (Page_Display) {
  case song_list:
    if (cur_song_cursor_index == -1) {
      rotate_string(cur_criteria, LEFT);
      MP3_menu__SONG_LIST_display_top_left(cur_criteria);
    } else {
      rotate_string(cur_selected_song, LEFT);
      MP3_menu__SONG_LIST_display_top_left(cur_selected_song);
    }
    break;
  case filter:
    break;
  default:
    break;
  }
}

extern QueueHandle_t song_name_queue;
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
    MP3_menu__SONG_LIST_handler(key_press);

    /*
      if (key_press == '^') {
        MP3_menu__main_menu_scroll_up();
      } else if (key_press == 'V') {
        MP3_menu__main_menu_scroll_down();
      } else if (key_press == 'E') {
        MP3_menu__song_handler();
      }*/
    break;
  case filter:
    MP3_menu__FILTER_handler(key_press);
    break;
  default:
    break;
  }
}

uint8_t MP3_menu__get_cur_song_index(void) { return cur_play_song; };