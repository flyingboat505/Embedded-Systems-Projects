#include "MP3_keypad.h"
#include "FreeRTOS.h"
#include "delay.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "queue.h"
#include <stdbool.h>

#define keypad_size 4
static const gpio_s keypad_row[keypad_size] = {{2, 0}, {2, 2}, {2, 5}, {2, 7}};
static const gpio_s keypad_col[keypad_size] = {{2, 9}, {0, 15}, {0, 18}, {0, 1}};
static const gpio_s keypad_int = {0, 25}; // Interupt Pin

typedef const char keypad_map[4][4];
static keypad_map MAP = {{'1', '2', '3', 'B'}, {'4', '5', '6', '^'}, {'7', '8', '9', 'V'}, {'<', '0', '>', 'E'}};

extern QueueHandle_t keypad_char_queue;

static uint8_t MP3_keypad_read_col(void) {
  uint8_t data = 0;
  for (int8_t INDEX = keypad_size - 1; INDEX >= 0; INDEX--) {
    data <<= 1;
    if (gpio__get(keypad_col[INDEX]))
      data += 1;
  }
  return data;
}

static void MP3_keypad_write_row(uint8_t data) {
  for (size_t INDEX = 0; INDEX < keypad_size; INDEX++, data >>= 1) {
    if (data & 1)
      gpio__set(keypad_row[INDEX]);
    else
      gpio__reset(keypad_row[INDEX]);
  }
}

static char MP3_keypad_decode(uint8_t row_col) {
  uint8_t row_col_encode[2] = {row_col >> 4, row_col & 0xF};
  uint8_t row_col_decode[2] = {0};
  for (size_t INDEX = 0; INDEX < 2; INDEX++) {
    while (row_col_encode[INDEX] & 0xF) {
      row_col_decode[INDEX]++;
      row_col_encode[INDEX] >>= 1;
    }
  }
  return MAP[row_col_decode[0] - 1][row_col_decode[1] - 1];
}

static void MP3_keypad_poll(void) {
  uint8_t row = 0b0001, col;
  bool char_found = false;
  while (row & 0xF) {
    MP3_keypad_write_row(row);
    col = MP3_keypad_read_col() & 0xF;
    if (col) {
      char_found = true;
      break;
    }
    row <<= 1;
  }
  // fprintf(stderr, "Row: %d, Col %d\n", row, col);
  if (char_found) {
    char key = MP3_keypad_decode((row << 4) | (col & 0xF));
    xQueueSendFromISR(keypad_char_queue, &key, NULL);
  }
}

static void MP3_refresh__interrupt(void) { // Refresh it by setting row pins to high
  MP3_keypad_write_row(0b1111);
}

static void MP3_keypad__ISR(void) {
  MP3_keypad_poll();
  MP3_refresh__interrupt();
  LPC_GPIOINT->IO0IntClr |= (1 << (keypad_int.pin_number));
}

static void configure_pin_as_GPIO(void) {
  for (size_t INDEX = 0; INDEX < keypad_size; INDEX++) {
    gpio__set_function(keypad_row[INDEX], 0);
    gpio__set_function(keypad_col[INDEX], 0);
  }
  gpio__set_function(keypad_int, 0);
}

void MP3_keypad__init(void) {
  configure_pin_as_GPIO();
  for (size_t INDEX = 0; INDEX < keypad_size; INDEX++) {
    gpio__set_as_output(keypad_row[INDEX]);
    gpio__set_as_input(keypad_col[INDEX]);
  }
  gpio__set_as_input(keypad_int);
  MP3_refresh__interrupt();

  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__GPIO, MP3_keypad__ISR, "Keypad_Interrupt");
  LPC_GPIOINT->IO0IntEnR |= (1 << (keypad_int.pin_number));
  NVIC_EnableIRQ(GPIO_IRQn);

  printf("keypad ready\n");
  /*
    while (1) {
      printf("Col: 0x%01X\n", read_col());
      delay__ms(500);
    }*/
}