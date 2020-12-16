#include "MP3_decoder.h"
#include "delay.h"
#include "ssp2.h"
#include <stdio.h>

// Reference:
// https://learn.adafruit.com/adafruit-vs1053-mp3-aac-ogg-midi-wav-play-and-record-codec-tutorial/downloads-and-links

// Note decoder requires 7 pins
static const gpio_s decoder_SCLK = {1, 0};
static const gpio_s decoder_MOSI = {1, 1};
static const gpio_s decoder_MISO = {1, 4};

static const gpio_s decoder_reset = {0, 8};
static const gpio_s decoder_CS = {0, 26};
static const gpio_s decoder_XDCS = {1, 31};
static const gpio_s decoder_DREQ = {1, 20};
/*
static const gpio_s decoder_reset = {1, 20};
static const gpio_s decoder_CS = {1, 28};
static const gpio_s decoder_XDCS = {2, 0};
static const gpio_s decoder_DREQ = {2, 2};
*/

static void MP3_decoder__enable_XDCS(void) { gpio__reset(decoder_XDCS); }

static void MP3_decoder__disable_XDCS(void) { gpio__set(decoder_XDCS); }

static void MP3_decoder__enable_CS(void) { gpio__reset(decoder_CS); }

static void MP3_decoder__disable_CS(void) { gpio__set(decoder_CS); }

#define VS1053_REG_MODE 0x0
#define VS1053_MODE_SM_SDINEW 0x0800
#define VS1053_MODE_SM_RESET 0x0004
#define VS1053_REG_VOLUME 0x0B // pg 31, expression
#define VS1053_REG_BASS 0x2    // pg 37
#define VS1053_REG_TREMBLE 0x2 // pg 37

static uint16_t MP3_decoder__sci_read(uint8_t address) {
  uint16_t data = 0;
  MP3_decoder__enable_CS();
  ssp2__exchange_byte(0x3);
  ssp2__exchange_byte(address);
  data = (ssp2__exchange_byte(0xFF) << 8) | (ssp2__exchange_byte(0xFF));
  MP3_decoder__disable_CS();
  return data;
}

// Public Function For Debugging.
uint16_t sci_read(uint8_t address) { return MP3_decoder__sci_read(address); }

// 0x2 opcode for sci_write
static void MP3_decoder__sci_write(uint8_t address, uint16_t data) {
  MP3_decoder__enable_CS();
  ssp2__exchange_byte(0x2);
  ssp2__exchange_byte(address);
  ssp2__exchange_byte((data >> 8) & 0xFF);
  ssp2__exchange_byte((data >> 0) & 0xFF);
  MP3_decoder__disable_CS();
}

void MP3_decoder__set_bass(uint8_t data) {
  // for tremble use ST_AMPLITUDE and uses bits 15:12, Is activated when it is non-zero
  // for bass use SB_AMPLITUDE and use bits 7:4, set to user's prefernces,
  // SB_FREQLIMIT should be set 1.5times the lowest frequency the user's audio reproduces
  uint16_t bass_temp = MP3_decoder__sci_read(VS1053_REG_BASS);
  bass_temp &= ~(0xF << 4); // to get bits 7:4 to be zeros
  MP3_decoder__sci_write(VS1053_REG_BASS, bass_temp | (data << 4));
}

void MP3_decoder__set_tremble(uint8_t data) {
  uint16_t tremble_temp = MP3_decoder__sci_read(VS1053_REG_BASS);
  tremble_temp &= ~(0xF << 12); // to get bits 15:12 to be zeros
  MP3_decoder__sci_write(VS1053_REG_TREMBLE, tremble_temp | (data << 12));
}

static void MP3_decoder__softreset(void) {
  MP3_decoder__sci_write(VS1053_REG_MODE, VS1053_MODE_SM_SDINEW | VS1053_MODE_SM_RESET);
  delay__ms(100);
}

static void MP3_decoder__reset(void) {
  if (!gpio__get(decoder_reset)) {
    gpio__reset(decoder_reset);
    delay__ms(100);
    gpio__set(decoder_reset);
  }
  gpio__set(decoder_CS);
  gpio__set(decoder_XDCS);
  delay__ms(100);
  MP3_decoder__softreset();
  delay__ms(100);

  MP3_decoder__sci_write(0x3, 0x6000);
  // MP3_decoder__set_volume(0, 0);
}

void MP3_decoder__init(void) { // Will add pin number in these parameter later...
                               // Need to set clk, miso, mosi to function SSP2
  gpio__set_function(decoder_SCLK, GPIO__FUNCTION_4);
  gpio__set_function(decoder_MISO, GPIO__FUNCTION_4);
  gpio__set_function(decoder_MOSI, GPIO__FUNCTION_4);

  gpio__set_function(decoder_reset, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_function(decoder_CS, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_function(decoder_XDCS, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_function(decoder_DREQ, GPIO__FUNCITON_0_IO_PIN);

  gpio__set_as_output(decoder_reset);
  gpio__set_as_output(decoder_CS);
  gpio__set_as_output(decoder_XDCS);
  gpio__set_as_input(decoder_DREQ);

  ssp2__initialize(1000); // 1khz?
  MP3_decoder__reset();
  MP3_decoder__sci_write(0x02, 0x050F);
}
void MP3_decoder__set_volume(uint8_t left, uint8_t right) {
  MP3_decoder__sci_write(VS1053_REG_VOLUME, (left << 8) | right);
}

void MP3_decoder__send_data(uint8_t *data, size_t BYTE) {
  // MP3_decoder__reset();
  MP3_decoder__enable_XDCS();
  while (!gpio__get(decoder_DREQ)) {
    ;
  }
  for (size_t index = 0; index < BYTE; index++) {
    ssp2__exchange_byte(data[index]); // Need to change
  }
  MP3_decoder__disable_XDCS();
}

void MP3_decoder__sine_test(uint8_t n, uint16_t delay_in_ms) {
  MP3_decoder__reset();
  uint16_t mode = MP3_decoder__sci_read(VS1053_REG_MODE);
  mode |= 0x20;
  MP3_decoder__sci_write(VS1053_REG_MODE, mode);

  while (!gpio__get(decoder_DREQ)) {
    ;
  }

  MP3_decoder__enable_XDCS();
  // delay__ms(10);
  ssp2__exchange_byte(0x53);
  ssp2__exchange_byte(0xEF);
  ssp2__exchange_byte(0x6E);
  ssp2__exchange_byte(n);
  for (uint8_t iteration = 0; iteration < 4; iteration++)
    ssp2__exchange_byte(0x0);
  /*ssp2__exchange_byte(0x00);
  ssp2__exchange_byte(0x00);
  ssp2__exchange_byte(0x00);
  ssp2__exchange_byte(0x00);*/
  // delay__ms(1000);
  MP3_decoder__disable_XDCS();

  delay__ms(delay_in_ms);

  MP3_decoder__enable_XDCS();
  // delay__ms(10);
  ssp2__exchange_byte(0x45);
  ssp2__exchange_byte(0x78);
  ssp2__exchange_byte(0x69);
  ssp2__exchange_byte(0x74);

  for (uint8_t iteration = 0; iteration < 4; iteration++)
    ssp2__exchange_byte(0x0);
  /*ssp2__exchange_byte(0x00);
  ssp2__exchange_byte(0x00);
  ssp2__exchange_byte(0x00);
  ssp2__exchange_byte(0x00);*/
  // delay__ms(1000);
  MP3_decoder__disable_XDCS();
}
