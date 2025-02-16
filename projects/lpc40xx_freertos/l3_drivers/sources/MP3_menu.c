#include "MP3_menu.h"
#include "FreeRTOS.h"
#include "MP3_song.h"
#include "delay.h"
#include "lcd.h"
#include "queue.h"
#include <string.h>

static UI_Page Page_Display;

void suspend_rotate_task(void);
//==================================================================
//               T A B L E   O F    C O N T E N T
//==================================================================
// Note: Unused Functions. Use for redirecting under this source file
// Right click function name and go to definition;
static void Redirect_to_reboot_code();
static void Redirect_to_login_code();
static void Redirect_to_main_menu_code();
static void Redirect_to_setting_code();
static void Redirect_to_song_list_code();
static void Redirect_to_song_info_code();
static void Redirect_to_filter_code();

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

typedef enum {
  pot_vol = 1, // states
  pot_bass,
  pot_trem,
  pot_LOCK,
} POT;

static POT pot_controller = pot_vol;

static void MP3_menu__rebooting_mp3(void);

void MP3_menu__init(void) {
  MP3_song__init();
  lcd__init();
  MP3_menu__rebooting_mp3();
  Page_Display = reboot;
}

static bool MP3_stop = false;
static bool MP3_pause = false;

static MP3_speed SPEED = MP3_normal;

MP3_speed MP3_menu__get_MP3_speed(void) { return SPEED; }

static void MP3_menu__pause(void) { MP3_pause = !MP3_pause; }

static void MP3_menu__stop(void) { MP3_stop = true; }

bool MP3_menu__get_stop(void) { return MP3_stop; }

bool MP3_menu__get_pause(void) { return MP3_pause; }

