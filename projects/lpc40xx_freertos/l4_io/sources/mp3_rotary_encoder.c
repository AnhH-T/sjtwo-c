#include "mp3_rotary_encoder.h"

// Buttons
gpio_s middle_button; // SW1
gpio_s left_button;   // SW3
gpio_s up_button;     // SW2
gpio_s down_button;   // SW4
gpio_s right_button;  // SW5

// Rotary Wheel
gpio_s encA;
gpio_s encB;

void button_init() {
  middle_button = gpio__construct_as_input(0, 18);
  gpio__set_function(middle_button, GPIO__FUNCITON_0_IO_PIN);
  left_button = gpio__construct_as_input(2, 9);
  gpio__set_function(left_button, GPIO__FUNCITON_0_IO_PIN);
  up_button = gpio__construct_as_input(0, 15);
  gpio__set_function(up_button, GPIO__FUNCITON_0_IO_PIN);
  down_button = gpio__construct_as_input(2, 7);
  gpio__set_function(down_button, GPIO__FUNCITON_0_IO_PIN);
  right_button = gpio__construct_as_input(2, 5);
  gpio__set_function(right_button, GPIO__FUNCITON_0_IO_PIN);
  encA = gpio__construct_with_function(GPIO__PORT_1, 20, GPIO__FUNCTION_3);
  encB = gpio__construct_with_function(GPIO__PORT_1, 23, GPIO__FUNCTION_3);
}

void encoder__turn_on_power(void) { lpc_peripheral__turn_on_power_to(LPC_PERIPHERAL__QEI); } // Turn on power for ENC
uint32_t encoder__get_index(void) { return (LPC_QEI->INXCNT); } // Gets Index Value used to change volume
void encoder__set_max_position(void) {
  LPC_QEI->MAXPOS = 1;
} // POS needs a max value so it can tick to increment / decrement index