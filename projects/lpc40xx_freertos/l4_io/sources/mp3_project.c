#include <stdint.h>
#include <stdio.h>

#include "mp3_project.h"

void chip_select(void) { LPC_GPIO2->PIN &= ~(1 << 1); }
void chip_deselect(void) { LPC_GPIO2->PIN |= (1 << 1); }

void data_select(void) { LPC_GPIO2->PIN &= ~(1 << 2); }
void data_deselect(void) { LPC_GPIO2->PIN |= (1 << 2); }

void set_RST() { LPC_GPIO2->SET |= (1 << 4); }
void deset_RST() { LPC_GPIO2->CLR |= (1 << 4); }

bool DREQ_Ready() { return LPC_GPIO2->PIN & (1 << 0) ? true : false; }

void decoder_pin_config() {
  gpio__construct_with_function(1, 0, GPIO__FUNCTION_4); // SCK2 [13]
  gpio__construct_with_function(1, 1, GPIO__FUNCTION_4); // MOSI [11]
  gpio__construct_with_function(1, 4, GPIO__FUNCTION_4); // MISO [12]

  gpio_s DREQ = gpio__construct_with_function(2, 0, GPIO__FUNCITON_0_IO_PIN); // [3]
  gpio_s CS = gpio__construct_with_function(2, 1, GPIO__FUNCITON_0_IO_PIN);   // [7]
  gpio_s DS = gpio__construct_with_function(2, 2, GPIO__FUNCITON_0_IO_PIN);   // [6]
  gpio_s RST = gpio__construct_with_function(2, 4, GPIO__FUNCITON_0_IO_PIN);

  gpio__set_as_input(DREQ);
  gpio__set_as_output(CS);
  gpio__set_as_output(DS);
  gpio__set_as_output(RST);
}

void mp3_decoder_init() {
  decoder_pin_config();
  printf("Pin Configured...\n");
  ssp2__initialize(1000);
  printf("SPI Port 2 Init...\n");

  deset_RST();
  delay__ms(200);
  set_RST();

  chip_deselect();
  data_deselect();
  printf("Chip/Data Deselect...\n");

  uint16_t MP3Status = sj2_read_decoder(SCI_STATUS);
  int vsVersion = (MP3Status >> 4) & 0x000F; // four version bits
  printf("VS1053 Ver %d\n", vsVersion);

  delay__ms(200);

  uint16_t MP3Mode = sj2_read_decoder(SCI_MODE);
  printf("SCI_MODE = 0x%x\n", MP3Mode);

  delay__ms(100);
  sj2_write_decoder(SCI_VOL, 0x5050);
  uint16_t volume = sj2_read_decoder(SCI_VOL);
  printf("SCI_VOL = 0x%x\n", volume);
  delay__ms(200);
  sj2_write_decoder(SCI_CLOCKF, 0x6000);
  printf("Set SCI_CLOCKF...\n");
}

void sj2_write_decoder(uint8_t address, uint16_t data) {
  while (!DREQ_Ready()) {
    ; // wait
  }
  chip_select();
  ssp2__exchange_byte(0x2); // Opcode for write
  ssp2__exchange_byte(address);
  ssp2__exchange_byte(data >> 8);
  ssp2__exchange_byte(data);
  while (!DREQ_Ready()) {
    ; // wait
  }
  chip_deselect();
}

uint16_t sj2_read_decoder(uint8_t address) {
  while (!DREQ_Ready()) {
    ; // wait
  }
  uint16_t data = 0x0000;
  chip_select();
  ssp2__exchange_byte(0x3); // Opcode for read
  ssp2__exchange_byte(address);
  data |= ssp2__exchange_byte(0xFF) << 8;
  data |= ssp2__exchange_byte(0xFF) << 0;
  while (!DREQ_Ready()) {
    ; // wait
  }
  chip_deselect();

  return data;
}

void sj2_send_music_data(char data) {
  data_select();
  ssp2__exchange_byte(data);
  data_deselect();
}