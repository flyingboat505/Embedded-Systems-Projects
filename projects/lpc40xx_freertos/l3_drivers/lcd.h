#pragma once

#include "gpio.h"
#include <stdbool.h>
#include <stdint.h>

typedef char string16_t[16 + 1]; // Accommodate 1 space for NULL termination

void lcd__init(void);

typedef enum { LINE_1 = 0, LINE_2 } LCD_LINE;

/*Parameters Description
 * 1st: Input string to write LCD (Type const char array)
 * 2nd: Input for placing string at specfic line (Type enum)
 * 3rd: Input for placing string at specific position (Type uint8_t (unsigned char))
 * 4th: Input delay for each char is written (Type uint16_t, (unsigned short))
 */
void lcd__write_string(const string16_t STRING, LCD_LINE line, uint8_t ADDRESS_offset, uint16_t DELAY_in_ms);
