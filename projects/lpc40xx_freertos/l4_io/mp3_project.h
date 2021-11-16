#pragma once

#include "gpio.h"
#include "lpc40xx.h"
#include "ssp2.h"
#include <stdint.h>

/* VS1053B V4  Registers */
#define SCI_MODE 0x00
#define SCI_STATUS 0x01
#define SCI_BASS 0x02
#define SCI_CLOCKF 0x03
#define SCI_DECODE_TIME 0x04
#define SCI_AUDATA 0x05
#define SCI_WRAM 0x06
#define SCI_WRAMADDR 0x07
#define SCI_HDAT0 0x08
#define SCI_HDAT1 0x09
#define SCI_AIADDR 0x0A
#define SCI_VOL 0x0B
#define SCI_AICTRL0 0x0C
#define SCI_AICTRL1 0x0D
#define SCI_AICTRL2 0x0E
#define SCI_AICTRL3 0x0F
#define MAX_VOLUME 0x0000
#define MIN_VOLUME 0xFAFA

void chip_select(void);
void chip_deselect(void);
void data_select(void);
void data_deselect(void);
void set_RST();
void deset_RST();
bool DREQ_Ready();

void decoder_pin_config();

void mp3_decoder_init();

void sj2_write_decoder(uint8_t address, uint16_t data);
void sj2_write_decoder_unprotected(uint8_t address, uint16_t data);

uint16_t sj2_read_decoder(uint8_t address);
uint16_t sj2_read_decoder_unprotected(uint8_t address);

void sj2_send_music_data(char data);