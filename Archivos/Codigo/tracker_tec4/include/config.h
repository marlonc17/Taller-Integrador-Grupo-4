#ifndef CONFIG_H_
#define CONFIG_H_

// =====================================================
// IDENTIFICACION DEL NODO
// =====================================================
#define CALLSIGN            "TI0TEC4-7"

// =====================================================
// PARAMETROS LORA — verificados en hardware
// =====================================================
#define LORA_FRECUENCIA     433775000   // Hz
#define LORA_SF             12          // Spreading Factor
#define LORA_BW             125000.0    // Bandwidth Hz
#define LORA_CR             5           // Coding Rate 4/5
#define LORA_POTENCIA       20          // dBm
#define LORA_SYNC_WORD      0x12        // Sync word APRS LoRa

// =====================================================
// PARAMETROS OPERATIVOS
// =====================================================
#define INTERVALO_MOVIMIENTO    (5  * 60 * 1000000ULL)  // 5 min en microsegundos
#define INTERVALO_REPOSO        (60 * 60 * 1000000ULL)  // 1 hora en microsegundos
#define UMBRAL_VELOCIDAD        5.0     // km/h
#define TIMEOUT_GPS             60000   // ms
#define TIMEOUT_TX              5000    // ms
#define MAX_REINTENTOS          3

// =====================================================
// VELOCIDAD SERIAL GPS
// =====================================================
#define GPS_BAUD            9600

#endif
