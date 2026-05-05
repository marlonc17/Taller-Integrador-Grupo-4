// =====================================================
// Tracker TI0TEC4-7 — T-Beam AXP2101 V1.2
// Taller Integrador — Escuela de Ingeniería en Electrónica
// Marlon Cordero Sandoval — Rolando Vega Marino
// =====================================================
//
// Firmware propio basado en la FSM definida en el informe parcial.
// Formato de trama: CALLSIGN,LAT,LON,VEL,RUM,*CHK
// El formato APRS completo se implementa en semanas 10-12.
// =====================================================

#include <Arduino.h>
#include <Wire.h>
#include <SPI.h>
#include <TinyGPSPlus.h>
#include "RadioLibConfig.h"    // debe ir ANTES de RadioLib.h
#include <RadioLib.h>
#include <XPowersLib.h>

#include "board_pinout.h"
#include "config.h"

// =====================================================
// ESTADOS DE LA FSM
// =====================================================
enum Estado {
    INIT,
    LOAD_CONFIG,
    CHECK_HW,
    SLEEP,
    GPS_ON,
    SENSING,
    BUILD_PACKET,
    TX_DATA,
    WAIT_TX,
    ERROR_RETRY,
    FALLO_CRITICO
};

// =====================================================
// VARIABLES GLOBALES DE ESTADO
// =====================================================
Estado estadoActual     = INIT;
Estado estadoFallo      = SLEEP;    // estado donde ocurrio el ultimo fallo
int    contadorReintentos = 0;
bool   enMovimiento     = false;
uint64_t timerProximoCiclo = INTERVALO_REPOSO;

// =====================================================
// DATOS GPS
// =====================================================
double  latitud     = 0.0;
double  longitud    = 0.0;
double  velocidad   = 0.0;
double  rumbo       = 0.0;

// =====================================================
// TRAMA DE TRANSMISION
// =====================================================
String trama = "";

// =====================================================
// FLAG DE TRANSMISION (seteado por interrupcion DIO0)
// =====================================================
volatile bool txCompletado = false;

// =====================================================
// INSTANCIAS DE HARDWARE
// =====================================================
TinyGPSPlus         gps;
XPowersAXP2101      PMU;
SX1278              radio = new Module(
                        RADIO_CS_PIN,
                        RADIO_DIO0_PIN,
                        RADIO_RST_PIN,
                        RADIO_DIO0_PIN  // DIO1 no usado, mismo pin
                    );

// =====================================================
// PROTOTIPOS DE FUNCIONES
// =====================================================
void estadoInit();
void estadoLoadConfig();
void estadoCheckHW();
void estadoSleep();
void estadoGpsOn();
void estadoSensing();
void estadoBuildPacket();
void estadoTxData();
void estadoWaitTx();
void estadoErrorRetry();
void falloCritico();

bool    inicializarAXP2101();
bool    inicializarSX1278();
void    activarGPS();
void    desactivarGPS();
uint8_t calcularChecksum(const String& datos);
void    IRAM_ATTR onTxCompletado();

