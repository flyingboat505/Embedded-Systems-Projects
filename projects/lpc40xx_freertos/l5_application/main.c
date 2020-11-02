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
#include <string.h>

// testing
static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

QueueHandle_t song_name_queue;
static QueueHandle_t song_data_queue;

// typedef enum { switch__off, switch__on } switch_e;
typedef char songname_t[256];
typedef char songdata_t[512 / 2];

// make into cli
#define our_songname "Major Lazer  Too Original feat Elliphant  Jovi Rockwell Official Lyric Video.mp3"
#define test_sample_mp3_file "Nature Beautiful short video 720p HD.mp3"
/*
static void cli_sim_task(void *p) {
  //songname_t filename = "README.md"; // Will change name
  songname_t songname = {0};
  strncpy(songname, filename, sizeof(songname) - 1);
  printf("SONGNAME: %s\n", songname);
  if (xQueueSend(song_name_queue, &songname, 0)) {
    puts("SUCESS: SONGNAME WAS SENT TO THE QUEUE");
  } else {
    puts("FAILED: SONGNAME WAS NOT SENT TO THE QUEUE");
  }

  vTaskSuspend(NULL);
}*/

uint8_t pause = 0;

static void read_file(const char *filename) {
  printf("Request received to play/read: '%s'\n", filename);
  FIL file;
  if (!(f_open(&file, filename, FA_OPEN_ALWAYS | FA_READ) == FR_OK)) {
  } else {
    printf("FILE OPENED: file size: %i\n", f_size(&file));
    songdata_t buffer = {0}; // zero initialize
    UINT bytes_read = 0;

    while (!f_eof(&file)) {
      if (FR_OK == f_read(&file, buffer, sizeof(buffer) - 1, &bytes_read)) {
        // printf("Block size: %i\n", bytes_read);
        xQueueSend(song_data_queue, buffer, portMAX_DELAY);
      } else
        puts("ERROR: Failed to read file");
      memset(&buffer[0], 0, sizeof(buffer)); // Write NULLs to all 256 bytes
    }
    printf("Here\n");
    f_close(&file);
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
  for (size_t index = 0; index < sizeof(songdata_t); index++) {
    /* Real code for your SJ2
     * if (mp3_decoder_gpio_is_high) {
     *   spi_exchange(data[index]);
     * } else {
     *   vTaskDelay(1);
     * }
     */
    vTaskDelay(50);
    putchar(data[index]);
    // printf("0x%02X ", data[index]);
  }
  // printf("\n");
}

static void mp3_data_player_task(void *p) {
  songdata_t songdata;

  while (1) {
    memset(&songdata[0], 0, sizeof(songdata_t));
    if (xQueueReceive(song_data_queue, &songdata[0], portMAX_DELAY)) {
      mp3_decoder_send_block(songdata);
    }
}

//==========================================================

int main(void) {
  create_blinky_tasks();
  create_uart_task();

  setvbuf(stdout, 0, _IONBF, 0);
  song_name_queue = xQueueCreate(1, sizeof(songname_t));
  song_data_queue = xQueueCreate(2, sizeof(songdata_t));
  pause = 0;
  puts("Starting RTOS");
  // xTaskCreate(cli_sim_task, "cli", 1024, NULL, 1, NULL);
  xTaskCreate(mp3_file_reader_task, "reader", 1024, NULL, 1, NULL);
  xTaskCreate(mp3_data_player_task, "player", 1024, NULL, 2, NULL);

  vTaskStartScheduler(); // Ths function never returns unless RTOS scheduler runs out of memory and fails
  return 0;
}

static void create_blinky_tasks(void) {
/**
 * Use '#if (1)' if you wish to observe how two tasks can blink LEDs
 * Use '#if (0)' if you wish to use the 'periodic_scheduler.h' that will spawn 4 periodic tasks, one for each LED
 */
#if (1)
  // These variables should not go out of scope because the 'blink_task' will reference this memory
  static gpio_s led0, led1;

  led0 = board_io__get_led0();
  led1 = board_io__get_led1();

  xTaskCreate(blink_task, "led0", configMINIMAL_STACK_SIZE, (void *)&led0, PRIORITY_LOW, NULL);
  xTaskCreate(blink_task, "led1", configMINIMAL_STACK_SIZE, (void *)&led1, PRIORITY_LOW, NULL);
#else
  const bool run_1000hz = true;
  const size_t stack_size_bytes = 2048 / sizeof(void *); // RTOS stack size is in terms of 32-bits for ARM M4 32-bit CPU
  periodic_scheduler__initialize(stack_size_bytes,
                                 !run_1000hz); // Assuming we do not need the high rate 1000Hz task
  UNUSED(blink_task);
#endif
}

static void create_uart_task(void) {
  // It is advised to either run the uart_task, or the SJ2 command-line (CLI), but not both
  // Change '#if (0)' to '#if (1)' and vice versa to try it out
#if (0)
  // printf() takes more stack space, size this tasks' stack higher
  xTaskCreate(uart_task, "uart", (512U * 8) / sizeof(void *), NULL, PRIORITY_LOW, NULL);
#else
  sj2_cli__init();
  UNUSED(uart_task); // uart_task is un-used in if we are doing cli init()
#endif
}

static void blink_task(void *params) {
  const gpio_s led = *((gpio_s *)params); // Parameter was input while calling xTaskCreate()

  // Warning: This task starts with very minimal stack, so do not use printf() API here to avoid stack overflow
  while (true) {
    gpio__toggle(led);
    vTaskDelay(500);
  }
}

// This sends periodic messages over printf() which uses system_calls.c to send them to UART0
static void uart_task(void *params) {
  TickType_t previous_tick = 0;
  TickType_t ticks = 0;

  while (true) {
    // This loop will repeat at precise task delay, even if the logic below takes variable amount of ticks
    vTaskDelayUntil(&previous_tick, 2000);

    /* Calls to fprintf(stderr, ...) uses polled UART driver, so this entire output will be fully
     * sent out before this function returns. See system_calls.c for actual implementation.
     *
     * Use this style print for:
     *  - Interrupts because you cannot use printf() inside an ISR
     *    This is because regular printf() leads down to xQueueSend() that might block
     *    but you cannot block inside an ISR hence the system might crash
     *  - During debugging in case system crashes before all output of printf() is sent
     */
    ticks = xTaskGetTickCount();
    fprintf(stderr, "%u: This is a polled version of printf used for debugging ... finished in", (unsigned)ticks);
    fprintf(stderr, " %lu ticks\n", (xTaskGetTickCount() - ticks));

    /* This deposits data to an outgoing queue and doesn't block the CPU
     * Data will be sent later, but this function would return earlier
     */
    ticks = xTaskGetTickCount();
    printf("This is a more efficient printf ... finished in");
    printf(" %lu ticks\n\n", (xTaskGetTickCount() - ticks));
  }
}
