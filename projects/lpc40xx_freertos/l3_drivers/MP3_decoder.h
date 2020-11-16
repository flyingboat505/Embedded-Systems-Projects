#pragma once
#include "gpio.h"
#include <stddef.h>
#include <stdint.h>

void MP3_decoder__init(void);

void MP3_decoder__send_data(uint8_t *data, size_t BYTE);

void MP3_decoder__set_volume(uint8_t left, uint8_t right);

void MP3_decoder__sine_test(uint8_t n, uint16_t delay_in_ms);

uint16_t sci_read(uint8_t address);