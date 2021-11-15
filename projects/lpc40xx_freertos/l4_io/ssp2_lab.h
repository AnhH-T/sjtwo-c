#pragma once

#include <stdint.h>
typedef struct {
  char byte[512];
} song_data_s;

// Refer to LPC User manual and setup the register bits correctly
// a) Power on Peripheral
// b) Setup control registers CR0 and CR1
// c) Setup prescalar register to be <= max_clock_mhz
void ssp2_lab__init(uint32_t max_clock_mhz);

// Configure the Data register(DR) to send and receive data by checking the SPI peripheral status register
uint8_t ssp2_lab__exchange_byte(uint8_t data_out);
song_data_s ssp2_lab__exchange_song_byte(song_data_s data_out);