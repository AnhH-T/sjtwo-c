#include "mp3_lcd.h"
#include "delay.h"
gpio_s lcd__reg_select;
gpio_s lcd__read_write_select;
gpio_s lcd__enable;
gpio_s lcd__db7, lcd__db6, lcd__db5, lcd__db4, lcd__db3, lcd__db2, lcd__db1, lcd__db0;

// private pin config functions
static void Reg_select_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__reg_select);
  else
    gpio__reset(lcd__reg_select);
}

static void RW_bit(bool read__1h) {
  if (read__1h)
    gpio__set(lcd__read_write_select);
  else
    gpio__reset(lcd__read_write_select);
}

static void DB7_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db7);
  else
    gpio__reset(lcd__db7);
}

static void DB6_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db6);
  else
    gpio__reset(lcd__db6);
}

static void DB5_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db5);
  else
    gpio__reset(lcd__db5);
}

static void DB4_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db4);
  else
    gpio__reset(lcd__db4);
}

static void DB3_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db3);
  else
    gpio__reset(lcd__db3);
}

static void DB2_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db2);
  else
    gpio__reset(lcd__db2);
}

static void DB1_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db1);
  else
    gpio__reset(lcd__db1);
}

static void DB0_bit(bool active__1h) {
  if (active__1h)
    gpio__set(lcd__db0);
  else
    gpio__reset(lcd__db0);
}

static void lcd_uart_pins_init() {
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

  lcd__db6 = gpio__construct_with_function(GPIO__PORT_2, 6, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db6);

  lcd__db7 = gpio__construct_with_function(GPIO__PORT_1, 29, GPIO__FUNCITON_0_IO_PIN);
  gpio__set_as_output(lcd__db7);
}

static void lcd_clock() {
  gpio__set(lcd__enable);
  delay__ms(1);
  gpio__reset(lcd__enable);
  delay__ms(1);
}

static void lcd_print(uint8_t character) {
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

static void lcd_set_position(uint8_t cursor, uint8_t line) {
  if (cursor > 19) {
    cursor = 0;
  }
  switch (line) {
  case 0:
    cursor += 0;
    break;
  case 1:
    cursor += 0x40;
    break;
  case 2:
    cursor += 0x14;
    break;
  case 3:
    cursor += 0x54;
    break;
  default:
    cursor = 0;
  }

  lcd_command(0x80 | cursor);
}

static void lcd_print_helper(const char *string) {
  int count = 0;
  for (count = 0; count < 20; count++) {
    bool end_of_string = (string[count] == '\0');
    bool string_dot_extension = (string[count] == '.');
    if (end_of_string || string_dot_extension)
      break;
    lcd_print(string[count]);
  }
  while (count < 20) {
    lcd_print(' ');
    count++;
  }
}

void lcd_clear_line(int line) {
  char clear[20] = "                    ";
  bool valid_line = (line < 4 && line >= 0);
  if (valid_line) {
    lcd_set_position(0, line);
    lcd_print_helper(clear);
  }
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

static void lcd_set_8bit_mode() { lcd_command(0x30); }

void lcd_clear() { lcd_command(0x01); }

void lcd_home() { lcd_command(0x02); }

void lcd_entry_mode_set(uint8_t inc_dec, uint8_t shift) {
  inc_dec = (inc_dec << 1) & 0x02;
  shift = shift & 0x01;
  lcd_command(0x04 | inc_dec | shift);
}

void lcd_display_control(uint8_t display_on_off, uint8_t cursor_on_off, uint8_t cursor_blink) {
  display_on_off = (display_on_off << 2) & 0x04;
  cursor_on_off = (display_on_off << 1) & 0x02;
  cursor_blink = cursor_blink & 0x01;
  lcd_command(0x08 | display_on_off | cursor_on_off | cursor_blink);
}

void lcd_cursor_control(uint8_t cursor_shift, uint8_t right_left) {
  cursor_shift = (cursor_shift << 3) & 0x08;
  right_left = (right_left << 2) & 0x04;
  lcd_command(0x10 | cursor_shift | right_left);
}

void lcd_function_set(uint8_t data_len, uint8_t lines, uint8_t font) {
  data_len = (data_len << 4) & 0x10;
  lines = (lines << 3) & 0x08;
  font = (font << 2) & 0x04;
  lcd_command(0x10 | data_len | lines | font);
}

void lcd_init() {
  lcd_uart_pins_init();
  Reg_select_bit(0);
  RW_bit(0);
  delay__ms(50);
  lcd_set_8bit_mode(); // LCD_8BITMODE 0x30
  lcd_set_8bit_mode();
  lcd_set_8bit_mode();
  lcd_set_8bit_mode();
  lcd_command(0x3C);
  // lcd_function_set(LCD_LENGTH_8BIT, LCD_2_LINE, LCD_FONT_5_10);
  lcd_clear();
  lcd_entry_mode_set(LCD_INCREMENT, LCD_NO_SHIFT);
  lcd_display_control(LCD_DISPLAY_ON, LCD_CURSOR_OFF, LCD_CURSOR_BLINK_OFF);
}

void lcd_print_string(const char *string, int line) {
  lcd_set_position(0, line);
  lcd_print_helper(string);
}

void lcd_print_single_char_at_cursor_position(const char character, int cursor, int line) {
  lcd_set_position(cursor, line);
  lcd_print(character);
}

// Song must be in the form Artist-Song Title
void lcd_print_song_details_in_line_1_and_2(const char *string) {
  int index_of_string = 0;
  int cursor = 0;
  lcd_set_position(0, 1);
  for (cursor = 0; cursor < 20; cursor++) {
    bool separator = (string[index_of_string] == '-');
    if (separator) {
      index_of_string++; // get rid of - character
      break;
    }
    lcd_print(string[index_of_string]);
    index_of_string++;
  }
  while (cursor < 20) {
    lcd_print(' ');
    cursor++;
  }

  lcd_set_position(0, 2);
  for (cursor = 0; cursor < 20; cursor++) {
    bool end_of_string = (string[index_of_string] == '\0');
    bool string_dot_extension = (string[index_of_string] == '.');
    if (end_of_string || string_dot_extension)
      break;
    lcd_print(string[index_of_string]);
    index_of_string++;
  }
  while (cursor < 20) {
    lcd_print(' ');
    cursor++;
  }
}

void lcd_print_arrow_on_right_side(int line) {
  lcd_set_position(19, line);
  lcd_print('<');
}