#include <stdio.h>

#include "FreeRTOS.h"
#include "task.h"

#include "board_io.h"
#include "common_macros.h"
#include "gpio.h"
#include "periodic_scheduler.h"
#include "sj2_cli.h"

#include "i2c.h"
#include "i2c_slave_functions.h"
#include "i2c_slave_init.h"
static gpio_s led[4] = {{2, 3}, {1, 26}, {1, 24}, {1, 18}};

static volatile uint8_t slave_memory[256];
bool i2c_slave_callback__read_memory(uint8_t memory_index, uint8_t *memory) {
  if (memory_index > 255)
    return false;
  else
    *memory = slave_memory[memory_index];
  return true;
}

bool i2c_slave_callback__write_memory(uint8_t memory_index, uint8_t memory_value) {
  if (memory_index > 255)
    return false;
  else
    slave_memory[memory_index] = memory_value;
  return true;
}

static void ledtest() {
  while (true) {
    if (slave_memory[105] == 69) {
      for (int j = 3; j >= 0; j--) {
        gpio__reset(led[j]); // Set pin Low
        vTaskDelay(200);
      }
    } else {
      for (int j = 3; j >= 0; j--) {
        gpio__set(led[j]); // Set pin High
        vTaskDelay(200);
      }
    }
  }
}

static void init_gpio_stuff() {
  for (int i = 0; i < 4; i++)
    gpio__set_as_output(led[i]);
}

int main(void) {
  sj2_cli__init();
  i2c1__slave_init(0x68);
  for (unsigned slave_address = 2; slave_address <= 254; slave_address += 2) {
    if (i2c__detect(I2C__2, slave_address)) {
      printf("I2C slave detected at address: 0x%02X\n", slave_address);
    }
  }
  init_gpio_stuff();
  xTaskCreate(ledtest, "led", 2048 / sizeof(void *), NULL, PRIORITY_LOW, NULL);

  vTaskStartScheduler(); // This function never returns unless RTOS scheduler runs out of memory and fails

  return 0;
}