//==================================================================
//                         C R A S H
//==================================================================
void MP3_crash__print_crash_message(void) {
  Page_Display = crash;
  suspend_rotate_task();
  lcd__write_string("SD NOT CONNECTED", LINE_1, 0, 0);
  lcd__write_string("PLEASE REBOOT   ", LINE_2, 0, 0);
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
#define LOGIN_clear_key 'E'

#define LOGIN_passcode 1882

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
  vTaskSuspend(xTaskGetHandle("mp3_login"));
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
static void MP3_menu__SONG_LIST_reset(void);
static void MP3_menu__LOGIN_handle_correct_passcode(void) {
  MP3_menu__LOGIN_unlock_menu_page();
  LOG_stats = SUCCESS;
  delay__ms(500);
  lcd__write_string("               ", LINE_1, 1, 0);
  lcd__write_string("               ", LINE_2, 0, 0);
  lcd__write_string("WELCOME!", LINE_1, 4, 100);
  lcd__write_string("      ...    ", LINE_2, 0, 100);
  delay__ms(1000);
  Page_Display = main_menu;
  MP3_menu__MAIN_MENU_refresh();
  MP3_menu__SONG_LIST_reset();
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
static void Redirect_to_main_menu_code(){};
//-----------------------------------

uint8_t MAIN_MENU_cur_index = 0;
string16_t MAIN_MENU_options[2] = {"Setting        ", "Song list      "};

static void MP3_menu__MAIN_MENU_refresh(void) {
  MAIN_MENU_cur_index = 0;
  lcd__write_string("}", LINE_2, 0, 0);
  lcd__write_string(" --- MENU ----  ", LINE_1, 0, 0);
  lcd__write_string(MAIN_MENU_options[0], LINE_2, 1, 0);
}

static void MP3_menu__SONG_LIST_refresh(void);
static void MP3_menu__SETTING_refresh(void);
static void MP3_menu__MAIN_MENU_handle_select_song_list(void) {
  Page_Display = song_list;
  MP3_menu__SONG_LIST_refresh();
  vTaskResume(xTaskGetHandle("rotate_str"));
}

static void MP3_menu__MAIN_MENU_handler(key_press) {
  if (key_press == ENTER || key_press == SCROLL_RIGHT) {
    if (MAIN_MENU_cur_index) {
      MP3_menu__MAIN_MENU_handle_select_song_list();
    } else {
      Page_Display = setting;
      MP3_menu__SETTING_refresh();
    }
  } else if (!MAIN_MENU_cur_index && key_press == SCROLL_DOWN) {
    MAIN_MENU_cur_index++;
    lcd__write_string(MAIN_MENU_options[0], LINE_1, 1, 0);
    lcd__write_string(MAIN_MENU_options[1], LINE_2, 1, 0);
  } else if (MAIN_MENU_cur_index && key_press == SCROLL_UP)
    MP3_menu__MAIN_MENU_refresh();
}

//==================================================================
//                      S E T T I N G
//==================================================================
static void Redirect_to_setting_code() {}
//-----------------------------------

#define SETTING_playback 0
#define SETTING_autoplay 1
#define SETTING_rewind_fast_forward 2
static bool enable_playback = false;
static bool enable_autoplay = false;
static bool enable_rewind_fast_forward = false;
static uint8_t SETTING_cur_cursor = 0;

/*
  SETTING LOGIC
  PLAYBACK  AUTOPLAY | RESULT
     1          X    | PLAYBACK
     0          1    | AUTOPLAY
     0          0    | NONE
*/

static void MP3_menu__SETTING_refresh(void) {
  SETTING_cur_cursor = SETTING_playback;
  lcd__write_string("}", LINE_2, 0, 0);
  lcd__write_string(" -- SETTINGS -- ", LINE_1, 0, 0);
  lcd__write_string("Playback   ", LINE_2, 1, 0);
  string16_t enable_display = {0};
  sprintf(enable_display, "%s", ((enable_playback) ? "ON " : "OFF"));
  lcd__write_string(enable_display, LINE_2, 13, 0);
}

static void MP3_menu__SETTING_scroll_up(void) {
  if (SETTING_cur_cursor == SETTING_autoplay) {
    SETTING_cur_cursor--;
    MP3_menu__SETTING_refresh();
  } else if (SETTING_cur_cursor == SETTING_rewind_fast_forward) {
    SETTING_cur_cursor--;
    lcd__write_string("}", LINE_2, 0, 0);
    lcd__write_string("Playback   ", LINE_1, 1, 0);
    lcd__write_string("Autoplay   ", LINE_2, 1, 0);

    string16_t enable_playback_display = {0};
    sprintf(enable_playback_display, "%s", ((enable_playback) ? "ON " : "OFF"));

    string16_t enable_autoplay_display = {0};
    sprintf(enable_autoplay_display, "%s", ((enable_autoplay) ? "ON " : "OFF"));

    lcd__write_string(enable_playback_display, LINE_1, 13, 0);
    lcd__write_string(enable_autoplay_display, LINE_2, 13, 0);
  }
}
static void MP3_menu__SETTING_scroll_down(void) {
  if (SETTING_cur_cursor == SETTING_playback) {
    SETTING_cur_cursor++;
    lcd__write_string("}", LINE_2, 0, 0);
    lcd__write_string("Playback   ", LINE_1, 1, 0);
    lcd__write_string("Autoplay   ", LINE_2, 1, 0);

    string16_t enable_playback_display = {0};
    sprintf(enable_playback_display, "%s", ((enable_playback) ? "ON " : "OFF"));

    string16_t enable_autoplay_display = {0};
    sprintf(enable_autoplay_display, "%s", ((enable_autoplay) ? "ON " : "OFF"));

    lcd__write_string(enable_playback_display, LINE_1, 13, 0);
    lcd__write_string(enable_autoplay_display, LINE_2, 13, 0);
  } else if (SETTING_cur_cursor == SETTING_autoplay) {
    SETTING_cur_cursor++;
    lcd__write_string("}", LINE_2, 0, 0);
    lcd__write_string("Autoplay   ", LINE_1, 1, 0);
    lcd__write_string("Rewind/Fast", LINE_2, 1, 0);

    string16_t enable_autoplay_display = {0};
    sprintf(enable_autoplay_display, "%s", ((enable_autoplay) ? "ON " : "OFF"));

    string16_t enable_rewind_fast_forward_display = {0};
    sprintf(enable_rewind_fast_forward_display, "%s", ((enable_rewind_fast_forward) ? "ON " : "OFF"));

    lcd__write_string(enable_autoplay_display, LINE_1, 13, 0);
    lcd__write_string(enable_rewind_fast_forward_display, LINE_2, 13, 0);
  }
}

static void MP3_menu__SETTING_press_enter(void) {
  if (SETTING_cur_cursor == SETTING_playback) {
    enable_playback = !enable_playback;
    string16_t enable_playback_display = {0};
    sprintf(enable_playback_display, "%s", ((enable_playback) ? "ON " : "OFF"));
    lcd__write_string(enable_playback_display, LINE_2, 13, 0);
  } else if (SETTING_cur_cursor == SETTING_autoplay) {
    enable_autoplay = !enable_autoplay;
    string16_t enable_autoplay_display = {0};
    sprintf(enable_autoplay_display, "%s", ((enable_autoplay) ? "ON " : "OFF"));
    lcd__write_string(enable_autoplay_display, LINE_2, 13, 0);
  } else {
    enable_rewind_fast_forward = !enable_rewind_fast_forward;
    string16_t enable_rewind_fast_forward_display = {0};
    sprintf(enable_rewind_fast_forward_display, "%s", ((enable_rewind_fast_forward) ? "ON " : "OFF"));
    SPEED = MP3_normal;
    lcd__write_string(enable_rewind_fast_forward_display, LINE_2, 13, 0);
  }
}

static void MP3_menu__SETTING_handler(const char key) {
  if (key == ENTER || key == SCROLL_RIGHT)
    MP3_menu__SETTING_press_enter();
  else if (key == SCROLL_UP)
    MP3_menu__SETTING_scroll_up();
  else if (key == SCROLL_DOWN)
    MP3_menu__SETTING_scroll_down();
  else if (key == RETURN || key == SCROLL_LEFT) {
    Page_Display = main_menu;
    MP3_menu__MAIN_MENU_refresh();
  }
}
//==================================================================
//                    S O N G   L I S T
//==================================================================
static void Redirect_to_song_list_code() {}
//-----------------------------------

#define select_criteria -1
static const SONGS *SONG_LIST;
static int8_t cur_song_cursor_index = select_criteria;
static string64_t cur_selected_song = {0};
static MP3_song_payload request_payload;
static SONGS cur_playing_song = {0};

static string48_t cur_criteria = {0};
static uint8_t cur_select_song_id = 0;
static bool SONG_INFO_at_no_play_song_page = false;

//=========== Retrieve Current Criteria Display Cases ===========
static void MP3_menu__SONG_LIST__case_start_year_NULL(void) {
  string16_t end_year = {0};
  sprintf(end_year, "%d", *request_payload.end_year);
  strcat(cur_criteria, "<=");
  strncat(cur_criteria, end_year, sizeof(string16_t));
}

static void MP3_menu__SONG_LIST__case_end_year_NULL(void) {
  string16_t start_year = {0};
  sprintf(start_year, "%d", *request_payload.start_year);
  strcat(cur_criteria, ">=");
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
  snprintf(cur_criteria, sizeof(string32_t), "Genre: %s", request_payload.genre);
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

static void MP3_menu__SONG_LIST_display_bottom_right(const char *display_input) {
  string16_t display = {0};
  strncpy(display, display_input, 2);
  lcd__write_string(display, LINE_2, 14, 0);
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
static void MP3_menu__SONG_INFO_set_pot_control_handler(uint8_t);
static void MP3_menu__SONG_LIST_refresh(void) {
  cur_song_cursor_index = -1;
  cur_select_song_id = 0;
  SONG_INFO_at_no_play_song_page = false;
  lcd__write_string("}", LINE_1, 0, 0);
  lcd__write_string(" ", LINE_2, 0, 0);
  lcd__write_string("|", LINE_1, 13, 0);
  lcd__write_string("|", LINE_2, 13, 0);
  MP3_menu__SONG_INFO_set_pot_control_handler(pot_controller + 0x30);
  MP3_menu__SONG_LIST_display_top_left(cur_criteria);
  MP3_menu__SONG_LIST_display_top_right(cur_song_cursor_index);
  string64_t song_display = {0};
  if (MP3_song__get_response_size()) {
    MP3_menu__SONG_LIST_concat_songname_artist(song_display, cur_song_cursor_index + 1);
    MP3_menu__SONG_LIST_display_bottom_left(song_display);
  } else {
    string16_t NO_RESULT = " No Result  ";
    MP3_menu__SONG_LIST_display_bottom_left(NO_RESULT);
  }
}

static void MP3_menu__SONG_LIST_reset(void) {
  request_payload = (MP3_song_payload){"All", NULL, NULL};
  SONG_LIST = MP3_song__query_by_genre_and_year(&request_payload);
  MP3_menu__SONG_LIST_retrieve_cur_crtieria_display();
  // MP3_menu__SONG_LIST_refresh();
}

//================== KEYPAD INTEGRATION ========================
static void MP3_menu__FILTER_refresh(void);
static void MP3_menu__SONG_INFO_open(void);

static void MP3_menu_SONG_LIST_enter_handler(void) {
  if (cur_song_cursor_index == select_criteria) {
    suspend_rotate_task();
    Page_Display = filter;
    MP3_menu__FILTER_refresh();
  } else {
    Page_Display = song_info;
    MP3_menu__SONG_INFO_open();
  }
}

static void MP3_menu_SONG_LIST_return_handler(void) {
  Page_Display = main_menu;
  suspend_rotate_task();
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
    MP3_menu__SONG_LIST_concat_songname_artist(cur_selected_song, cur_song_cursor_index);
    MP3_menu__SONG_LIST_display_top_left(cur_selected_song);
    MP3_menu__SONG_LIST_display_bottom_left(next_song);
    MP3_menu__SONG_LIST_display_top_right(cur_song_cursor_index);
  }
}

static void MP3_menu__SONG_INFO_open_current_playing_song(void);
static void MP3_menu__SONG_LIST_handler(const char key) {
  if (key == ENTER) {
    MP3_menu_SONG_LIST_enter_handler();
  } else if (key == RETURN || key == SCROLL_LEFT) {
    MP3_menu_SONG_LIST_return_handler();
  } else if (key == SCROLL_RIGHT) {
    Page_Display = song_info;
    MP3_menu__SONG_INFO_open_current_playing_song();
  } else if (key == SCROLL_UP) {
    MP3_menu_SONG_LIST_scroll_up();
  } else if (key == SCROLL_DOWN) {
    MP3_menu_SONG_LIST_scroll_down();
  }
}

//==================================================================
//                   S O N G   I N F O
//==================================================================
static void Redirect_to_song_info_code() {}
//-----------------------------------

static string192_t SONG_INFO_select_song_display = {0};
static void MP3_menu__SONG_INFO_set_display(SONGS SONG) {
  snprintf(SONG_INFO_select_song_display, sizeof(string192_t) - 1, "%s - %s - %s - %d - %s    ", SONG.song_name,
           SONG.artist, SONG.album, SONG.year, SONG.genre);
}

static void MP3__menu__SONG_INFO_print_status(SONGS SONG) {
  cur_select_song_id = SONG.id;
  lcd__write_string("}            ", LINE_2, 0, 0);
  if (cur_playing_song.id == SONG.id) {
    lcd__write_string(" P", LINE_1, 14, 0);
    if (MP3_menu__get_pause())
      lcd__write_string("Paused", LINE_2, 1, 0);
    else {
      if (SPEED == MP3_fast_forward) {
        lcd__write_string("Fast Forw...", LINE_2, 1, 0);
      } else if (SPEED == MP3_rewind) {
        lcd__write_string("Rewind...", LINE_2, 1, 0);
      } else {
        lcd__write_string("Playing...", LINE_2, 1, 0);
      }
    }
  } else {
    lcd__write_string("Play", LINE_2, 1, 0);
  }
}

static void MP3_menu__SONG_INFO_open_current_playing_song(void) {
  lcd__write_string("             ", LINE_1, 0, 0);
  lcd__write_string("             ", LINE_2, 0, 0);
  if (cur_playing_song.id) {
    cur_song_cursor_index = MP3_song__search_song_response_by_id(cur_playing_song.id);
    MP3_menu__SONG_INFO_set_display(cur_playing_song);
    MP3__menu__SONG_INFO_print_status(cur_playing_song);
  } else {
    lcd__write_string("  ", LINE_1, 14, 0);
    lcd__write_string("  ", LINE_2, 14, 0);
    SONG_INFO_at_no_play_song_page = true;
    snprintf(SONG_INFO_select_song_display, sizeof(string192_t) - 1,
             "No song is currently playing. Please select a song     ");
    lcd__write_string("{ Press Ret", LINE_2, 0, 0);
  }
}

static void MP3_menu__SONG_INFO_open(void) {
  lcd__write_string("             ", LINE_1, 0, 0);
  lcd__write_string("             ", LINE_2, 0, 0);
  MP3_menu__SONG_INFO_set_display(SONG_LIST[cur_song_cursor_index]);
  MP3__menu__SONG_INFO_print_status(SONG_LIST[cur_song_cursor_index]);
  lcd__write_string("}", LINE_2, 0, 0);
}

extern QueueHandle_t song_name_queue;
static void MP3_menu__SONG_INFO_play_song(const char *filename) { xQueueSend(song_name_queue, filename, 0); }

static void MP3_menu__SONG_INFO_press_enter_handler(void) {
  if (!SONG_INFO_at_no_play_song_page) {
    if (!cur_playing_song.id || cur_select_song_id != cur_playing_song.id) {
      if (MP3_menu__get_pause())
        MP3_menu__pause();
      cur_playing_song = SONG_LIST[cur_song_cursor_index];
      SPEED = MP3_normal;
      MP3_menu__SONG_INFO_play_song(cur_playing_song.file_name);
    } else {
      MP3_menu__pause();
    }
    MP3__menu__SONG_INFO_print_status(cur_playing_song);
    lcd__write_string("}", LINE_2, 0, 0);
  }
}

static void MP3_menu__SONG_INFO_set_pot_control_handler(uint8_t key) {
  if (key == '1') {
    pot_controller = pot_vol;
    MP3_menu__SONG_LIST_display_bottom_right(" V");
  } else if (key == '2') {
    pot_controller = pot_bass;
    MP3_menu__SONG_LIST_display_bottom_right(" B");
  } else if (key == '3') {
    pot_controller = pot_trem;
    MP3_menu__SONG_LIST_display_bottom_right(" T");
  } else if (key == '4') {
    pot_controller = pot_LOCK;
    MP3_menu__SONG_LIST_display_bottom_right(" L");
  }
}

static void MP3_menu__SONG_INFO_forward_rewind(uint8_t key) {
  if (cur_playing_song.id && cur_select_song_id == cur_playing_song.id) {
    if ((SPEED == MP3_rewind && key == SCROLL_RIGHT) || (SPEED == MP3_fast_forward && key == SCROLL_LEFT)) {
      SPEED = MP3_normal;
      lcd__write_string("}            ", LINE_2, 0, 0);
      lcd__write_string("Playing...", LINE_2, 1, 0);
    } else if (SPEED == MP3_normal) {
      if (key == SCROLL_LEFT) {
        SPEED = MP3_rewind;
        lcd__write_string("}            ", LINE_2, 0, 0);
        lcd__write_string("Rewind...", LINE_2, 1, 0);
      } else if (key == SCROLL_RIGHT) {
        SPEED = MP3_fast_forward;
        lcd__write_string("}            ", LINE_2, 0, 0);
        lcd__write_string("Fast Forw...", LINE_2, 1, 0);
      }
    }
  }
}

static void MP3_menu__SONG_INFO_handler(uint8_t key_press) {
  if (key_press == RETURN || (key_press == SCROLL_LEFT && SONG_INFO_at_no_play_song_page)) {
    MP3_menu__SONG_LIST_refresh();
    Page_Display = song_list;
  } else if (enable_rewind_fast_forward && !MP3_menu__get_pause() &&
             (key_press == SCROLL_LEFT || key_press == SCROLL_RIGHT) && !SONG_INFO_at_no_play_song_page) {
    MP3_menu__SONG_INFO_forward_rewind(key_press);
  } else if (key_press == SCROLL_UP && !SONG_INFO_at_no_play_song_page && cur_song_cursor_index != -1 &&
             MP3_song__get_response_size() != 1) { // SCROLLING UP SONG INFO
    cur_song_cursor_index =
        (cur_song_cursor_index == 0) ? MP3_song__get_response_size() - 1 : cur_song_cursor_index - 1;
    MP3_menu__SONG_LIST_display_top_right(cur_song_cursor_index);
    MP3_menu__SONG_INFO_open();
  } else if (key_press == SCROLL_DOWN && !SONG_INFO_at_no_play_song_page && cur_song_cursor_index != -1 &&
             MP3_song__get_response_size() != 1) { // SCROLLING DOWN SONG INFO
    cur_song_cursor_index =
        (cur_song_cursor_index == MP3_song__get_response_size() - 1) ? 0 : cur_song_cursor_index + 1;
    MP3_menu__SONG_LIST_display_top_right(cur_song_cursor_index);
    MP3_menu__SONG_INFO_open();
  } else if (key_press >= '1' && key_press <= '4') {
    MP3_menu__SONG_INFO_set_pot_control_handler(key_press);
  } else if (key_press == ENTER) {
    MP3_menu__SONG_INFO_press_enter_handler();
  } else if (key_press == '0') {
    if (!MP3_menu__get_pause() && cur_select_song_id && cur_select_song_id == cur_playing_song.id) {
      MP3_menu__stop();
    }
  }
}

//==================================================================
//                      F I L T E R
//==================================================================
static void Redirect_to_filter_code() {}
//-----------------------------------

#define FILTER_genre 0
#define FILTER_year 1
#define FILTER_submit 2
static uint8_t FILTER_cursor;

//==========GENRE================
static uint8_t FILTER_cur_genre_option = 0;
static const string16_t *FILTER_genre_options;
static bool FILTER_is_genre_option_change;

static void MP3_menu__FILTER_display_genre_text(const char *display_input) {
  lcd__write_string("              ", LINE_2, 1, 0);
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
  lcd__write_string("     -        ", LINE_2, 1, 0);
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
//================== KEYPAD INTEGRATION ========================
#define FILTER_submit_success_msg " Input Successfully   "
#define FILTER_invalid_criteria_msg " *Invalid Criteria*   "
static string32_t FILTER_submit_display_message = {0};

static bool MP3_menu__FILTER_check_valid_criteria(void) {
  bool check_start_yr_lte_end_year = (FILTER_start_year) && (FILTER_end_year) && (FILTER_start_year > FILTER_end_year);
  bool check_invalid_yr_input =
      (FILTER_start_year && FILTER_start_year < 1000) || (FILTER_end_year && FILTER_end_year < 1000);
  return !(check_start_yr_lte_end_year || check_invalid_yr_input);
}

static void MP3_menu__FILTER_press_enter(void) {
  if (FILTER_cursor == FILTER_submit) {
    if (MP3_menu__FILTER_check_valid_criteria()) {
      request_payload = (MP3_song_payload){FILTER_genre_options[FILTER_cur_genre_option],
                                           ((FILTER_start_year) ? &FILTER_start_year : NULL),
                                           ((FILTER_end_year) ? &FILTER_end_year : NULL)};
      strncpy(FILTER_submit_display_message, FILTER_submit_success_msg, sizeof(string32_t) - 1);
      MP3_song__query_by_genre_and_year(&request_payload);
      MP3_menu__SONG_LIST_retrieve_cur_crtieria_display();
    } else {
      strncpy(FILTER_submit_display_message, FILTER_invalid_criteria_msg, sizeof(string32_t) - 1);
    }
    vTaskResume(xTaskGetHandle("rotate_str"));
  } else if (FILTER_cursor == FILTER_genre) {
    FILTER_cur_genre_option = 0;
    MP3_menu__FILTER_display_genre_text(FILTER_genre_options[FILTER_cur_genre_option]);
  } else if (FILTER_cursor == FILTER_year) {
    if (FILTER_year_cursor == FILTER_start_year_cursor) {
      FILTER_start_year = 0;
      MP3_menu__FILTER_display_year_text(FILTER_start_year, FILTER_end_year);
    } else {
      FILTER_end_year = 0;
      MP3_menu__FILTER_display_year_text(FILTER_start_year, FILTER_end_year);
      MP3_menu__FILTER_year_move_cursor_to_right();
    }
  }
}
static void MP3_menu__FILTER_press_back(void) {
  Page_Display = song_list;
  MP3_menu__SONG_LIST_refresh();
  vTaskResume(xTaskGetHandle("rotate_str"));
}

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
  else if (FILTER_cursor == FILTER_submit) {
    strcpy(FILTER_submit_display_message, "\0");
    suspend_rotate_task();
    MP3_menu_FILTER_at_year();
  }
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
  FILTER_start_year = (request_payload.start_year == NULL) ? 0 : *request_payload.start_year;
  FILTER_end_year = (request_payload.end_year == NULL) ? 0 : *request_payload.end_year;
  strcpy(FILTER_submit_display_message, "\0");
  MP3_menu_FILTER_at_genre();
}

static void MP3_menu__FILTER_handler(const char key) {
  if (key == ENTER) {
    MP3_menu__FILTER_press_enter();
  } else if (key == RETURN) {
    MP3_menu__FILTER_press_back();
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

// =========VOLUME/BASS/TREMBLE=============
#include "MP3_decoder.h"
#include "semphr.h"
extern SemaphoreHandle_t lcd_write_mutex;
void MP3_menu__VOL_BASS_TREM_adjust_display_handler(uint16_t adc_value, uint16_t *prev_value) {
  if (Page_Display == song_info || Page_Display == song_list) {
    xSemaphoreTake(lcd_write_mutex, portMAX_DELAY);
    if (adc_value < *prev_value && *prev_value - adc_value >= 25) {
      lcd__write_string("V", LINE_2, 14, 0);
      *prev_value = adc_value;
    } else if (adc_value > *prev_value && adc_value - *prev_value >= 25) {
      lcd__write_string("^", LINE_2, 14, 0);
      *prev_value = adc_value;
    }
    // xSemaphoreGive(lcd_write_mutex);
    vTaskDelay(200);
    // xSemaphoreTake(lcd_write_mutex, portMAX_DELAY);
    lcd__write_string(" ", LINE_2, 14, 0);
    xSemaphoreGive(lcd_write_mutex);
  } else {
    *prev_value = adc_value;
  }
}

void MP3_menu__VOL_BASS_TREM_handler(uint16_t adc_value) {
  uint16_t data_send = 0;
  // printf("ADC_VALUE: %i\n", adc_value);
  if (pot_controller == pot_vol) {
    if (adc_value < 200)
      data_send = 0xFE;
    else {
      data_send = 4095 - adc_value;
      data_send = data_send * (0xFF / 3 + 0x20) / 4095;
    }
    MP3_decoder__set_volume(data_send, data_send);
  } else if (pot_controller == pot_bass) {
    data_send = adc_value * (0xF + 1) / 4095;
    data_send = (data_send == 0xF + 1) ? 0xF : data_send;
    MP3_decoder__set_bass(data_send);
  } else if (pot_controller == pot_trem) {
    data_send = adc_value * (0xF + 1) / 4095;
    data_send = (data_send == 0xF + 1) ? 0xF : data_send;

    if (data_send < 0x8)
      data_send = 0xF - (0x8 - 1 - data_send);
    else
      data_send -= 0x8;

    MP3_decoder__set_tremble(data_send);
  }
}

// ==========================================

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
  case song_info:
    MP3_menu__SONG_LIST_display_top_left(SONG_INFO_select_song_display);
    rotate_string(SONG_INFO_select_song_display, LEFT);
    break;
  case filter:
    MP3_menu__SONG_LIST_display_top_left(FILTER_submit_display_message);
    rotate_string(FILTER_submit_display_message, LEFT);
    break;
  default:
    // does nothing
    break;
  }
}

void MP3_menu__finish_song_handler(void) {
  string32_t DONT_USE;
  bool at_cur_song_page = (cur_select_song_id && cur_playing_song.id == cur_select_song_id);
  printf("Song Finish\n");
  // Checks if other song is requested to play
  if (!xQueuePeek(song_name_queue, DONT_USE, 0)) {
    if (SPEED == MP3_rewind && !MP3_stop) {
      printf("Cannot Rewind Further\n");
      MP3_menu__SONG_INFO_play_song(cur_playing_song.file_name);
      if (at_cur_song_page) {
        lcd__write_string("}Playing...  ", LINE_2, 0, 0);
      }
    } else if (enable_playback && !MP3_stop) {
      MP3_menu__SONG_INFO_play_song(cur_playing_song.file_name);
      if (Page_Display == song_info) {
        lcd__write_string("}Replaying...", LINE_2, 0, 0);
        delay__ms(2000);
        MP3_menu__SONG_INFO_open_current_playing_song();
        lcd__write_string("}Playing...  ", LINE_2, 0, 0);
      }
    } else if (enable_autoplay && !MP3_stop) {
      printf("Autoplay\n");
      int8_t cur_song_index = MP3_song__search_song_response_by_id(cur_playing_song.id);

      // If response payload has no result, it would do a replay
      if (MP3_song__get_response_size()) {
        if (cur_song_index != MP3_song_search_invalid && cur_song_index < MP3_song__get_response_size() - 1) {
          cur_song_cursor_index = cur_song_index + 1;
          // cur_playing_song = SONG_LIST[cur_song_index + 1];
        } else {
          cur_song_cursor_index = 0;
        }
        cur_playing_song = SONG_LIST[cur_song_cursor_index];
        MP3_menu__SONG_INFO_play_song(cur_playing_song.file_name);
      } else { // If the playlist is empty
        cur_playing_song = (SONGS){0};
      }
      if (Page_Display == song_info) {
        lcd__write_string("}Next song...", LINE_2, 0, 0);
        delay__ms(2000);
        MP3_menu__SONG_INFO_open_current_playing_song();
        if (MP3_song__get_response_size())
          lcd__write_string("}Playing...  ", LINE_2, 0, 0);
      }
    } else {
      if (at_cur_song_page) {
        lcd__write_string("}Stopping...", LINE_2, 0, 0);
        delay__ms(2000);
        if (MP3_song__search_song_response_by_id(cur_playing_song.id) == MP3_song_search_invalid) {
          Page_Display = song_list;
          MP3_menu__SONG_LIST_refresh();
        } else {
          lcd__write_string("}Play        ", LINE_2, 0, 0);
        }
      }
      cur_playing_song = (SONGS){0};
    }
    SPEED = MP3_normal;
    MP3_stop = false;
  }
}

void MP3_menu__UI_handler(const unsigned char key_press) {
  switch (Page_Display) {
  case reboot:
    MP3_menu__LOGIN_lock_menu_page();
    MP3_menu__LOGIN_setup_passcode_UI();
    break;
  case login:
    MP3_menu__LOGIN_handler(key_press);
    break;
  case main_menu:
    MP3_menu__MAIN_MENU_handler(key_press);
    break;
  case setting:
    MP3_menu__SETTING_handler(key_press);
    break;
  case song_list:
    MP3_menu__SONG_LIST_handler(key_press);
    break;
  case song_info:
    MP3_menu__SONG_INFO_handler(key_press);
    break;
  case filter:
    MP3_menu__FILTER_handler(key_press);
    break;
  default:
    break;
  }
}
