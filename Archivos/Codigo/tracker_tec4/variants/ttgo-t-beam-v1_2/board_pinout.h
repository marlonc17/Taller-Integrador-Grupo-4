#ifndef BOARD_PINOUT_H_
#define BOARD_PINOUT_H_

// =====================================================
// Pines verificados contra esquematico oficial
// LilyGo T22_GPS_V1.1 con AXP2101
// =====================================================

// LoRa SX1276 — bus SPI
#define RADIO_SCLK_PIN      5
#define RADIO_MISO_PIN      19
#define RADIO_MOSI_PIN      27
#define RADIO_CS_PIN        18
#define RADIO_RST_PIN       23
#define RADIO_DIO0_PIN      26      // IRQ fin de transmision

// GPS NEO-6M/M8N — bus UART2
#define GPS_RX_PIN          34      // ESP32 recibe (solo entrada)
#define GPS_TX_PIN          12      // ESP32 envia al GPS

// AXP2101 — bus I2C
#define I2C_SDA             21
#define I2C_SCL             22
#define PMU_IRQ_PIN         35      // Interrupcion del PMIC
#define PMU_PWRON_PIN       38      // Power on del PMIC (no usar como boton)

#endif
