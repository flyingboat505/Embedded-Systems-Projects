#include "lcd.h"
#include "delay.h"
#include <stdio.h>

#define DATA_BITS 4

typedef struct {
  gpio_s DATA[DATA_BITS];
  gpio_s RS;
  gpio_s EN;
} LCD_PINS_Typedef;

static const LCD_PINS_Typedef LCD_PINS = {
    {(gpio_s){0, 17}, (gpio_s){0, 16}, (gpio_s){2, 8}, (gpio_s){2, 6}}, (gpio_s){0, 0}, (gpio_s){0, 22}};

/*================================================
 *    P R I V A T E   F U N C T I O N S
 *================================================
 */

static void lcd__enable(void);
static void lcd__disable(void);

static void lcd__select_instr_reg(void);
static void lcd__select_data_reg(void);

static void lcd__transmit_data(uint8_t);

static void lcd__send_instr_code(uint8_t);
static void lcd__write_char(uint8_t);

static void lcd__set_4_bitmode(void);

/*================================================
 *      P U B L I C   F U N C T I O N S
 *================================================
 */
#define ENABLE_BLINKING 0 // Change this to non-zero to enable blinking
#define DISPLAY_CURSOR 0  // Change this to non-zero to display cursor

// Reference: http://web.alfredstate.edu/faculty/weimandn/lcd/lcd_initialization/lcd_initialization_index.html
void lcd__init() {
  // config_as_GPIO();
  gpio__set_as_output(LCD_PINS.EN);
  gpio__set_as_output(LCD_PINS.RS);
  lcd__select_instr_reg();

  lcd__set_4_bitmode();

  uint8_t DISPLAY_mask = (1 << 2), CURSOR_mask = (1 << 1), BLINK_mask = (1 << 0);

  uint8_t FUNCTION_SET = 0b0010;
  uint8_t SET_2_LINE_DISPLAY = (FUNCTION_SET << 4) | (1 << 3);
  uint8_t CLEAR_ON_OFF = (1 << 3) & ~(DISPLAY_mask | CURSOR_mask | BLINK_mask);
  uint8_t SET_CURSOR_HOME = (1 << 0);
  uint8_t SET_ENTRY_MODE = (0b11 << 1);
  uint8_t DISPLAY_ON_OFF = (1 << 3) | (DISPLAY_mask | CURSOR_mask | BLINK_mask);

#if !(ENABLE_BLINKING)
  DISPLAY_ON_OFF &= ~BLINK_mask;
#endif
#if !(DISPLAY_CURSOR)
  DISPLAY_ON_OFF &= ~CURSOR_mask;
#endif

  lcd__send_instr_code(SET_2_LINE_DISPLAY);
  lcd__send_instr_code(CLEAR_ON_OFF);
  lcd__send_instr_code(SET_CURSOR_HOME);
  lcd__send_instr_code(SET_ENTRY_MODE);

  //---------------Initialization Ends---------------------
  lcd__send_instr_code(DISPLAY_ON_OFF);
}
void lcd__write_string(const string16_t STRING, LCD_LINE line, uint8_t ADDRESS_offset, uint16_t DELAY_in_ms) {
  uint8_t SET_DRAM_ADDRESS_INSTR = (1 << 7);
  uint8_t FIRST_LINE_START_ADDRESS = 0x00, SECOND_LINE_START_ADDRESS = 0x40;

  uint8_t INSTR_CODE = (line) ? (SET_DRAM_ADDRESS_INSTR | SECOND_LINE_START_ADDRESS)
                              : (SET_DRAM_ADDRESS_INSTR | FIRST_LINE_START_ADDRESS);

  lcd__send_instr_code(INSTR_CODE + ADDRESS_offset);

  for (uint8_t INDEX = 0; INDEX < (sizeof(string16_t) - 1 - ADDRESS_offset) && STRING[INDEX] != '\0'; INDEX++) {
    if (DELAY_in_ms) // Don't call this function if delay is 0. Can cause minor delays and it's kinda noticable
      delay__ms(DELAY_in_ms);
    lcd__write_char(STRING[INDEX]);
  }
}

//===============PRIVATE FUNCTION DEFINITIONS======================
// These delays are require to make the LCD working because it requires time delay to process the data
#define INSTR_DELAY_in_us 1000
#define DATA_DELAY_in_us 40

#define rightArrow 0x7E
#define leftArrow 0x7F

static void lcd__enable(void) { gpio__set(LCD_PINS.EN); }
static void lcd__disable(void) { gpio__reset(LCD_PINS.EN); }

static void lcd__select_instr_reg(void) { gpio__reset(LCD_PINS.RS); }
static void lcd__select_data_reg(void) { gpio__set(LCD_PINS.RS); }

static void lcd__transmit_data(uint8_t DATA) {
  for (uint8_t INDEX = 0; INDEX < DATA_BITS; INDEX++, DATA >>= 1) {
    gpio__set_as_output(LCD_PINS.DATA[INDEX]);
    if (DATA & 1)
      gpio__set(LCD_PINS.DATA[INDEX]);
    else
      gpio__reset(LCD_PINS.DATA[INDEX]);
  }
  lcd__enable();
  lcd__disable();
}

static void lcd__send_instr_code(uint8_t INSTR_CODE) {
  lcd__select_instr_reg();
  lcd__transmit_data((INSTR_CODE >> 4) & 0xF);
  lcd__transmit_data((INSTR_CODE >> 0) & 0xF);
  delay__us(INSTR_DELAY_in_us);
}

static void replace_lessthan_greaterthan_to_arrow(uint8_t *CHAR) {
  if (*CHAR == '<')
    *CHAR = leftArrow;
  else if (*CHAR == '>')
    *CHAR = rightArrow;
}

static void lcd__write_char(uint8_t CHAR) {
  lcd__select_data_reg();
  replace_lessthan_greaterthan_to_arrow(&CHAR); // char replacement ONLY APPLIES to char '<' and '>'
  lcd__transmit_data((CHAR >> 4) & 0xF);
  lcd__transmit_data((CHAR >> 0) & 0xF);
  delay__us(DATA_DELAY_in_us);
}

static void lcd__set_4_bitmode(void) {
  uint8_t FUNCTION_SET = 0b0011;
  uint8_t SET_4_BIT_MODE = 0b0010;

  delay__us(INSTR_DELAY_in_us);
  lcd__transmit_data(FUNCTION_SET);
  delay__us(INSTR_DELAY_in_us);
  lcd__transmit_data(FUNCTION_SET);
  delay__us(INSTR_DELAY_in_us);
  lcd__transmit_data(FUNCTION_SET);
  delay__us(INSTR_DELAY_in_us);
  lcd__transmit_data(SET_4_BIT_MODE);
}

static void config_as_GPIO(void) { // I don't think we need this...
  gpio__set_function(LCD_PINS.EN, 0);
  gpio__set_function(LCD_PINS.RS, 0);
  for (uint8_t i = 0; i < 4; i++) {
    gpio__set_function(LCD_PINS.DATA[i], 0);
  }
}
//============END OF PRIVATE FUNCTION DEFINITIONS==================