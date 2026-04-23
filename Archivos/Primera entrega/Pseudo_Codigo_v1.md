```cpp

// Variables Globales

CONFIGURACION:
  INTERVALO_MOVIMIENTO  = 5 minutos
  INTERVALO_REPOSO      = 1 hora
  UMBRAL_VELOCIDAD      = 5 km/h
  TIMEOUT_GPS           = 60 segundos
  TIMEOUT_TX            = 5 segundos
  MAX_REINTENTOS        = 3

  // Parámetros LoRa verificados en hardware
  LORA_FRECUENCIA       = 433775000 Hz
  LORA_SF               = 12
  LORA_BW               = 125000 Hz
  LORA_CR               = 5
  LORA_POTENCIA         = 20 dBm
  LORA_SYNC_WORD        = 0x12

  // Identificación del nodo
  CALLSIGN              = "TI0TEC4-7"

ESTADO:
  estado_actual
  estado_fallo
  contador_reintentos
  en_movimiento
  timer_proximo_ciclo

DATOS_GPS:
  latitud
  longitud
  velocidad
  rumbo

// Fase de Arranque 

función setup():

  estado_actual = INIT
  
  // ── INIT ──
  inicializar Serial
  inicializar bus I2C  (SDA=GPIO21, SCL=GPIO22)
  inicializar bus SPI
  configurar GPIO26 como entrada de interrupción (DIO0)

  si alguna inicialización falla:
    estado_actual = FALLO_CRÍTICO
    parpadear LED indefinidamente
    detener ejecución

  estado_actual = LOAD_CONFIG

  // ── LOAD_CONFIG ──
  cargar parámetros operativos definidos en CONFIGURACION
  contador_reintentos  = 0
  en_movimiento        = falso
  timer_proximo_ciclo  = INTERVALO_REPOSO

  estado_actual = CHECK_HW

  // ── CHECK_HW ──
  verificar AXP2101 mediante comando I2C
  verificar SX1276 mediante comando SPI
  configurar SX1276:
    frecuencia   = LORA_FRECUENCIA
    SF           = LORA_SF
    BW           = LORA_BW
    CR           = LORA_CR
    potencia     = LORA_POTENCIA
    sync word    = LORA_SYNC_WORD

  si alguna verificación falla:
    estado_actual = FALLO_CRÍTICO
    parpadear LED indefinidamente
    detener ejecución

  estado_actual = SLEEP

// Ciclo periodico 

función loop():

  según estado_actual:

  // ── SLEEP ──
  caso SLEEP:
    apagar LED
    reiniciar contador_reintentos
    configurar timer de Deep Sleep con timer_proximo_ciclo
    entrar en Deep Sleep
    // al despertar continúa aquí
    estado_actual = GPS_ON

  // ── GPS_ON ──
  caso GPS_ON:
    enviar comando I2C a AXP2101 para energizar GPS
    esperar respuesta del GPS con timeout TIMEOUT_GPS

    si timeout expira sin respuesta:
      estado_fallo  = GPS_ON
      estado_actual = ERROR_RETRY
      retornar

    estado_actual = SENSING

  // ── SENSING ──
  caso SENSING:
    leer sentencias NMEA por UART
    extraer latitud, longitud, velocidad, rumbo
    validar coherencia de datos con timeout TIMEOUT_GPS

    si timeout expira sin datos válidos:
      estado_fallo  = SENSING
      estado_actual = ERROR_RETRY
      retornar

    estado_actual = BUILD_PACKET

  // ── BUILD_PACKET ──
  caso BUILD_PACKET:

    // Formato propio para entrega parcial:
    // CALLSIGN,LAT,LON,VEL,RUM,*CHK
    construir paquete:
      campo 1 = CALLSIGN          // "TI0TEC4-7"
      campo 2 = latitud           // grados decimales, 6 decimales
      campo 3 = longitud          // grados decimales, 6 decimales
      campo 4 = velocidad         // km/h, entero
      campo 5 = rumbo             // grados 0-359, entero
      campo 6 = checksum XOR      // "*XX"

    // Nota: formato APRS formal (APLT00, WIDE1-1, conversión a
    // grados y minutos) se implementa en semanas 10-12.

    si velocidad > UMBRAL_VELOCIDAD:
      timer_proximo_ciclo = INTERVALO_MOVIMIENTO
      en_movimiento       = verdadero
    sino:
      timer_proximo_ciclo = INTERVALO_REPOSO
      en_movimiento       = falso

    registrar estado de movimiento actual
    estado_actual = TX_DATA

  // ── TX_DATA ──
  caso TX_DATA:
    enviar comando I2C a AXP2101 para apagar GPS
    encender LED
    iniciar transmisión por SX1276 (no bloqueante)
    estado_actual = WAIT_TX

  // ── WAIT_TX ──
  caso WAIT_TX:
    esperar interrupción en GPIO26 (DIO0) con timeout TIMEOUT_TX

    si DIO0 se activa dentro del timeout:
      apagar LED
      estado_actual = SLEEP
      retornar

    si timeout expira sin DIO0:
      estado_fallo  = WAIT_TX
      estado_actual = ERROR_RETRY
      retornar

  // ── ERROR_RETRY ──
  caso ERROR_RETRY:
    contador_reintentos++

    si contador_reintentos < MAX_REINTENTOS:
      estado_actual = estado_fallo
      retornar

    // reintentos agotados
    timer_proximo_ciclo = INTERVALO_REPOSO
    estado_actual       = SLEEP

```