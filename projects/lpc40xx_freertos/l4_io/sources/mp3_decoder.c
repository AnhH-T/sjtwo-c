#include "mp3_decoder.h"
#include "delay.h"
#include <stdint.h>
#include <stdio.h>

static uint8_t volume;
static uint8_t treble;
static uint8_t bass;

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
  ssp2__initialize(1000);

  deset_RST();
  delay__ms(200);
  set_RST();

  chip_deselect();
  data_deselect();

  // uint16_t MP3Status = sj2_read_decoder(SCI_STATUS);
  // int vsVersion = (MP3Status >> 4) & 0x000F; // four version bits
  // printf("VS1053 Ver %d\n", vsVersion);

  delay__ms(200);
  sj2_write_decoder(SCI_MODE, 0x4800);
  delay__ms(200);

  // uint16_t MP3Mode = sj2_read_decoder(SCI_MODE);
  // printf("SCI_MODE = 0x%x\n", MP3Mode);
  // delay__ms(100);

  sj2_write_decoder(SCI_VOL, 0x4B4B);
  volume = 0x4B;
  bass = 0x07;
  treble = 0x07;

  // uint16_t volumeRead = sj2_read_decoder(SCI_VOL);
  // printf("SCI_VOL = 0x%x\n", volumeRead);
  // delay__ms(200);

  sj2_write_decoder(SCI_CLOCKF, 0x6000);
}

void mp3_increase_vol_by_10_percent() {
  uint16_t setVolume = 0;
  bool volume_is_already_maxed = (volume <= 0);
  bool volume_is_super_low = (volume >= 150);

  if (volume_is_already_maxed) {
    return; // if already max
  } else if (volume_is_super_low) {
    volume = 135;
  } else {
    volume -= 15;
  }
  setVolume |= (volume << 8);
  setVolume |= (volume << 0);
  sj2_write_decoder(SCI_VOL, setVolume);
}

void mp3_decrease_vol_by_10_percent() {
  uint16_t setVolume = 0;
  if (volume >= 150) {
    sj2_write_decoder(SCI_VOL, 0xFEFE);
    return;
  } else {
    volume += 15;
    setVolume |= (volume << 8);
    setVolume |= (volume << 0);
  }
  sj2_write_decoder(SCI_VOL, setVolume);
}

uint8_t mp3_get_volume_percentage() {
  uint8_t temp;
  if (volume == 0) {
    temp = 100;
  } else if (volume >= 150)
    temp = 0;
  else {
    temp = (100 - ((volume / 15) * 10));
  }
  return temp;
}

static void update_current_treble_and_bass_value() {
  uint16_t temp = 0;
  temp |= (treble << 8);
  temp |= (bass << 0);
  sj2_write_decoder(SCI_BASS, temp);
}

void mp3_increase_treble() {
  uint8_t increased_treble = (treble >> 4);
  if (increased_treble == 0x07) {
    return;
  } else if (increased_treble == 0x0F) {
    increased_treble = 0x00;
  } else {
    increased_treble += 0x01;
  }
  treble &= ~(0xF << 4);
  treble |= (increased_treble << 4);
  update_current_treble_and_bass_value();
}

void mp3_decrease_treble() {
  uint8_t decreased_treble = (treble >> 4);
  if (decreased_treble == 0x00) {
    decreased_treble = 0x0F;
  } else if (decreased_treble == 0x08) {
    return;
  } else {
    decreased_treble -= 0x01;
  }
  treble &= ~(0xF << 4);
  treble |= (decreased_treble << 4);
  update_current_treble_and_bass_value();
}

void mp3_increase_bass() {
  uint8_t increased_bass = (bass >> 4);
  if (increased_bass == 0x0F) {
    return;
  } else {
    increased_bass += 0x01;
    bass &= ~(0xF << 4);
    bass |= (increased_bass << 4);
  }
  update_current_treble_and_bass_value();
}

void mp3_decrease_bass() {
  uint8_t decreased_bass = (bass >> 4);
  if (decreased_bass == 0x00) {
    return;
  } else {
    decreased_bass -= 0x01;
    bass &= ~(0xF << 4);
    bass |= (decreased_bass << 4);
  }
  update_current_treble_and_bass_value();
}

uint8_t mp3_get_treble() {
  uint8_t temp = 0x00;
  temp |= (treble >> 4);
  return temp;
}

uint8_t mp3_get_bass() {
  uint8_t temp = 0x00;
  temp |= (bass >> 4);
  return temp;
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