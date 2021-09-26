#include "gpio_isr.h"
#include "gpio.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"
#include <stdio.h>

// Note: You may want another separate array for falling vs. rising edge callbacks
static function_pointer_t gpio0_callbacksR[32]; // Rising Edge
static function_pointer_t gpio0_callbacksF[32]; // Falling Edge
static function_pointer_t gpio2_callbacksR[32]; // Rising Edge
static function_pointer_t gpio2_callbacksF[32]; // Falling Edge

void gpiox__attach_interrupt(uint8_t port, uint8_t pin, gpio_interrupt_e interrupt_type, function_pointer_t callback) {
  // 1) Store the callback based on the pin at gpio0_callbacks
  // 2) Configure GPIO 0 pin for rising or falling edge
  if (interrupt_type == GPIO_INTR__FALLING_EDGE) {
    if (port == 0) {
      gpio0_callbacksF[pin] = callback; // Put the callback function into lookup table based on their port and int type
      LPC_GPIOINT->IO0IntEnF |= (1 << pin); // Configure pin to enable interrupt for rising or falling edge
    } else if (port == 2) {
      gpio2_callbacksF[pin] = callback;
      LPC_GPIOINT->IO2IntEnF |= (1 << pin);
    }
  } else if (interrupt_type == GPIO_INTR__RISING_EDGE) {
    if (port == 0) {
      gpio0_callbacksR[pin] = callback;
      LPC_GPIOINT->IO0IntEnR |= (1 << pin);
    } else if (port == 2) {
      gpio2_callbacksR[pin] = callback;
      LPC_GPIOINT->IO2IntEnR |= (1 << pin);
    }
  }
}

void gpiox__interrupt_dispatcher(void) {
  // Check which pin generated the interrupt
  gpio_interrupt_e interrupt_type = -1;
  int pin = -1;  // temp var to save the pin
  int port = -1; // temp var to save the port

  /* Depending on which one returns:
      IO0IntStatR or IO0IntStatF
      IO2IntStatR or IO2IntStatF
    Whenever it detects the first "1", it will save that pin number,
    and depending on which if/else statement it triggered,
    it will also save the port, and depending on if it's R or F,
    it will also save the interrupt type
  */
  for (int i = 0; i < 32; i++) {
    if ((LPC_GPIOINT->IO0IntStatR & (1 << i))) {
      pin = i;
      port = 0;
      interrupt_type = GPIO_INTR__RISING_EDGE;
      break;
    } else if ((LPC_GPIOINT->IO0IntStatF & (1 << i))) {
      pin = i;
      port = 0;
      interrupt_type = GPIO_INTR__FALLING_EDGE;
      break;
    } else if ((LPC_GPIOINT->IO2IntStatR & (1 << i))) {
      pin = i;
      port = 2;
      interrupt_type = GPIO_INTR__RISING_EDGE;
      break;
    } else if ((LPC_GPIOINT->IO2IntStatF & (1 << i))) {
      pin = i;
      port = 2;
      interrupt_type = GPIO_INTR__FALLING_EDGE;
      break;
    }
  }

  if (pin < 0 || port < 0)
    return; // If pin and port are not set, there was no interrupt

  const int pin_that_generated_interrupt = pin; // put the pin that generated the interrupt into the const variable
  function_pointer_t attached_user_handler; // Once the correct lookup table function is found, it will be stored here
  if (port == 0) {                          // Check which port triggered the interrupt
    if (interrupt_type == GPIO_INTR__RISING_EDGE) // Check the interrupt type so it could check the correct lookup table
      attached_user_handler = gpio0_callbacksR[pin_that_generated_interrupt];
    else if (interrupt_type == GPIO_INTR__FALLING_EDGE)
      attached_user_handler = gpio0_callbacksF[pin_that_generated_interrupt];
  } else if (port == 2) {
    if (interrupt_type == GPIO_INTR__RISING_EDGE)
      attached_user_handler = gpio2_callbacksR[pin_that_generated_interrupt];
    else if (interrupt_type == GPIO_INTR__FALLING_EDGE)
      attached_user_handler = gpio2_callbacksF[pin_that_generated_interrupt];
  }
  // Invoke the user registered callback
  attached_user_handler();
  // Clear the interrupts for all pins on both ports
  LPC_GPIOINT->IO0IntClr = 0xFFFFFFFF;
  LPC_GPIOINT->IO2IntClr = 0xFFFFFFFF;
}