// =====================================================
// SETUP — FASE DE ARRANQUE
// =====================================================
void setup() {

    // ── INIT ──
    estadoActual = INIT;

    Serial.begin(115200);
    delay(500);
    Serial.println("[INIT] Iniciando firmware tracker TI0TEC4-7");

    // Inicializar bus I2C para AXP2101
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.println("[INIT] Bus I2C inicializado (SDA=21, SCL=22)");

    // Inicializar bus SPI para SX1278
    SPI.begin(RADIO_SCLK_PIN, RADIO_MISO_PIN, RADIO_MOSI_PIN, RADIO_CS_PIN);
    Serial.println("[INIT] Bus SPI inicializado");

    // Configurar DIO0 como entrada de interrupcion
    pinMode(RADIO_DIO0_PIN, INPUT);
    Serial.println("[INIT] GPIO26 configurado como IRQ DIO0");

    estadoActual = LOAD_CONFIG;

    // ── LOAD_CONFIG ──
    // Los parametros ya estan definidos en config.h como constantes.
    // En versiones futuras se pueden cargar desde SPIFFS o EEPROM.
    contadorReintentos  = 0;
    enMovimiento        = false;
    timerProximoCiclo   = INTERVALO_REPOSO;

    Serial.println("[LOAD_CONFIG] Parametros cargados:");
    Serial.printf("  Callsign:            %s\n",    CALLSIGN);
    Serial.printf("  Frecuencia LoRa:     %.3f MHz\n", LORA_FRECUENCIA / 1e6);
    Serial.printf("  Spreading Factor:    SF%d\n",  LORA_SF);
    Serial.printf("  Bandwidth:           %.0f kHz\n", LORA_BW / 1000.0);
    Serial.printf("  Potencia TX:         %d dBm\n", LORA_POTENCIA);
    Serial.printf("  Intervalo movimiento:%d min\n", (int)(INTERVALO_MOVIMIENTO / 60000000ULL));
    Serial.printf("  Intervalo reposo:    %d h\n",   (int)(INTERVALO_REPOSO / 3600000000ULL));
    Serial.printf("  Umbral velocidad:    %.1f km/h\n", UMBRAL_VELOCIDAD);

    estadoActual = CHECK_HW;

    // ── CHECK_HW ──
    Serial.println("[CHECK_HW] Verificando hardware...");

    // Verificar e inicializar AXP2101
    if (!inicializarAXP2101()) {
        Serial.println("[CHECK_HW] ERROR: AXP2101 no responde");
        estadoActual = FALLO_CRITICO;
        falloCritico();
        return;
    }
    Serial.println("[CHECK_HW] AXP2101 OK");

    // Verificar e inicializar SX1278 con parametros de config
    if (!inicializarSX1278()) {
        Serial.println("[CHECK_HW] ERROR: SX1278 no responde");
        estadoActual = FALLO_CRITICO;
        falloCritico();
        return;
    }
    Serial.println("[CHECK_HW] SX1278 OK");

    // Registrar interrupcion de fin de transmision
    radio.setPacketSentAction(onTxCompletado);

    Serial.println("[CHECK_HW] Hardware verificado. Iniciando ciclo periodico.");
    estadoActual = SLEEP;

    // Iniciar GPS serial en UART2
    Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial.println("[CHECK_HW] GPS UART2 iniciado (RX=34, TX=12)");
}

// =====================================================
// LOOP — CICLO PERIODICO (despachador de FSM)
// =====================================================
void loop() {
    switch (estadoActual) {
        case SLEEP:         estadoSleep();          break;
        case GPS_ON:        estadoGpsOn();           break;
        case SENSING:       estadoSensing();         break;
        case BUILD_PACKET:  estadoBuildPacket();     break;
        case TX_DATA:       estadoTxData();          break;
        case WAIT_TX:       estadoWaitTx();          break;
        case ERROR_RETRY:   estadoErrorRetry();      break;
        case FALLO_CRITICO: falloCritico();          break;
        default:
            Serial.println("[FSM] Estado desconocido -> FALLO_CRITICO");
            estadoActual = FALLO_CRITICO;
            break;
    }
}

// =====================================================
// FUNCIONES DE CADA ESTADO
// =====================================================

// ── SLEEP ──
// Apaga LED, reinicia reintentos, entra en Deep Sleep.
// Al despertar por timer el ESP32 continua desde aqui.
void estadoSleep() {
    Serial.printf("[SLEEP] Proximo ciclo en %.0f segundos\n",
                  timerProximoCiclo / 1000000.0);
    Serial.flush();

    contadorReintentos = 0;
    txCompletado       = false;

    // Configurar despertar por timer
    esp_sleep_enable_timer_wakeup(timerProximoCiclo);

    // Entrar en Deep Sleep
    // Al despertar el ESP32 ejecuta setup() nuevamente.
    // Para mantener el estado entre ciclos se usa RTC memory
    // (mejora para implementacion futura).
    esp_deep_sleep_start();

    // Esta linea no se ejecuta hasta el proximo despertar
    estadoActual = GPS_ON;
}

// ── GPS_ON ──
// Energiza el GPS mediante AXP2101 por I2C.
// Espera que el modulo este listo con timeout.
void estadoGpsOn() {
    Serial.println("[GPS_ON] Energizando GPS via AXP2101...");
    activarGPS();

    // Esperar respuesta del GPS con timeout
    unsigned long inicio = millis();
    bool gpsResponde     = false;

    while (millis() - inicio < TIMEOUT_GPS) {
        if (Serial2.available()) {
            gpsResponde = true;
            break;
        }
        delay(100);
    }

    if (!gpsResponde) {
        Serial.println("[GPS_ON] ERROR: timeout sin respuesta del GPS");
        estadoFallo  = GPS_ON;
        estadoActual = ERROR_RETRY;
        return;
    }

    Serial.println("[GPS_ON] GPS responde. Pasando a SENSING.");
    estadoActual = SENSING;
}

