#include "mp3_lcd.h"
#include "delay.h"

/**
lcd__reg_select; (0)
lcd__read_write_select;(1)
lcd__enable;(2)
lcd__db0 -> lcd__db7;(3-11)
**/
void lcd_uart_pins_init() {
  lcd__reg_select = gpio__construct_with_function(GPIO__PORT_4, 28, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__reg_select);
  gpio__reset(lcd__reg_select);

  lcd__read_write_select = gpio__construct_with_function(GPIO__PORT_0, 6, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__read_write_select);
  gpio__reset(lcd__read_write_select);

  lcd__enable = gpio__construct_with_function(GPIO__PORT_0, 8, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__enable);
  gpio__reset(lcd__enable);

  // bits 0-3 are for 8 bit mode
  lcd__db0 = gpio__construct_with_function(GPIO__PORT_1, 14, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db0);

  lcd__db1 = gpio__construct_with_function(GPIO__PORT_4, 29, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db1);

  lcd__db2 = gpio__construct_with_function(GPIO__PORT_0, 7, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db2);

  lcd__db3 = gpio__construct_with_function(GPIO__PORT_0, 9, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db3);
  // bits 4-7 are for 4 bit mode
  lcd__db4 = gpio__construct_with_function(GPIO__PORT_0, 25, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db4);

  lcd__db5 = gpio__construct_with_function(GPIO__PORT_1, 30, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db5);

  lcd__db6 = gpio__construct_with_function(GPIO__PORT_1, 23, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db6);

  lcd__db7 = gpio__construct_with_function(GPIO__PORT_1, 29, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db7);
}

void lcd_init() {
  lcd_uart_pins_init();
  Reg_select_bit(0);
  RW_bit(0);
  delay__ms(50);
  lcd_command(LCD_8BITMODE);
  lcd_command(LCD_8BITMODE);
  lcd_command(LCD_8BITMODE);
  lcd_command(LCD_8BITMODE | LCD_2LINE | LCD_5x10DOTS); // 0001 1100
  lcd_command(clear_display);                           // 0000 0001
  lcd_command(entry_mode_increment_on_shift_off);       // 0000 0110
  lcd_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON);      // 0000 1100
  lcd_command(0x1C);
}

void lcd_clock() {
  gpio__set(lcd__enable);
  delay__ms(1);
  gpio__reset(lcd__enable);
  delay__ms(1);
}

void lcd_command(uint8_t command) {
  Reg_select_bit(0);
  RW_bit(0);
  DB7_bit(((1 << 7) & command));
  DB6_bit(((1 << 6) & command));
  DB5_bit(((1 << 5) & command));
  DB4_bit(((1 << 4) & command));
  DB3_bit(((1 << 3) & command));
  DB2_bit(((1 << 2) & command));
  DB1_bit(((1 << 1) & command));
  DB0_bit(((1 << 0) & command));
  lcd_clock();
  delay__ms(10);
}

void lcd_print_string_for_line_1(const char *string) {
  lcd_command(clear_display);
  // set position to the start of line 1
  lcd_set_position(0, 0);

  for (int i = 0; i < 16; i++) {
    if (string[i] == '\0')
      break;
    lcd_print(string[i]);
  }
}

void lcd_print(uint8_t character) {
  Reg_select_bit(1);
  RW_bit(0);
  DB7_bit(((1 << 7) & character));
  DB6_bit(((1 << 6) & character));
  DB5_bit(((1 << 5) & character));
  DB4_bit(((1 << 4) & character));
  DB3_bit(((1 << 3) & character));
  DB2_bit(((1 << 2) & character));
  DB1_bit(((1 << 1) & character));
  DB0_bit(((1 << 0) & character));
  lcd_clock();
}

void lcd_print_string_for_line_2(const char *string) {
  lcd_command(clear_display);
  // set position to the start of line 1
  lcd_set_position(0, 1);

  for (int i = 0; i < 16; i++) {
    if (string[i] == '\0')
      break;
    lcd_print(string[i]);
  }
}

void lcd_set_position(uint8_t cursor, uint8_t line) {
  bool second_line_currently_selected = (line == 1);
  if (second_line_currently_selected) {
    cursor += 0x40; //
  }
  lcd_command(0x80 | cursor);
}

// pin configerations
void Reg_select_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__reg_select);
  else
    gpio__reset(lcd__reg_select);
}

void RW_bit(bool read__1h) {
  if (read__1h)
    gpio__set(lcd__read_write_select);
  else
    gpio__reset(lcd__read_write_select);
}

void DB7_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db7);
  else
    gpio__reset(lcd__db7);
}

void DB6_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db6);
  else
    gpio__reset(lcd__db6);
}

void DB5_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db5);
  else
    gpio__reset(lcd__db5);
}

void DB4_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db4);
  else
    gpio__reset(lcd__db4);
}

void DB3_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db3);
  else
    gpio__reset(lcd__db3);
}

void DB2_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db2);
  else
    gpio__reset(lcd__db2);
}

void DB1_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db1);
  else
    gpio__reset(lcd__db1);
}

void DB0_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db0);
  else
    gpio__reset(lcd__db0);
}
