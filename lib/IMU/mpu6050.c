#include "mpu6050.h"

// Função para resetar e inicializar o MPU6050
void mpu6050_reset(i2c_inst_t *i2c, int addr)
{
    // Dois bytes para reset: primeiro o registrador, segundo o dado
    uint8_t buf[] = {0x6B, 0x80};
    i2c_write_blocking(i2c, addr, buf, 2, false);
    sleep_ms(100); // Aguarda reset e estabilização

    // Sai do modo sleep (registrador 0x6B, valor 0x00)
    buf[1] = 0x00;
    i2c_write_blocking(i2c, addr, buf, 2, false);
    sleep_ms(10); // Aguarda estabilização após acordar
}

// Função para ler dados crus do acelerômetro, giroscópio e temperatura
void mpu6050_read_raw(int16_t accel[3], int16_t gyro[3], int16_t *temp, i2c_inst_t *i2c, int addr)
{
    uint8_t buffer[6];

    // Lê aceleração a partir do registrador 0x3B (6 bytes)
    uint8_t val = 0x3B;
    i2c_write_blocking(i2c, addr, &val, 1, true);
    i2c_read_blocking(i2c, addr, buffer, 6, false);

    for (int i = 0; i < 3; i++)
    {
        accel[i] = (buffer[i * 2] << 8) | buffer[(i * 2) + 1];
    }

    // Lê giroscópio a partir do registrador 0x43 (6 bytes)
    val = 0x43;
    i2c_write_blocking(i2c, addr, &val, 1, true);
    i2c_read_blocking(i2c, addr, buffer, 6, false);

    for (int i = 0; i < 3; i++)
    {
        gyro[i] = (buffer[i * 2] << 8) | buffer[(i * 2) + 1];
    }

    // Lê temperatura a partir do registrador 0x41 (2 bytes)
    val = 0x41;
    i2c_write_blocking(i2c, addr, &val, 1, true);
    i2c_read_blocking(i2c, addr, buffer, 2, false);

    *temp = (buffer[0] << 8) | buffer[1];
}
