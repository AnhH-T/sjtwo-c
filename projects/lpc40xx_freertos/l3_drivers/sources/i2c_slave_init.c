#include "i2c_slave_init.h"
#include "i2c.h"

void i2c1__slave_init(uint8_t slave_address_to_respond_to) {
  const uint32_t i2c_speed_hz = UINT32_C(400) * 1000;
  LPC_IOCON->P0_0 &= ~(0b111);
  LPC_IOCON->P0_0 |= 0b011;
  LPC_IOCON->P0_0 |= (0b1 << 10);

  LPC_IOCON->P0_1 &= ~(0b111);
  LPC_IOCON->P0_1 |= 0b011;
  LPC_IOCON->P0_1 |= (0b1 << 10);

  /* I2C driver initialization requires memory of binary semaphore and mutex provided by the user
   * This should not go out of scope, and hence is 'static'
   */
  static StaticSemaphore_t binary_semaphore_struct;
  static StaticSemaphore_t mutex_struct;
  i2c__initialize(I2C__1, i2c_speed_hz, clock__get_peripheral_clock_hz(), &binary_semaphore_struct, &mutex_struct);

  // For a 2 board setup, below is all you need, comment out everything up top and change I2C1 to I2C2
  LPC_I2C1->ADR0 = slave_address_to_respond_to;
  LPC_I2C1->CONSET = 0x44;
}
