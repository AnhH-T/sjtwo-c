#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include "queue.h"
#include "uart_lab.h"

// Private queue handle of our uart_lab.c
static QueueHandle_t your_uart2_rx_queue;
static QueueHandle_t your_uart3_rx_queue;

// Private function of our uart_lab.c
static void your_receive_interrupt2(void) {
  char byte = 'x'; // Rough error checking method
  // Read the IIR register: Confirm the interrupt is "Receive Data Available (RDA)"
  if (LPC_UART2->IIR & (2 << 1)) {
    // Based on IIR status, read the LSR register to confirm if there is data to be read
    // First pin checks if 0: receiver FIFO is empty, 1: receiver FIFO is not empty
    if (LPC_UART2->LSR & (1 << 0)) { // Checks if receiver FIFO isn't empty
      byte = LPC_UART2->RBR;         // take data from receiver FIFO
    }
  }

  if (byte == 'x') {                                   // if byte didn't change, it didn't read anything
    fprintf(stderr, "you should never be here!!!!\n"); // print some error msg
  } else                                               // If byte changed, send it into the queue
    xQueueSendFromISR(your_uart2_rx_queue, &byte, NULL);
}

static void your_receive_interrupt3(void) {
  char byte = 'x'; // Rough error checking method
  // Read the IIR register: Confirm the interrupt is "Receive Data Available (RDA)"
  if (LPC_UART3->IIR & (2 << 1)) {
    // Based on IIR status, read the LSR register to confirm if there is data to be read
    // First pin checks if 0: receiver FIFO is empty, 1: receiver FIFO is not empty
    if (LPC_UART3->LSR & (1 << 0)) { // Checks if receiver FIFO isn't empty
      byte = LPC_UART3->RBR;         // take data from receiver FIFO
    }
  }

  if (byte == 'x') {                                   // if byte didn't change, it didn't read anything
    fprintf(stderr, "you should never be here!!!!\n"); // print some error msg
  } else                                               // If byte changed, send it into the queue
    xQueueSendFromISR(your_uart3_rx_queue, &byte, NULL);
}

// Public function to enable UART interrupt
void uart__enable_receive_interrupt(uart_number_e uart_number) {
  // Const vars for ease of code reading
  const uint8_t enable_receive_interrupt = (1 << 0);
  const uint8_t DLAB = (1 << 7);

  if (uart_number == UART_2) {
    LPC_UART2->LCR &= ~DLAB; // required disabling DLAB to access U0/2/3 IER
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART2, your_receive_interrupt2, "UART2Int");
    NVIC_EnableIRQ(UART2_IRQn);
    LPC_UART2->IER |= enable_receive_interrupt;
    // Make unique queue for UART 2
    your_uart2_rx_queue = xQueueCreate(20, sizeof(char));
  } else if (uart_number == UART_3) {
    LPC_UART3->LCR &= ~DLAB;
    lpc_peripheral__enable_interrupt(LPC_PERIPHERAL__UART3, your_receive_interrupt3, "UART3Int");
    NVIC_EnableIRQ(UART3_IRQn);
    LPC_UART3->IER |= enable_receive_interrupt;
    // Make unique queue for UART 2
    your_uart3_rx_queue = xQueueCreate(20, sizeof(char));
  }
}

// Public function to get a char from the queue [UNMODIFIED from Lab Instruction]
bool uart_lab__get_char_from_queue2(char *input_byte, uint32_t timeout) {
  return xQueueReceive(your_uart2_rx_queue, input_byte, timeout);
}

bool uart_lab__get_char_from_queue3(char *input_byte, uint32_t timeout) {
  return xQueueReceive(your_uart3_rx_queue, input_byte, timeout);
}

void uart_lab__init(uart_number_e uart, uint32_t peripheral_clock, uint32_t baud_rate) {
  // Const variables for ease of reading code
  const uint32_t power_on_UART_2 = (1 << 24);
  const uint32_t power_on_UART_3 = (1 << 25);
  const uint8_t DLAB = (1 << 7);

  // a) Power on Peripheral
  if (uart == UART_2)
    LPC_SC->PCONP |= power_on_UART_2;

  else if (uart == UART_3)
    LPC_SC->PCONP |= power_on_UART_3;

  // b) Setup DLL, DLM, FDR, LCR registers
  const uint16_t divider = peripheral_clock / (16 * baud_rate);
  const uint8_t dlm = (divider >> 8) & 0xFF;
  const uint8_t dll = (divider >> 0) & 0xFF;

  if (uart == UART_2) {
    LPC_UART2->LCR |= (3 << 0); // set 8 bit transfer
    LPC_UART2->LCR |= DLAB;     // DLAB = 1 to access DLM/DLL registers
    LPC_UART2->DLM = dlm;       // Shares memory with RBR & THR
    LPC_UART2->DLL = dll;       // Shares memory with IER
    LPC_UART2->LCR &= ~DLAB;    // DLAB = 0 for RBR/THR usage
    // LPC_UART2->FDR |= (1 << 4); already preset
  } else if (uart == UART_3) {
    LPC_UART3->LCR |= (3 << 0); // set 8 bit transfer
    LPC_UART3->LCR |= DLAB;
    LPC_UART3->DLM = dlm;
    LPC_UART3->DLL = dll;
    LPC_UART3->LCR &= ~DLAB;
    // LPC_UART3->FDR |= (1 << 4); already preset
  }
}

// Read the byte from RBR and actually save it to the pointer
bool uart_lab__polled_get(uart_number_e uart, char *input_byte) {
  // a) Check LSR for Receive Data Ready
  if (uart == UART_2) {
    while (!(LPC_UART2->LSR & (1 << 0))) { // while it is empty
      ;                                    // Do nothing
    }
    // Copy data from RBR register to input_byte
    *input_byte = LPC_UART2->RBR;
  } else if (uart == UART_3) {
    while (!(LPC_UART3->LSR & (1 << 0))) { // while it is empty
      ;                                    // Do nothing
    }
    // Copy data from RBR register to input_byte
    *input_byte = LPC_UART3->RBR;
  }
}

bool uart_lab__polled_put(uart_number_e uart, char output_byte) {
  // a) Check LSR for Transmit Hold Register Empty
  if (uart == UART_2) {
    while (!(LPC_UART2->LSR & (1 << 5))) { // while not empty
      ;                                    // Do nothing
    }
    // b) Copy output_byte to THR register
    LPC_UART2->THR = output_byte;
  } else if (uart == UART_3) {
    while (!(LPC_UART3->LSR & (1 << 5))) { // while not empty
      ;                                    // Do nothing
    }
    // b) Copy output_byte to THR register
    LPC_UART3->THR = output_byte;
  }
}