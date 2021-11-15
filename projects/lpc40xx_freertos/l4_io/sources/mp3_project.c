#include <stdint.h>

#include "mp3_project.h"

void chip_select(void) { LPC_GPIO0->PIN &= ~(1 << 26); }
void chip_deselect(void) { LPC_GPIO0->PIN |= (1 << 26); }

void data_select(void) { LPC_GPIO1->PIN &= ~(1 << 31); }
void data_deselect(void) { LPC_GPIO1->PIN |= (1 << 31); }

void decoder__initialize() {
  gpio_s DREQ = gpio__construct_as_input(1, 20);
  gpio_s CS = gpio__construct_as_output(0, 26);
  gpio_s DS = gpio__construct_as_output(1, 31);

  ssp2__initialize(1000);

  gpio__set(CS);
  gpio__set(DS);

  delay__ms(100);
  sj2_write_decoder(0x0, 0x800 | 0x4);
  delay__ms(200);
  sj2_write_decoder(0x3, 0x6000);
}

void sj2_write_decoder(uint8_t address, uint16_t data) {
  chip_select();
  ssp2__exchange_byte(0x2); // Opcode for write
  ssp2__exchange_byte(address);
  ssp2__exchange_byte(data >> 8);
  ssp2__exchange_byte(data);
  chip_deselect();
}

uint16_t sj2_read_decoder(uint8_t address) {
  uint16_t data;
  chip_select();
  ssp2__exchange_byte(0x3); // Opcode for read
  ssp2__exchange_byte(address);
  data |= ssp2__exchange_byte(0xFF) << 8;
  data |= ssp2__exchange_byte(0xFF) << 0;
  chip_deselect();

  return data;
}

void sj2_play_music(char data) {
  data_select();
  ssp2__exchange_byte(data);
  data_deselect();
}