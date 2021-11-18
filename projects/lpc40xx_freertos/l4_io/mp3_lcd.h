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

#define LCD_INCREMENT 1
#define LCD_DECREMENT 0
#define LCD_SHIFT 1
#define LCD_NO_SHIFT 0
#define LCD_DISPLAY_ON 1
#define LCD_DISPLAY_OFF 0
#define LCD_CURSOR_ON 1
#define LCD_CURSOR_OFF 0
#define LCD_CURSOR_BLINK_ON 1
#define LCD_CURSOR_BLINK_OFF 0
#define LCD_MOVE_CURSOR 1
#define LCD_MOVE_SHIFT 0
#define LCD_MOVE_RIGHT 1
#define LCD_MOVE_LEFT 0
#define LCD_LENGTH_8BIT 1
#define LCD_LENGTH_4BIT 0
#define LCD_2_LINE 1
#define LCD_1_LINE 0
#define LCD_FONT_5_10 1
#define LCD_FONT_5_8 0

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