// ── SENSING ──
// Lee sentencias NMEA por UART2.
// Extrae latitud, longitud, velocidad y rumbo.
// Valida que el fix sea valido con timeout.
void estadoSensing() {
    Serial.println("[SENSING] Esperando fix GPS...");

    unsigned long inicio   = millis();
    bool datosValidos      = false;

    while (millis() - inicio < TIMEOUT_GPS) {
        // Leer y parsear NMEA
        while (Serial2.available()) {
            gps.encode(Serial2.read());
        }

        // Verificar que el fix sea valido y los datos coherentes
        if (gps.location.isValid() &&
            gps.location.isUpdated() &&
            gps.speed.isValid() &&
            gps.course.isValid()) {

            latitud   = gps.location.lat();
            longitud  = gps.location.lng();
            velocidad = gps.speed.kmph();
            rumbo     = gps.course.deg();

            // Validacion basica de coherencia
            if (latitud  >= -90.0  && latitud  <= 90.0  &&
                longitud >= -180.0 && longitud <= 180.0 &&
                velocidad >= 0.0) {
                datosValidos = true;
                break;
            }
        }
        delay(100);
    }

    if (!datosValidos) {
        Serial.println("[SENSING] ERROR: timeout sin datos validos");
        estadoFallo  = SENSING;
        estadoActual = ERROR_RETRY;
        return;
    }

    Serial.printf("[SENSING] Fix OK — Lat: %.6f  Lon: %.6f  Vel: %.1f km/h  Rum: %.1f\n",
                  latitud, longitud, velocidad, rumbo);
    estadoActual = BUILD_PACKET;
}

// ── BUILD_PACKET ──
// Construye la trama propia para la entrega parcial:
// CALLSIGN,LAT,LON,VEL,RUM,*CHK
//
// Nota: el formato APRS completo (APLT00, WIDE1-1,
// conversion a grados y minutos) se implementa en semanas 10-12.
void estadoBuildPacket() {

    // Construir trama sin checksum
    String datos = String(CALLSIGN) + "," +
                   String(latitud,  6)  + "," +
                   String(longitud, 6)  + "," +
                   String((int)velocidad) + "," +
                   String((int)rumbo);

    // Calcular checksum XOR sobre todos los bytes de datos
    uint8_t chk = calcularChecksum(datos);

    // Armar trama completa
    trama = datos + ",*" + String(chk, HEX);
    trama.toUpperCase();

    Serial.printf("[BUILD_PACKET] Trama: %s\n", trama.c_str());

    // Evaluar movimiento para determinar proximo intervalo de SLEEP
    if (velocidad > UMBRAL_VELOCIDAD) {
        timerProximoCiclo = INTERVALO_MOVIMIENTO;
        enMovimiento      = true;
        Serial.println("[BUILD_PACKET] Estado: en movimiento -> proximo ciclo en 5 min");
    } else {
        timerProximoCiclo = INTERVALO_REPOSO;
        enMovimiento      = false;
        Serial.println("[BUILD_PACKET] Estado: detenido -> proximo ciclo en 1 hora");
    }

    estadoActual = TX_DATA;
}

// ── TX_DATA ──
// Apaga GPS para reducir consumo durante TX.
// Inicia transmision LoRa en modo no bloqueante.
void estadoTxData() {
    Serial.println("[TX_DATA] Apagando GPS...");
    desactivarGPS();

    Serial.printf("[TX_DATA] Transmitiendo: %s\n", trama.c_str());
    txCompletado = false;

    // Iniciar transmision no bloqueante
    int estado = radio.startTransmit(trama);

    if (estado != RADIOLIB_ERR_NONE) {
        Serial.printf("[TX_DATA] ERROR al iniciar TX: %d\n", estado);
        estadoFallo  = TX_DATA;
        estadoActual = ERROR_RETRY;
        return;
    }

    estadoActual = WAIT_TX;
}

// ── WAIT_TX ──
// Espera la interrupcion DIO0 que indica fin de transmision.
// Si llega dentro del timeout: exito -> SLEEP.
// Si expira el timeout: error -> ERROR_RETRY.
void estadoWaitTx() {
    unsigned long inicio = millis();

    while (millis() - inicio < TIMEOUT_TX) {
        if (txCompletado) {
            Serial.println("[WAIT_TX] TX completado OK");
            estadoActual = SLEEP;
            return;
        }
        delay(10);
    }

    Serial.println("[WAIT_TX] ERROR: timeout esperando DIO0");
    estadoFallo  = WAIT_TX;
    estadoActual = ERROR_RETRY;
}

