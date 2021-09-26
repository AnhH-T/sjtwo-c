#include <stdbool.h>
#include <stdio.h>

#include "lpc40xx.h"
/// Should alter the hardware registers to set the pin as input
void gpiox__set_as_input(uint8_t port, uint8_t pin_num) {
  if (pin_num < 32) {
    switch (port) {
    case 0:
      LPC_GPIO0->DIR &= ~(1 << pin_num);
      break;
    case 1:
      LPC_GPIO1->DIR &= ~(1 << pin_num);
      break;
    case 2:
      LPC_GPIO2->DIR &= ~(1 << pin_num);
      break;
    case 3:
      LPC_GPIO3->DIR &= ~(1 << pin_num);
      break;
    case 4:
      LPC_GPIO4->DIR &= ~(1 << pin_num);
      break;

    default:
      break;
    }
  }
}

/// Should alter the hardware registers to set the pin as output
void gpiox__set_as_output(uint8_t port, uint8_t pin_num) {
  if (pin_num < 32) {
    switch (port) {
    case 0:
      LPC_GPIO0->DIR |= (1 << pin_num);
      break;
    case 1:
      LPC_GPIO1->DIR |= (1 << pin_num);
      break;
    case 2:
      LPC_GPIO2->DIR |= (1 << pin_num);
      break;
    case 3:
      LPC_GPIO3->DIR |= (1 << pin_num);
      break;
    case 4:
      LPC_GPIO4->DIR |= (1 << pin_num);
      break;

    default:
      break;
    }
  }
}

/// Should alter the hardware registers to set the pin as high
void gpiox__set_high(uint8_t port, uint8_t pin_num) {
  if (pin_num < 32) {
    switch (port) {
    case 0:
      LPC_GPIO0->PIN |= (1 << pin_num);
      break;
    case 1:
      LPC_GPIO1->PIN |= (1 << pin_num);
      break;
    case 2:
      LPC_GPIO2->PIN |= (1 << pin_num);
      break;
    case 3:
      LPC_GPIO3->PIN |= (1 << pin_num);
      break;
    case 4:
      LPC_GPIO4->PIN |= (1 << pin_num);
      break;

    default:
      break;
    }
  }
}

/// Should alter the hardware registers to set the pin as low
void gpiox__set_low(uint8_t port, uint8_t pin_num) {
  if (pin_num < 32) {
    switch (port) {
    case 0:
      LPC_GPIO0->PIN &= ~(1 << pin_num);
      break;
    case 1:
      LPC_GPIO1->PIN &= ~(1 << pin_num);
      break;
    case 2:
      LPC_GPIO2->PIN &= ~(1 << pin_num);
      break;
    case 3:
      LPC_GPIO3->PIN &= ~(1 << pin_num);
      break;
    case 4:
      LPC_GPIO4->PIN &= ~(1 << pin_num);
      break;

    default:
      break;
    }
  }
}

/**
 * Should alter the hardware registers to set the pin as low
 *
 * @param {bool} high - true => set pin high, false => set pin low
 */
void gpiox__set(uint8_t port, uint8_t pin_num, bool high) {
  if (high)
    gpiox__set_high(port, pin_num);
  else
    gpiox__set_low(port, pin_num);
}

/**
 * Should return the state of the pin (input or output, doesn't matter)
 *
 * @return {bool} level of pin high => true, low => false
 */
bool gpiox__get_level(uint8_t port, uint8_t pin_num) {
  if (pin_num < 32) {
    switch (port) {
    case 0:
      return (LPC_GPIO0->PIN & (1 << pin_num));
      break;
    case 1:
      return (LPC_GPIO1->PIN & (1 << pin_num));
      break;
    case 2:
      return (LPC_GPIO2->PIN & (1 << pin_num));
      break;
    case 3:
      return (LPC_GPIO3->PIN & (1 << pin_num));
      break;
    case 4:
      return (LPC_GPIO4->PIN & (1 << pin_num));
      break;

    default:
      break;
    }
  }

  return false; // should never get here, but here so compiler doesn't complain
}