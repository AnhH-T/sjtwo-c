#include "lpc40xx.h"

#include <stdint.h>

#include "ssp2_lab.h"

void ssp2_lab__init(uint32_t max_clock_mhz) {
  // Refer to LPC User manual and setup the register bits correctly
  // a) Power on Peripheral
  LPC_SC->PCONP |= (1 << 20); // Turn on bit 20 to power on SSP2
  // b) Setup control registers CR0 and CR1
  LPC_SSP2->CR0 &= ~(3 << 4); // Set SPI mode
  LPC_SSP2->CR0 |= (7 << 0);  // Control how many bits are transferred each frame

  LPC_SSP2->CR1 &= ~(1 << 0); // Normal Operation
  LPC_SSP2->CR1 &= ~(1 << 2); // Master Mode
  LPC_SSP2->CR1 |= (1 << 1);  // Enable SSP controller

  // c) Setup prescalar register to be <= max_clock_mhz
  uint8_t divider = 2;
  const uint32_t cpu_clock_mhz = clock__get_core_clock_hz() / (1000UL * 1000UL);
  // Keep incrementing divider until cpu clock can be divided to less than the max clock
  // max_clock_mhz / 2 is only there for logic analyzer so it works correctly suggested by lab manual
  while (max_clock_mhz / 2 <= (cpu_clock_mhz / divider) && divider <= 254) {
    divider += 2;
  }

  LPC_SSP2->CPSR = divider;
}

uint8_t ssp2_lab__exchange_byte(uint8_t data_out) {
  // Configure the Data register(DR) to send and receive data by checking the SPI peripheral status register
  LPC_SSP2->DR = data_out;
  while (LPC_SSP2->SR & (1 << 4)) {
    // do nothing until it's ready
  }
  return (uint8_t)(LPC_SSP2->DR & 0xFF); // Mask everything but the 8 bits we want
}