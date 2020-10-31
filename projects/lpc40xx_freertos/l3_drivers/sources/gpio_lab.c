// file gpio_lab.h

#include "lpc40xx.h"
#include <gpio_lab.h>
#include <stdbool.h>
#include <stdint.h>
// include this file at gpio_lab.c file
// #include "lpc40xx.h"

// NOTE: The IOCON is not part of this driver

/// Should alter the hardware registers to set the pin as input
/// pass a pin
void gpio1__set_as_input(uint8_t pin_num) {
  // set an Assert for invalid ports-for GPIO 0 in the future for now-assume that the user will pass right ports
  LPC_GPIO1->DIR &= ~(1 << pin_num);
}

/// Should alter the hardware registers to set the pin as output
void gpio1__set_as_output(uint8_t pin_num) { LPC_GPIO1->DIR |= (1U << pin_num); } ////CHANGE BACK

/// Should alter the hardware registers to set the pin as high
void gpio1__set_high(uint8_t pin_num) {
  LPC_GPIO1->SET = (1U << pin_num);
  // LPC_GPIO0->PIN |= 1 << pin_num;
}

/// Should alter the hardware registers to set the pin as low
void gpio1__set_low(uint8_t pin_num) {
  LPC_GPIO1->CLR = (1U << pin_num);
  // LPC_GPIO0->PIN &= ~(1 << pin_num);
}

/**
 * Should alter the hardware registers to set the pin as low
 *
 * @param {bool} high - true => set pin high, false => set pin low
 */
void gpio1__set(uint8_t pin_num, bool high) {
  if (high)
    gpio1__set_high(pin_num);
  else
    gpio1__set_low(pin_num);
}

/**
 * Should return the state of the pin (input or output, doesn't matter)
 *
 * @return {bool} level of pin high => true, low => false
 */
bool gpio1__get_level(uint8_t pin_num) {
  if (LPC_GPIO1->PIN & (1 << pin_num))
    return true;
  else
    return false;
}