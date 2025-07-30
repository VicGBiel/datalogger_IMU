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

// Definição dos botões, buzzer e leds
#define botaoA 5
#define botaoB 6
#define botaoJoy 22
#define LED_R 13
#define LED_G 11
#define LED_B 12
#define BUZZER 10

// Definição dos pinos I2C para o MPU6050
#define I2C_PORT i2c0                 // I2C0 usa pinos 0 e 1
#define I2C_SDA 0
#define I2C_SCL 1

// Definição dos pinos I2C para o display OLED
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define ENDERECO_DISP 0x3C            // Endereço I2C do display

// Estrutura para dados do sensor
typedef struct {
    float accel[3];
    int16_t gyro[3];
    float temp;
    uint32_t timestamp;
} SensorData;

// Estados do sistema
typedef enum {
    STATE_INIT,
    STATE_READY,
    STATE_RECORDING,
    STATE_SAVING,
    STATE_ERROR
} SystemState;

SystemState current_state = STATE_INIT;

// Endereço padrão do MPU6050
static int addr = 0x68;

static bool is_recording = 0;
static bool is_mounted = 0;
unsigned long sample_count = 0; 
static bool is_first_sample = true;
static volatile uint32_t last_time = 0;
static volatile bool flag_mount_toggle = false;
static volatile bool flag_record_toggle = false;

static char filename[20] = "data_log.csv";

// Protótipos das funções
void gpio_setup();
void gpio_irq_handler(uint gpio, uint32_t events);
void beep(int times);
void read_sensor_data(SensorData *data);
void capture_to_file(const char *filename);
void update_led();
void update_display(ssd1306_t *disp);

int main(){
    stdio_init_all();
    gpio_setup();

    gpio_set_irq_enabled_with_callback(botaoA, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
    gpio_set_irq_enabled_with_callback(botaoJoy, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

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

    bool cor = true;    

    while (1)
    {
        if (flag_mount_toggle) {
            flag_mount_toggle = false;
            is_mounted = !is_mounted;
            if (is_mounted) {
                sd_mount();
                current_state = STATE_READY;
            } else {
                sd_unmount();
                current_state = STATE_INIT;
            }
        }

        if (flag_record_toggle) {
            sample_count = 0;  // Reset do contador ao iniciar nova gravação
            flag_record_toggle = false;
            is_recording = !is_recording;
            
            if (is_recording) {
                current_state = STATE_RECORDING;
                beep(1);
            } else {
                current_state = STATE_SAVING;
                beep(2);
            }
        }
        
        if (is_recording) {
            capture_to_file(filename);
        }

        update_led();
        update_display(&ssd); 
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

    gpio_init(LED_B);
    gpio_set_dir(LED_B, GPIO_OUT);

    gpio_init(LED_R);
    gpio_set_dir(LED_R, GPIO_OUT);

    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);

    gpio_init(BUZZER);
    gpio_set_dir(BUZZER, GPIO_OUT);
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
    uint32_t current_time = to_us_since_boot(get_absolute_time());
    if (current_time - last_time > 200000) { // 200ms de debounce
        last_time = current_time;

        if (gpio == botaoA) {
            flag_record_toggle = true;
        } else if (gpio == botaoB){
            flag_mount_toggle = true;
        } else {
            reset_usb_boot(0, 0); // entra em bootsel
        }
    }
}

void beep(int times){
    for (int i = 0; i < times; i++) {
        gpio_put(BUZZER, 1);
        sleep_ms(100);
        gpio_put(BUZZER, 0);
        sleep_ms(100);
    }
}

void read_sensor_data(SensorData *data) {
    int16_t raw_accel[3], raw_gyro[3], raw_temp;
    
    mpu6050_read_raw(raw_accel, raw_gyro, &raw_temp, I2C_PORT, addr);
    
    // Conversão para "g" (9.81 m/s^2)
    data->accel[0] = raw_accel[0] / 16384.0f;
    data->accel[1] = raw_accel[1] / 16384.0f;
    data->accel[2] = raw_accel[2] / 16384.0f;
    
    // Conversão para graus por segundo (°/s)
    data->gyro[0] = raw_gyro[0]/131.0f;
    data->gyro[1] = raw_gyro[1]/131.0f;
    data->gyro[2] = raw_gyro[2]/131.0f;
    
    data->temp = raw_temp / 340.0f + 36.53f; // Conversão para Celsius
    data->timestamp = to_ms_since_boot(get_absolute_time());
}

void capture_to_file(const char *filename){
    static FIL file;
    static bool file_open = false;
    
    if (!is_recording){
        f_close(&file);
        file_open = false;
        return;
    }

    if (!file_open) {
        if (f_open(&file, filename, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK) {
            current_state = STATE_ERROR;
            return;
        }
        f_printf(&file, "sample,time,accel_x,accel_y,accel_z,gyro_x,gyro_y,gyro_z,temp\n");
        file_open = true;
        sample_count = 0;  // Reset do contador ao iniciar nova gravação
    }
    
    SensorData data;
    read_sensor_data(&data);
    
    f_printf(&file, "%lu,%lu,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.1f\n",
             sample_count++, data.timestamp,
             data.accel[0], data.accel[1], data.accel[2],
             data.gyro[0], data.gyro[1], data.gyro[2],
             data.temp);
    
    if (sample_count % 10 == 0) {
        f_sync(&file);
    }
}

void update_led() {
    switch(current_state) {
        case STATE_INIT:
            gpio_put(LED_R, 1); gpio_put(LED_G, 1); gpio_put(LED_B, 0); // Amarelo
            break;
        case STATE_READY:
            gpio_put(LED_R, 0); gpio_put(LED_G, 1); gpio_put(LED_B, 0); // Verde
            break;
        case STATE_RECORDING:
            gpio_put(LED_R, 1); gpio_put(LED_G, 0); gpio_put(LED_B, 0); // Vermelho
            break;
        case STATE_SAVING:
            // Piscar azul
            static bool led_state = false;
            led_state = !led_state;
            gpio_put(LED_R, 0); gpio_put(LED_G, 0); gpio_put(LED_B, led_state);
            break;
        case STATE_ERROR:
            // Piscar roxo
            static bool err_led_state = false;
            err_led_state = !err_led_state;
            gpio_put(LED_R, err_led_state); gpio_put(LED_G, 0); gpio_put(LED_B, err_led_state);
            break;
    }
}

void update_display(ssd1306_t *disp) {
    ssd1306_fill(disp, false);
    
    const char *status_msg;
    switch(current_state) {
        case STATE_INIT: status_msg = "Inicializando"; break;
        case STATE_READY: status_msg = "Pronto"; break;
        case STATE_RECORDING: status_msg = "Gravando..."; break;
        case STATE_SAVING: status_msg = "Salvando..."; break;
        case STATE_ERROR: status_msg = "ERRO SD"; break;
    }
    ssd1306_draw_string(disp, status_msg, 0, 0);
    
    if (current_state == STATE_RECORDING) {
        char sample_str[20];
        snprintf(sample_str, sizeof(sample_str), "Amostras: %lu", sample_count);
        ssd1306_draw_string(disp, sample_str, 0, 16);
    }
    
    const char *sd_status = is_mounted ? "SD: Montado" : "SD: Desmontado";
    ssd1306_draw_string(disp, sd_status, 0, 32);
    
    ssd1306_send_data(disp);
}