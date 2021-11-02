#include "i2c_slave_init.h"
#include "i2c.h"

#define reset_bits ~(0b111)
#define i2c_config 0b011
#define open_drain (0b1 << 10)
#define enable_i2c_and_assert_ack_flag 0x44

void i2c1__slave_init(uint8_t slave_address_to_respond_to) {
  const uint32_t i2c_speed_hz = UINT32_C(400) * 1000;
  LPC_IOCON->P0_0 &= reset_bits;
  LPC_IOCON->P0_0 |= i2c_config;
  LPC_IOCON->P0_0 |= open_drain;

  LPC_IOCON->P0_1 &= reset_bits;
  LPC_IOCON->P0_1 |= i2c_config;
  LPC_IOCON->P0_1 |= open_drain;

  /* I2C driver initialization requires memory of binary semaphore and mutex provided by the user
   * This should not go out of scope, and hence is 'static'
   */
  static StaticSemaphore_t binary_semaphore_struct;
  static StaticSemaphore_t mutex_struct;
  i2c__initialize(I2C__1, i2c_speed_hz, clock__get_peripheral_clock_hz(), &binary_semaphore_struct, &mutex_struct);

  // For a 2 board setup, below is all you need, comment out everything up top and change I2C1 to I2C2
  LPC_I2C1->ADR0 = slave_address_to_respond_to;
  LPC_I2C1->CONSET = enable_i2c_and_assert_ack_flag;
}
