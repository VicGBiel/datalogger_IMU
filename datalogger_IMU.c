#include <stdio.h>
#include <string.h>
#include <math.h>                    
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <time.h>

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/bootrom.h"
#include "hardware/i2c.h"
#include "hardware/rtc.h"
#include "ssd1306.h"
#include "font.h"
#include "mpu6050.h"
#include "sdcard_utils.h"
// Definição dos botões
#define botaoA 5
#define botaoB 6
#define botaoJoy 22

// Definição dos pinos I2C para o MPU6050
#define I2C_PORT i2c0                 // I2C0 usa pinos 0 e 1
#define I2C_SDA 0
#define I2C_SCL 1

// Definição dos pinos I2C para o display OLED
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define ENDERECO_DISP 0x3C            // Endereço I2C do display

// Endereço padrão do MPU6050
static int addr = 0x68;

void gpio_setup();
void gpio_irq_handler(uint gpio, uint32_t events);
/*
void adc_capture_to_file(const char *filename) {
    FIL file;
    FRESULT res = f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS);
    if (res != FR_OK) {
        printf("[ERRO] Não foi possível criar o arquivo %s\n", filename);
        return;
    }
    for (int i = 0; i < 128; i++) {
        adc_select_input(0);
        uint16_t adc_value = adc_read();
        char buffer[50];
        sprintf(buffer, "%d %d\n", i + 1, adc_value);
        UINT bw;
        res = f_write(&file, buffer, strlen(buffer), &bw);
        if (res != FR_OK) {
            printf("[ERRO] Falha na escrita\n");
            f_close(&file);
            return;
        }
        sleep_ms(100);
    }
    f_close(&file);
    printf("Dados salvos em %s.\n", filename);
}
*/
int main()
{
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    stdio_init_all();
    sleep_ms(5000);

    // Inicializa a I2C do Display OLED em 400kHz
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);

    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, ENDERECO_DISP, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_send_data(&ssd);

    // Limpa o display
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    // Inicialização da I2C do MPU6050
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Declara os pinos como I2C na Binary Info
    bi_decl(bi_2pins_with_func(I2C_SDA, I2C_SCL, GPIO_FUNC_I2C));
    mpu6050_reset(I2C_PORT, addr);

    int16_t aceleracao[3], gyro[3], temp;

    bool cor = true;    

    while (1)
    {
        // Leitura dos dados de aceleração, giroscópio e temperatura
        mpu6050_read_raw(aceleracao, gyro, &temp, I2C_PORT, addr);

        // Conversão para unidade de 'g'
        float ax = aceleracao[0] / 16384.0f;
        float ay = aceleracao[1] / 16384.0f;
        float az = aceleracao[2] / 16384.0f;

        // Cálculo dos ângulos em graus (Roll e Pitch)
        float roll  = atan2(ay, az) * 180.0f / M_PI;
        float pitch = atan2(-ax, sqrt(ay*ay + az*az)) * 180.0f / M_PI;

        // Montagem das strings para o display
        char str_roll[20];
        char str_pitch[20];

        
        snprintf(str_roll,  sizeof(str_roll),  "%5.1f", roll);
        snprintf(str_pitch, sizeof(str_pitch), "%5.1f", pitch);
        

        // Exibição no display
        ssd1306_fill(&ssd, !cor);                            // Limpa o display
        ssd1306_rect(&ssd, 3, 3, 122, 60, cor, !cor);        // Desenha um retângulo
        ssd1306_line(&ssd, 3, 25, 123, 25, cor);             // Desenha uma linha horizontal
        ssd1306_line(&ssd, 3, 37, 123, 37, cor);             // Desenha outra linha horizontal
        ssd1306_draw_string(&ssd, "CEPEDI   TIC37", 8, 6);   // Escreve texto no display
        ssd1306_draw_string(&ssd, "EMBARCATECH", 20, 16);    // Escreve texto no display
        ssd1306_draw_string(&ssd, "IMU    MPU6050", 10, 28); // Escreve texto no display
        ssd1306_line(&ssd, 63, 35, 63, 60, cor);             // Desenha uma linha vertical
        ssd1306_draw_string(&ssd, "roll", 14, 41);           // Escreve texto no display
        ssd1306_draw_string(&ssd, str_roll, 14, 52);         // Exibe Roll
        ssd1306_draw_string(&ssd, "pitch", 73, 41);           // Escreve texto no display        
        ssd1306_draw_string(&ssd, str_pitch, 73, 52);        // Exibe Pitch
        ssd1306_send_data(&ssd);
        sleep_ms(500);
    }
}

void gpio_setup(){
    gpio_init(botaoA);
    gpio_set_dir(botaoA, GPIO_IN);
    gpio_pull_up(botaoA);

    gpio_init(botaoB);
    gpio_set_dir(botaoB, GPIO_IN);
    gpio_pull_up(botaoB);
    
    gpio_init(botaoJoy);
    gpio_set_dir(botaoJoy, GPIO_IN);
    gpio_pull_up(botaoJoy);
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    reset_usb_boot(0, 0);
}
