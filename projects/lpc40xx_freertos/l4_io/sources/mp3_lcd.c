
#include "mp3_lcd.h"
#define enable_i2c_and_assert_ack_flag 0x44 // 22.8, control set register

void write_to_lcd() {}

void display_clear() {}
void set_function() {}
void set_display() {}
void entry_mode() {}

void init_i2c_lcd(uint8_t slave_address_to_respond_to) {
  display_clear();
  set_function();
  set_display();
  entry_mode();

  LPC_I2C2->ADR0 = slave_address_to_respond_to;
  LPC_I2C2->CONSET = enable_i2c_and_assert_ack_flag;
}
