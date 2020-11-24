#pragma once
#include "gpio.h"
#include <stdint.h>
#include <stdio.h>

typedef uint8_t ROW_COL; // last 4 bits: row, first 4 bits: col

void MP3_keypad__init(void);

void MP3_keypad__enable_interrupt(void);

void MP3_keypad__disable_interrupt(void);