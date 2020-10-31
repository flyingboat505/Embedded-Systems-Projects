#include "gpio_isr.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdio.h>

static function_pointer_t gpio0_callbacks[32];

void gpio0__attach_interrupt(uint32_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  if (interrupt_type == GPIO_INTR__RISING_EDGE)
    LPC_GPIOINT->IO0IntEnR |= (1 << pin);
  else
    LPC_GPIOINT->IO0IntEnF |= (1 << pin);
  gpio0_callbacks[pin] = callback;
  int x = LPC_GPIOINT->IO0IntEnF;
  int y = LPC_GPIOINT->IO0IntEnR;

  fprintf(stderr, "GPIO REGIS Fall %x \n", x);
  fprintf(stderr, "GPIO REGIS Rise %x \n", y);

  // 1) Store the callback based on the pin at gpio0_callbacks
  // 2) Configure GPIO 0 pin for rising or falling edge
}

int logic_that_you_will_write() {
  uint32_t val = LPC_GPIOINT->IO0IntStatF;
  int count = -1;
  if (val != 0) {
    for (int i = 0; i < 32; i++) {
      if (val == 0) {
        break;
      } else {
        val = val >> 1;
        count++;
      }
    }
    if (count != -1)
      return count;
  } else {
    val = LPC_GPIOINT->IO0IntStatR;
    count = -1;
    for (int i = 0; i < 32; i++) {
      if (!val) {
        return count;
      } else {
        val = val >> 1;
        count++;
      }
    }
  }
  return count;
}

void gpio0__interrupt_dispatcher(void) {
  const int pin_that_generated_interrupt = logic_that_you_will_write();
  if (pin_that_generated_interrupt == -1) {
    fprintf(stderr, "fail -wrong input");
    return;
  }

  function_pointer_t attached_user_handler = gpio0_callbacks[pin_that_generated_interrupt];
  attached_user_handler();
  LPC_GPIOINT->IO0IntClr = (1 << pin_that_generated_interrupt);
}

/*void gpio0__interrupt_dispatcher(void) {
  const int pin_that_generated_interrupt = logic_that_you_will_write();


  int x = LPC_GPIOINT->IO0IntEnF;
  int y = LPC_GPIOINT->IO0IntEnR;
  fprintf(stderr, "pin_that_generated_interrupt %d \n", pin_that_generated_interrupt);
  fprintf(stderr, "Resgister Fall %d \n", x);
  fprintf(stderr, "Resgister Rise %d \n", y);

  if (pin_that_generated_interrupt == -1) {
    fprintf(stderr, "fail -wrong input");
    return;
  }

  function_pointer_t attached_user_handler = gpio0_callbacks[pin_that_generated_interrupt];
  attached_user_handler();
  x = LPC_GPIOINT->IO0IntEnF;
  y = LPC_GPIOINT->IO0IntEnR;
  int z = LPC_GPIOINT->IO0IntClr;
  LPC_GPIOINT->IO0IntClr = (1U << pin_that_generated_interrupt);
  fprintf(stderr, " New Resgister Fall %d \n", x);
  fprintf(stderr, " New Resgister Rise %d \n ", y);
  fprintf(stderr, " Clear Register  %d \n \n", z);
}*/
