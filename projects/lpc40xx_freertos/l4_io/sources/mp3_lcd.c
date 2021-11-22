#include "mp3_lcd.h"

static uint8_t x_position = 0;
static uint8_t y_position = 0;

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
  RS_bit(0);
  RW_bit(0);
  delay__ms(50);
  lcd_command(LCD_8BITMODE);
  lcd_command(LCD_8BITMODE);
  lcd_command(LCD_8BITMODE);
  lcd_command(0x3C);
  lcd_command(0x01);
  lcd_command(0x06);
  lcd_command(0x0F);
  // lcd_command(LCD_8BITMODE | LCD_2LINE | LCD_5x10DOTS);
  // lcd_command(LCD_DISPLAYOFF);
  // lcd_command(clear_display);
  // lcd_command(entry_mode_increment_on_shift_off);
  // lcd_command(LCD_DISPLAYON);
}

void lcd_clock() {
  gpio__set(lcd__enable);
  delay__ms(1);
  gpio__reset(lcd__enable);
  delay__ms(1);
}

void lcd_command(uint8_t command) {
  RS_bit(0);
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

void lcd_set_position(uint8_t x, uint8_t y) {
  if (y == 1) {
    x += 0x40;
  }
  lcd_command(0x80 | x);
}

void lcd_print(uint8_t character) {
  if (x_position > 15) {
    if (y_position > 0) {
      lcd_set_position(0, 0);
      y_position = 0;
    } else {
      y_position++;
      lcd_set_position(0, 1);
    }
    x_position = 0;
  }

  RS_bit(1);
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

  x_position++;
}

void lcd_print_string(const char *song_name) {
  for (int i = 0; i < 32; i++) {
    if (song_name[i] == '\0')
      break;
    lcd_print(song_name[i]);
  }
}

// pin configerations
void RS_bit(bool active__1h) {
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
