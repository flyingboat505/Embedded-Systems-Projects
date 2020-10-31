
#include "uart_lab.h"
#include "FreeRTOS.h"
#include "lpc_peripherals.h"
#include "queue.h"
#include <stdbool.h>
#include <stdint.h>
static QueueHandle_t your_uart_rx_queue;

static void your_receive_interrupt(void) {
  uint32_t readValue = (LPC_UART3->IIR) & (0b111 << 1);
  readValue = readValue >> 1;
  // Read the IIR register to figure out why you got interrupted
  //  Based on IIR status, read the LSR register to confirm if there is data to be read
  if (readValue == 0x2) {
    // // Based on IIR status, read the LSR register to confirm if there is data to be read
    if (LPC_UART3->LSR & (1 << 0)) {
      // TODO: Based on LSR status, read the RBR register and input the data to the RX Queue
      const char byte = LPC_UART3->RBR;
      xQueueSendFromISR(your_uart_rx_queue, &byte, NULL);
    }
  }
}

// Public function to enable UART interrupt
// TODO Declare this at the header file
void uart__enable_receive_interrupt(uart_number_e uart_number) {
  // Use lpc_peripherals.h to attach your interrupt
  lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, &your_receive_interrupt, "Part2Interrupt");
  // Enable UART receive interrupt by reading the LPC User manual
  LPC_UART3->IER |= (1 << 0);
  // Create your RX queue
  your_uart_rx_queue = xQueueCreate(1, sizeof(char));
}

bool uart_lab__get_char_from_queue(char *input_byte, uint32_t timeout) {
  return xQueueReceive(your_uart_rx_queue, input_byte, timeout);
}

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  // choosing UART3
  // a) Power on Peripheral
  LPC_SC->PCONP |= (1 << (24 + uart));
  const uint16_t divider_16_bit = 96 * 1000 * 1000 / (16 * baud_rate);
  // b) Setup DLL, DLM, FDR, LCR registers
  // from lab code to simplify baud rate equation

  // enable divisor bit
  LPC_UART3->LCR |= (1 << 7);
  LPC_UART3->DLM = (divider_16_bit >> 8) & 0xFF;
  LPC_UART3->DLL = (divider_16_bit >> 0) & 0xFF;

  LPC_UART3->FDR |= (1 << 4); // Fract Div Register=4
  LPC_UART3->FCR |= (1 << 4); // control register- DMA mode

  // disable divisor bit
  LPC_UART3->LCR &= ~(1 << 7);
  // WLS = 8 bit char length
  LPC_UART3->LCR |= 0b111;
}

// Read the byte from RBR and actually save it to the pointer
bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  // a) Check LSR for Receive Data Ready
  // b) Copy data from RBR register to input_byte
  if (!(LPC_SC->PCONP & (1 << (24 + uart)))) {
    printf("false get \n");
    return false;
  } else {
    while (true) {
      if (LPC_UART3->LSR & (1 << 0)) {
        *input_byte = LPC_UART3->RBR;
        return true;
      }
    }
  }
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  // a) Check LSR for Transmit Hold Register Empty
  if (!(LPC_SC->PCONP & (1 << (24 + uart)))) {
    printf("false put \n");
    return false;
  } else {
    // b) Copy output_byte to THR register
    while (true) {
      if (LPC_UART3->LSR & (1 << 5)) {
        LPC_UART3->THR = output_byte;
        return true;
      }
    }
  }
}