#pragma once

#include "gpio.h"
#include "lpc40xx.h"
#include "ssp2.h"
#include <stdint.h>

void decoder__initialize();
void sj2_write_decoder(uint8_t address, uint16_t data);
uint16_t sj2_read_decoder(uint8_t address);
void sj2_play_music(char data);