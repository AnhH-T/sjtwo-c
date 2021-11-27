#include "mp3_rotary_encoder.h"

// Buttons
gpio_s middle_button; // SW1
gpio_s left_button;   // SW3
gpio_s up_button;     // SW2
gpio_s down_button;   // SW4
gpio_s right_button;  // SW5

void encoder_button_init() {
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
}