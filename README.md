# Datalogger de Movimento com Feedback Inteligente

Projeto desenvolvido para registrar dados de movimento utilizando o sensor IMU MPU6050 e armazená-los em um cartão SD no formato `.csv`, com feedback visual e sonoro usando os recursos da plataforma BitDogLab.

---

## Objetivo

Criar um dispositivo portátil baseado no Raspberry Pi Pico W que:

- Capture dados de aceleração e velocidade angular do sensor MPU6050;
- Armazene os dados em um cartão microSD (.csv);
- Forneça feedback em tempo real via:
  - Display OLED SSD1306;
  - LED RGB indicando o estado do sistema;
  - Buzzer com alertas sonoros;
  - Botões físicos para controle total.

---

### Hardware (BitDogLab / RP2040)
- MPU6050 (IMU: Acelerômetro e Giroscópio)
- Display OLED SSD1306 (I2C)
- Cartão MicroSD
- LED RGB
- Buzzer
- 3 Botões físicos

### Software
- Linguagem: C (SDK Pico)
- Bibliotecas:
  - `ssd1306.h` e `font.h` (display)
  - `mpu6050.h` (sensor IMU)
  - `sdcard_utils.h` (gravação SD)
- Script Python para visualização dos dados (`plot_datalogger.py`)

---

## Funcionamento

### Fluxo Geral:

1. **Inicialização**: sistema monta o SD e exibe status no display OLED;
2. **Controle por Botões**:
   - Botão A: Inicia / Para a gravação;
   - Botão B: Monta / Desmonta o cartão SD com segurança;
   - Botão Joy: Entra em modo bootloader (reset USB);
3. **Gravação**:
   - Dados do MPU6050 são coletados e salvos em um arquivo `.csv` (com nome dinâmico);
   - O LED RGB indica o estado atual (veja abaixo);
   - O display mostra mensagens como "Gravando..." e número de amostras;
   - O buzzer emite 1 beep ao iniciar, 2 beeps ao parar.

### 💡 LED RGB - Estados:
| Estado           | Cor no LED       |
|------------------|------------------|
| Inicializando    | Amarelo          |
| Pronto           | Verde            |
| Gravando         | Vermelho         |
| Salvando         | Azul (piscando)  |
| Erro no SD       | Roxo (piscando)  |
