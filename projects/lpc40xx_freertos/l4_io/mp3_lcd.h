#pragma once
#include "gpio.h"
#include "lpc40xx.h"
#include "stdbool.h"
#include <stdint.h>
#include <stdio.h>

// LCD instruction address
#define clear_display 0x01
#define return_home 0x02
#define entry_mode_set 0x04
#define display_power 0x08
#define cursor_display_shift 0x10
#define function_set 0x20
#define cgram_adress 0x40  // used for custom characters
#define ddram_address 0x80 // physical display location, which memoery addres we are going to display at

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00
#define entry_mode_increment_on_shift_off 0x6
/* LCD Display Pins */
gpio_s lcd__reg_select;
gpio_s lcd__read_write_select;
gpio_s lcd__enable;
gpio_s lcd__db7, lcd__db6, lcd__db5, lcd__db4, lcd__db3, lcd__db2, lcd__db1, lcd__db0;

void lcd_uart_pins_init();
void lcd_init();
void lcd_clock();
void lcd_command(uint8_t command);
void lcd_set_position(uint8_t x, uint8_t y);
void lcd_print(uint8_t character);
void lcd_print_string(const char *song_name);

/* Pin Config */
void RS_bit(bool active__1h);
void RW_bit(bool read__1h);
void DB7_bit(bool active__1h);
void DB6_bit(bool active__1h);
void DB5_bit(bool active__1h);
void DB4_bit(bool active__1h);
void DB3_bit(bool active__1h);
void DB2_bit(bool active__1h);
void DB1_bit(bool active__1h);
void DB0_bit(bool active__1h);