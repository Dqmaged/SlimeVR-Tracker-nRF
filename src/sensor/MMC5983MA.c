/* 09/23/2017 Copyright Tlera Corporation

    Created by Kris Winer

  This sketch uses SDA/SCL on pins 21/20 (Ladybug default), respectively, and it uses the Ladybug STM32L432 Breakout Board.
  The MMC5983MA is a low power magnetometer, here used as 3 DoF in a 9 DoF absolute orientation solution.

  Library may be used freely and without limit with attribution.

*/
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include <zephyr/types.h>
#include <zephyr/drivers/i2c.h>
#include "MMC5983MA.h"

uint8_t mmc_getChipID(struct i2c_dt_spec dev_i2c)
{
    uint8_t c;
    i2c_reg_read_byte_dt(&dev_i2c, MMC5983MA_PRODUCT_ID, &c);
    return c;
}

void mmc_reset(struct i2c_dt_spec dev_i2c)
{
    // reset device
    i2c_reg_write_byte_dt(&dev_i2c, MMC5983MA_CONTROL_1, 0x80); // Set bit 7 to 1 to reset MMC5983MA
    k_msleep(11); // Wait 10 ms for all registers to reset 
}

void mmc_init(struct i2c_dt_spec dev_i2c, uint8_t MODR, uint8_t MBW, uint8_t MSET)
{
    // enable auto set/reset (bit 5 == 1)
    i2c_reg_write_byte_dt(&dev_i2c, MMC5983MA_CONTROL_0, 0x20);

    // set magnetometer bandwidth
    i2c_reg_write_byte_dt(&dev_i2c, MMC5983MA_CONTROL_1, MBW);

    // enable continuous measurement mode (bit 3 == 1), set sample rate
    // enable automatic Set/Reset (bit 8 == 1), set set/reset rate
    i2c_reg_write_byte_dt(&dev_i2c, MMC5983MA_CONTROL_2, 0x80 | (MSET << 4) | 0x08 | MODR);
}

void mmc_SET(struct i2c_dt_spec dev_i2c)
{
    i2c_reg_write_byte_dt(&dev_i2c, MMC5983MA_CONTROL_0, 0x08);
    k_busy_wait(1); // self clearing after 500 ns
}

void mmc_RESET(struct i2c_dt_spec dev_i2c)
{
    i2c_reg_write_byte_dt(&dev_i2c, MMC5983MA_CONTROL_0, 0x10);
    k_busy_wait(1); // self clearing after 500 ns
}

uint8_t mmc_status(struct i2c_dt_spec dev_i2c)
{
    // Read status register
    uint8_t temp;
    i2c_reg_read_byte_dt(&dev_i2c, MMC5983MA_STATUS, &temp);
    return temp;
}

void mmc_clearInt(struct i2c_dt_spec dev_i2c)   
{
    // Clear data ready interrupts
    i2c_reg_write_byte_dt(&dev_i2c, MMC5983MA_STATUS, 0x01);
}

void mmc_readData(struct i2c_dt_spec dev_i2c, uint32_t * destination)
{
    uint8_t rawData[7]; // x/y/z mag register data stored here
    i2c_burst_read_dt(&dev_i2c, MMC5983MA_XOUT_0, &rawData[0], 7); // Read the 7 raw data registers into data array
    destination[0] = (uint32_t)(rawData[0] << 10 | rawData[1] << 2 | (rawData[6] & 0xC0) >> 6); // Turn the 18 bits into a unsigned 32-bit value
    destination[1] = (uint32_t)(rawData[2] << 10 | rawData[3] << 2 | (rawData[6] & 0x30) >> 4); // Turn the 18 bits into a unsigned 32-bit value
    destination[2] = (uint32_t)(rawData[4] << 10 | rawData[5] << 2 | (rawData[6] & 0x0C) >> 2); // Turn the 18 bits into a unsigned 32-bit value
}

void mmc_powerDown(struct i2c_dt_spec dev_i2c)
{
    i2c_reg_update_byte_dt(&dev_i2c, MMC5983MA_CONTROL_2, 0x07, 0); // clear lowest four bits
    k_msleep(20); // make sure to finish the last measurement
}

void mmc_powerUp(struct i2c_dt_spec dev_i2c, uint8_t MODR)
{
    i2c_reg_update_byte_dt(&dev_i2c, MMC5983MA_CONTROL_2, MODR, MODR); // start continuous mode
}

void mmc_mag_read(struct i2c_dt_spec mag, float m[3]) {
	uint32_t rawMag[3];
	mmc_readData(mag, rawMag);
	m[0] = ((float)rawMag[0] - MMC5983MA_offset) * MMC5983MA_mRes;
	m[1] = ((float)rawMag[1] - MMC5983MA_offset) * MMC5983MA_mRes;
	m[2] = ((float)rawMag[2] - MMC5983MA_offset) * MMC5983MA_mRes;
}
