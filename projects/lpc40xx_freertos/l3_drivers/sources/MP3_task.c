#include <stdio.h>

#include "FreeRTOS.h"
#include "adc.h"
#include "board_io.h"

#include "acceleration.h"
#include "common_macros.h"
#include "delay.h"
#include "event_groups.h"
#include "ff.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "gpio_lab.h"
#include "lcd.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "periodic_scheduler.h"
#include "pwm1.h"
#include "queue.h"
#include "sd_card.h"
#include "semphr.h"
#include "sj2_cli.h"
#include "task.h"
#include "uart_lab.h"
#include <stdbool.h>
#include <string.h>

//========== MP3 FIles Library ===========

#include "MP3_decoder.h"
#include "MP3_task.h"

#include "MP3_song.h"
#include "lcd.h"

#include "MP3_keypad.h"
#include "MP3_menu.h"

QueueHandle_t song_name_queue;
static QueueHandle_t song_data_queue;

static SemaphoreHandle_t lcd_write_mutex;
// This mutex is require otherwise char can be written at some undesirable places

// typedef enum { switch__off, switch__on } switch_e;
typedef char songname_t[32 + 1];
typedef char songdata_t[512];

static void read_file(const char *filename) {
  printf("Request received to play/read: '%s'\n", filename);
  FIL file;
  if (!(f_open(&file, filename, FA_OPEN_EXISTING | FA_READ) == FR_OK)) {
  } else {
    printf("FILE OPENED: file size: %i\n", f_size(&file));
    songdata_t buffer = {0}; // zero initialize
    UINT bytes_read = 0;
    songname_t DONT_USE;
    while (!f_eof(&file) && !xQueuePeek(song_name_queue, DONT_USE, 0) && !MP3_menu__get_stop()) {
      if (FR_OK == f_read(&file, buffer, sizeof(buffer) - 1, &bytes_read)) {
        xQueueSend(song_data_queue, buffer, portMAX_DELAY);
      } else
        puts("ERROR: Failed to read file");
      // memset(&buffer[0], 0, sizeof(buffer)); // Write NULLs to all 256 bytes
      while (MP3_menu__get_pause()) {
        vTaskDelay(1);
      }
    }
    MP3_keypad__disable_interrupt();
    xSemaphoreTake(lcd_write_mutex, portMAX_DELAY);
    f_close(&file);
    MP3_menu__finish_song_handler();
    xQueueReset(song_data_queue);
    xSemaphoreGive(lcd_write_mutex);
    MP3_keypad__enable_interrupt();
  }
}

static void mp3_file_reader_task(void *p) {
  songname_t songname = {0};
  while (1) {
    if (xQueueReceive(song_name_queue, songname, 3000)) {
      read_file(songname);
    } else {
      puts("WARNING: No new request to read a file");
    }
  }
}

static void mp3_decoder_send_block(songdata_t data) {
  size_t BYTE_SEND = 32;
  for (size_t index = 0; index < sizeof(songdata_t); index += BYTE_SEND) { // index += 32
    MP3_decoder__send_data((uint8_t *)&data[index], BYTE_SEND);
  }
}

static SemaphoreHandle_t volume_handler_semaphore;

static void mp3_data_player_task(void *p) {
  songdata_t songdata;
  while (1) {
    // memset(&songdata[0], 0, sizeof(songdata_t));
    if (xQueueReceive(song_data_queue, &songdata[0], portMAX_DELAY)) {
      mp3_decoder_send_block(songdata);
    }
    xSemaphoreGive(volume_handler_semaphore);
  }
}

static void mp3_adjust_volume(void *p) {
  uint16_t adc_value;
  uint8_t volume;
  string16_t volume_display;

  while (1) {
    if (xSemaphoreTake(volume_handler_semaphore, portMAX_DELAY) && xSemaphoreTake(lcd_write_mutex, portMAX_DELAY)) {
      adc_value = adc__get_adc_value(ADC__CHANNEL_4);
      volume = (adc_value * 0xFE / 4095);
      MP3_decoder__set_volume(0xFE, volume);
      /*
      volume = 99 - (volume * 99 / 0xFE);
      if (volume < 10)
        sprintf(volume_display, " %d", volume);
      else
        sprintf(volume_display, "%d", volume);
      lcd__write_string(volume_display, LINE_2, 14, 0);*/
      xSemaphoreGive(lcd_write_mutex);
    }
  }
}