// ── ERROR_RETRY ──
// Incrementa contador de reintentos.
// Si hay reintentos disponibles: vuelve al estado que fallo.
// Si se agotaron: fuerza SLEEP con intervalo largo.
void estadoErrorRetry() {
    contadorReintentos++;

    Serial.printf("[ERROR_RETRY] Intento %d de %d — fallo en estado %d\n",
                  contadorReintentos, MAX_REINTENTOS, estadoFallo);

    if (contadorReintentos < MAX_REINTENTOS) {
        estadoActual = estadoFallo;
        return;
    }

    // Reintentos agotados — forzar SLEEP con intervalo largo
    Serial.println("[ERROR_RETRY] Reintentos agotados -> SLEEP forzado");
    timerProximoCiclo = INTERVALO_REPOSO;
    estadoActual      = SLEEP;
}

// ── FALLO_CRITICO ──
// Estado sin recuperacion.
// Parpadea el LED del GPS (LED2) indefinidamente como indicador visual.
void falloCritico() {
    Serial.println("[FALLO_CRITICO] El sistema no puede continuar.");
    while (true) {
        // LED2 es el indicador de GPS en el T-Beam
        // Parpadeo rapido como indicador de error
        delay(200);
    }
}

// =====================================================
// FUNCIONES AUXILIARES
// =====================================================

// Inicializa el AXP2101 y configura los rails de alimentacion
bool inicializarAXP2101() {
    if (!PMU.begin(Wire, AXP2101_SLAVE_ADDRESS, I2C_SDA, I2C_SCL)) {
        return false;
    }

    // Apagar rails no necesarios
    PMU.disableDC2();
    PMU.disableDC3();
    PMU.disableDC4();
    PMU.disableDC5();
    PMU.disableALDO1();
    PMU.disableALDO4();
    PMU.disableBLDO1();
    PMU.disableBLDO2();
    PMU.disableDLDO1();
    PMU.disableDLDO2();

    // Mantener DC1 para el ESP32
    PMU.setDC1Voltage(3300);
    PMU.enableDC1();

    // Activar LoRa (ALDO2 segun power_utils.cpp del repositorio referencia)
    PMU.setALDO2Voltage(3300);
    PMU.enableALDO2();

    // Configurar carga de bateria
    PMU.setPrechargeCurr(XPOWERS_AXP2101_PRECHARGE_200MA);
    PMU.setChargerTerminationCurr(XPOWERS_AXP2101_CHG_ITERM_25MA);
    PMU.setChargeTargetVoltage(XPOWERS_AXP2101_CHG_VOL_4V2);
    PMU.setChargerConstantCurr(XPOWERS_AXP2101_CHG_CUR_800MA);
    PMU.setSysPowerDownVoltage(2600);

    PMU.disableIRQ(XPOWERS_AXP2101_ALL_IRQ);

    return true;
}

// Inicializa el SX1278 con los parametros LoRa del informe
bool inicializarSX1278() {
    int estado = radio.begin(
        LORA_FRECUENCIA / 1e6,  // frecuencia en MHz
        LORA_BW / 1e3,          // bandwidth en kHz
        LORA_SF,                // spreading factor
        LORA_CR,                // coding rate
        LORA_SYNC_WORD,         // sync word
        LORA_POTENCIA           // potencia en dBm
    );

    if (estado != RADIOLIB_ERR_NONE) {
        Serial.printf("[CHECK_HW] RadioLib error: %d\n", estado);
        return false;
    }

    return true;
}

// Energiza el GPS via AXP2101 (ALDO3 segun power_utils.cpp del repo referencia)
void activarGPS() {
    PMU.setALDO3Voltage(3300);
    PMU.enableALDO3();
    Serial.println("[PWR] GPS energizado via ALDO3");
}

// Desenergiza el GPS via AXP2101
void desactivarGPS() {
    PMU.disableALDO3();
    Serial.println("[PWR] GPS apagado via ALDO3");
}

// Calcula checksum XOR sobre todos los bytes del string
uint8_t calcularChecksum(const String& datos) {
    uint8_t chk = 0;
    for (int i = 0; i < datos.length(); i++) {
        chk ^= (uint8_t)datos[i];
    }
    return chk;
}

// Interrupcion de fin de transmision — llamada por RadioLib cuando DIO0 se activa
void IRAM_ATTR onTxCompletado() {
    txCompletado = true;
}
