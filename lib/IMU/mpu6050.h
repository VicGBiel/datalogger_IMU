#ifndef mpu6050_h
#define mpu6050_h 

#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"

void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp, i2c_inst_t *i2c, int addr);
void mpu6050_reset(i2c_inst_t *i2c, int addr);

#endif
