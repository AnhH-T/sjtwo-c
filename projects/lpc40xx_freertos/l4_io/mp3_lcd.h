#pragma once
#include "i2c.h"
#include <stdint.h>
// HD44780U LCD addresses
#define general_call_address 0x00 // 0b00000000
#define device_id_address 0x78    // 0b111 1000
#define lcd_slave_address 0x4E

// LCD instruction address
#define clear_display 0x01
#define return_home 0x02
#define entry_mode_set 0x04
#define display_power 0x08
#define cursor_display_shift 0x10
#define function_set 0x20
#define cgram_adress 0x40  // used for custom characters
#define ddram_address 0x80 // physical display location, which memoery addres we are going to display at

// #define read_busy_flag
// #define write_data_to_CG_or_dd_ram
// #define read_data_to_CG_or_dd_ram

#define read_bit 1
#define write_bit 0

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

// flags for backlight control
#define LCD_BACKLIGHT 0x08
#define LCD_NOBACKLIGHT 0x00

#define En B00000100  // Enable bit
#define Rw B00000010  // Read/Write bit
#define Rs B00000001  // Register select bit

typedef struct {
  uint8_t LCD_SCL;
  uint8_t LCD_SDA;
} lcd_s;

// Initialization of LCD Screen
void display_clear();
void set_function();
void set_display();
void entry_mode();

void init_i2c_lcd(uint8_t);
