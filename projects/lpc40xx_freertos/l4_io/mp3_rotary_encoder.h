#include <stdint.h>
#include <stdio.h>

#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

// Buttons
extern gpio_s middle_button; // SW1
extern gpio_s left_button;   // SW3
extern gpio_s up_button;     // SW2
extern gpio_s down_button;   // SW4
extern gpio_s right_button;  // SW5

void encoder_button_init(void);