QueueHandle_t keypad_char_queue;

static void mp3__interrupt_handler_task(void *p) {
  unsigned char key_press;
  while (1) {
    if (xQueueReceive(keypad_char_queue, &key_press, portMAX_DELAY) && xSemaphoreTake(lcd_write_mutex, portMAX_DELAY)) {
      MP3_keypad__disable_interrupt();
      // printf("%c\n", key_press);
      MP3_menu__UI_handler(key_press);
      xSemaphoreGive(lcd_write_mutex);
    }
    MP3_keypad__enable_interrupt();
  }
}

static volatile bool suspend_rotate = false;
void suspend_rotate_task(void) { suspend_rotate = true; } // Need this function to prevent possible deadlock

static void mp3__rotate_string(void *p) {
  while (MP3_menu__LOGIN_get_LOGIN_status() != SUCCESS) {
    vTaskDelay(100);
  }
  while (1) {

    xSemaphoreTake(lcd_write_mutex, portMAX_DELAY);
    MP3_menu_SONG_LIST_rotate_string();
    xSemaphoreGive(lcd_write_mutex);
    if (suspend_rotate) {
      vTaskSuspend(NULL);
      suspend_rotate = false;
    }
    vTaskDelay(300);
  }
}

//========================== L O G I N ======================================

static void mp3__menu_login(void *p) {
  string48_t login_message = "PLEASE ENTER 4-DIGIT PIN NUMBER     ";
  string16_t login_message_display = {0};
  MP3_menu__UI_handler('X');
  while (MP3_menu__LOGIN_get_LOGIN_status() != SUCCESS) {
    strncpy(login_message_display, login_message, 16 - 1);
    lcd__write_string(login_message_display, LINE_1, 1, 0);
    rotate_string(login_message, LEFT);
    delay__ms(500);
  }
  vTaskSuspend(NULL);
}

//==========================================================
static void adc_setup(void) {
  adc__initialize();
  LPC_IOCON->P1_30 &= ~(0b111 | (1 << 7));
  LPC_IOCON->P1_30 |= (0b11);
}

void decoder_test(void) {
  MP3_decoder__init();
  MP3_decoder__set_volume(0x77, 0x77);
  printf("Testing Decoder...\n");
  // uint8_t n = 0;
  // while (1) {
  printf("0x%04X\n", sci_read(0xB));
  MP3_decoder__sine_test(126, 3000);
  delay__ms(500);
}

static void test_pop_file(void) {
  MP3_song__init();
  MP3_song__print();
}
void MP3_task__set_up(void) {
  adc_setup();
  MP3_menu__init();
  MP3_keypad__init();
  MP3_song__print();
  decoder_test();
  puts("");

  string16_t string = "All";
  uint16_t start_year = 2000, end_year = 2010;
  MP3_song_payload payload = {MP3_song_get_genre_options()[1], NULL, NULL};
  MP3_song__query_by_genre_and_year(&payload);
  MP3_song__print_genres();
  MP3_song__print_response();

  setvbuf(stdout, 0, _IONBF, 0);

  //========== Queue ==========
  song_name_queue = xQueueCreate(1, sizeof(songname_t));
  song_data_queue = xQueueCreate(2, sizeof(songdata_t));
  keypad_char_queue = xQueueCreate(1, sizeof(char));
  //========== Semaphore ======
  volume_handler_semaphore = xSemaphoreCreateBinary();

  //========== Mutex ==========
  lcd_write_mutex = xSemaphoreCreateMutex();

  // xTaskCreate(cli_sim_task, "cli", 1024, NULL, 1, NULL);
  xTaskCreate(mp3_file_reader_task, "reader", 2500 / sizeof(void *), NULL, 1, NULL);
  xTaskCreate(mp3_data_player_task, "player", 2048 / sizeof(void *), NULL, 2, NULL);
  xTaskCreate(mp3_adjust_volume, "volume", 1024 / sizeof(void *), NULL, 3, NULL);
  xTaskCreate(mp3__interrupt_handler_task, "mp3_interrupt", 4096 / sizeof(void *), NULL, 3, NULL);
  xTaskCreate(mp3__menu_login, "mp3_login", 1024 / sizeof(void *), NULL, 2, NULL);
  xTaskCreate(mp3__rotate_string, "rotate_str", 2048 / sizeof(void *), NULL, 3, NULL);
}