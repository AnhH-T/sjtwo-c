#include "lpc40xx.h"

#include <stdint.h>

#include "ssp2_lab.h"

void ssp2__init(uint32_t max_clock_mhz) {
  // Refer to LPC User manual and setup the register bits correctly
  // a) Power on Peripheral
  LPC_SC->PCONP |= (1 << 20); // Turn on bit 20 to power on SSP2
  // b) Setup control registers CR0 and CR1
  LPC_SSP2->CR0 &= ~(3 << 4); // Set SPI mode
  // -------------Experimental----------------------------
  LPC_SSP2->CR0 &= ~(1 << 6); // low between frames [For high just set this bit] [CPOL]
  LPC_SSP2->CR0 &= ~(1 << 7); // Rising edge data capture [CPHA]
  // -----------------------------------------------------
  LPC_SSP2->CR0 |= (7 << 0); // Control how many bits are transferred each frame

  LPC_SSP2->CR1 &= ~(1 << 0); // Normal Operation
  LPC_SSP2->CR1 &= ~(1 << 2); // Master Mode
  LPC_SSP2->CR1 |= (1 << 1); // Enable SSP controller

  // c) Setup prescalar register to be <= max_clock_mhz
  uint8_t divider = 2;

  // Keep scaling down divider until calculated is higher
  while (max_clock_mhz < (clock__get_core_clock_hz()/(divider*1000*1000)) && divider <= 254) {
    divider += 2;
  }

  LPC_SSP2->CPSR = divider;
}

uint8_t ssp2__exchange_byte(uint8_t data_out) {
  // Configure the Data register(DR) to send and receive data by checking the SPI peripheral status register
}