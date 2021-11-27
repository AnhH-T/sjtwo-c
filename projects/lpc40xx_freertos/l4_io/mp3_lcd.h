#pragma once
#include "gpio.h"
#include "lpc40xx.h"
#include "stdbool.h"
#include <stdint.h>
#include <stdio.h>

// LCD instruction address
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

void lcd_init();
void lcd_command(uint8_t command);
void lcd_clear();
void lcd_home();
void lcd_entry_mode_set(uint8_t inc_dec, uint8_t shift);
void lcd_display_control(uint8_t display_on_off, uint8_t cursor_on_off, uint8_t cursor_blink);
void lcd_cursor_control(uint8_t cursor_shift, uint8_t right_left);
void lcd_function_set(uint8_t data_len, uint8_t lines, uint8_t font);
void lcd_print_arrow_on_right_side(int line);
void lcd_clear_line(int line);
void lcd_print_song_details_in_line_1_and_2(const char *string);
void lcd_print_string(const char *string, int line);