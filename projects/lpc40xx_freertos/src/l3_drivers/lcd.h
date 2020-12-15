#pragma once

#include "gpio.h"
#include "string_t.h"
#include <stdbool.h>
#include <stdint.h>

void lcd__init(void);

void lcd__test_lcd(void);

typedef enum { LINE_1 = 0, LINE_2 } LCD_LINE;

/*Parameters Description
 * 1st: Input string to write LCD (Type const char array)
 * 2nd: Input for placing string at specfic line (Type enum)
 * 3rd: Input for placing string at specific position (Type uint8_t, (unsigned char))
 * 4th: Provides delay before each upcoming char is written (Type uint16_t, (unsigned short))
 */
void lcd__write_string(const string16_t STRING, LCD_LINE line, uint8_t ADDRESS_offset, uint16_t DELAY_in_ms);
