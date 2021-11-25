#pragma once

#include "gpio.h"
#include <stdio.h>

// Function pointer type (demonstrated later in the code sample)
typedef void (*function_pointer_t)(void);

// Allow the user to attach their callbacks
void mp3__gpio_attach_interrupt(gpio_s portpin, function_pointer_t callback);

// Our main() should configure interrupts to invoke this dispatcher where we will invoke user attached callbacks
// You can hijack 'interrupt_vector_table.c' or use API at lpc_peripherals.h
void mp3__interrupt_dispatcher(void);