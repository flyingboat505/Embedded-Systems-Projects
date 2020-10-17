#include <stdio.h>

#include "FreeRTOS.h"
#include "adc.h"
#include "board_io.h"

#include "acceleration.h"
#include "common_macros.h"
#include "delay.h"
#include "ff.h"
#include "gpio.h"
#include "gpio_isr.h"
#include "gpio_lab.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "periodic_scheduler.h"
#include "pwm1.h"
#include "queue.h"
#include "semphr.h"
#include "sj2_cli.h"
#include "task.h"
#include "uart_lab.h"
#include <string.h>

static void create_blinky_tasks(void);
static void create_uart_task(void);
static void blink_task(void *params);
static void uart_task(void *params);

static QueueHandle_t sensor_queue;

typedef enum { switch__off, switch__on } switch_e;

// testing git

void write_file_using_fatfs_pi(acceleration__axis_data_s value) {
  const char *filename = "sensor.txt";
  FIL file; // File handle
  UINT bytes_written = 0;
  FRESULT result = f_open(&file, filename, (FA_WRITE | FA_CREATE_ALWAYS));

  if (FR_OK == result) {
    char string[64];
    sprintf(string, "x=%i y=%i z=%i \n", value.x, value.y, value.z);
    if (FR_OK == f_write(&file, string, strlen(string), &bytes_written)) {
    } else {
      printf("ERROR: Failed to write data to file\n");
    }
    f_close(&file);
  } else {
    printf("ERROR: Failed to open: %s\n", filename);
  }
}

// TODO: Create this task at PRIORITY_LOW
void producer(void *p) {
  acceleration__axis_data_s value;
  int count = 0, x_total = 0, y_total = 0, z_total = 0;
  while (count < 100) {
    value = acceleration__get_data();
    x_total += value.x;
    y_total += value.y;
    z_total += value.z;
    count++;
  }

  value.x = x_total / 100;
  value.y = y_total / 100;
  value.z = z_total / 100;
  if (xQueueSend(sensor_queue, &value, portMAX_DELAY)) // double check
    fprintf(stderr, "Sent Values x=%i y=%i z=%i \n", value.x, value.y, value.z);
  else
    fprintf(stderr, "Failed to send value  \n");
}

// TODO: Create this task at PRIORITY_HIGH
void consumer(void *p) {
  acceleration__axis_data_s value;
  while (1) {
    fprintf(stderr, "Attempting to recieve value \n");
    if (xQueueReceive(sensor_queue, &value, portMAX_DELAY)) {
      fprintf(stderr, "Received Values  x=%i y=%i z=%i \n", value.x, value.y, value.z);
      // write_file_using_fatfs_pi(value);
    } else
      fprintf(stderr, "Failed to Received value  \n");
  }
}

int main(void) {

  // create_blinky_tasks();
  create_uart_task();
  acceleration__init();
  sensor_queue = xQueueCreate(1, sizeof(acceleration__axis_data_s));
  xTaskCreate(producer, "producer", 2048 / sizeof(void *), NULL, 2, NULL);
  xTaskCreate(consumer, "consumer", 2048 / sizeof(void *), NULL, 2, NULL);
  vTaskStartScheduler();
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
