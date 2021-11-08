#include "i2c_slave_functions.h"

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