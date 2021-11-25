#include "mp3_isr.h"
#include "lpc40xx.h"
#include "lpc_peripherals.h"

// Note: You may want another separate array for falling vs. rising edge callbacks
static function_pointer_t gpio0_callbacksR[32]; // Rising Edge
static function_pointer_t gpio2_callbacksR[32]; // Rising Edge

void mp3__gpio_attach_interrupt(gpio_s portpin, function_pointer_t callback) {
  if (portpin.port_number == GPIO__PORT_0) {
    gpio0_callbacksR[portpin.pin_number] = callback;
    LPC_GPIOINT->IO0IntEnF |= (1 << portpin.pin_number);
  } else if (portpin.port_number == GPIO__PORT_2) {
    gpio2_callbacksR[portpin.pin_number] = callback;
    LPC_GPIOINT->IO2IntEnF |= (1 << portpin.pin_number);
  }
}

void mp3__interrupt_dispatcher(void) {
  // Check which pin generated the interrupt
  int pin = -1;  // temp var to save the pin
  int port = -1; // temp var to save the port
  for (int i = 0; i < 32; i++) {
    if ((LPC_GPIOINT->IO0IntStatF & (1 << i))) {
      pin = i;
      port = 0;
      break;
    } else if ((LPC_GPIOINT->IO2IntStatF & (1 << i))) {
      pin = i;
      port = 2;
      break;
    }
  }

  if (pin < 0 || port < 0)
    return; // If pin and port are not set, there was no interrupt

  const int pin_that_generated_interrupt = pin; // put the pin that generated the interrupt into the const variable
  function_pointer_t attached_user_handler; // Once the correct lookup table function is found, it will be stored here
  if (port == 0) {                          // Check which port triggered the interrupt
    attached_user_handler = gpio0_callbacksR[pin_that_generated_interrupt];
  } else if (port == 2) {
    attached_user_handler = gpio2_callbacksR[pin_that_generated_interrupt];
  }
  // Invoke the user registered callback
  attached_user_handler();
  // Clear the interrupts for all pins on both ports
  LPC_GPIOINT->IO0IntClr = 0xFFFFFFFF;
  LPC_GPIOINT->IO2IntClr = 0xFFFFFFFF;
}