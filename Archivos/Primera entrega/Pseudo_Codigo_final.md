# Pseudocódigo — Tracker T-Beam AXP2101 V1.2

## Descripción general

Este pseudocódigo representa la máquina de estados finitos del firmware del tracker.  
El flujo principal prioriza el ahorro de energía, la adquisición de posición GPS y la transmisión LoRa, garantizando siempre un camino de retorno hacia `SLEEP`, incluso ante fallos recuperables.

## Pseudocódigo

```cpp
// =====================================================
// PSEUDOCODIGO — Tracker T-Beam AXP2101 V1.2
// =====================================================


// =====================================================
// 1. CONFIGURACION GLOBAL
// =====================================================

CONFIGURACION:
    INTERVALO_MOVIMIENTO = 5 minutos
    INTERVALO_REPOSO     = 1 hora
    UMBRAL_VELOCIDAD     = 5 km/h
    TIMEOUT_GPS          = 60 segundos
    TIMEOUT_TX           = 5 segundos
    MAX_REINTENTOS       = 3

PARAMETROS_LORA:
    LORA_FRECUENCIA = 433775000 Hz
    LORA_SF         = 12
    LORA_BW         = 125000 Hz
    LORA_CR         = 5
    LORA_POTENCIA   = 20 dBm
    LORA_SYNC_WORD  = 0x12

IDENTIFICACION:
    CALLSIGN = "TI0TEC4-7"


// =====================================================
// 2. ESTADOS Y VARIABLES
// =====================================================

ESTADOS_POSIBLES:
    INIT
    LOAD_CONFIG
    CHECK_HW
    SLEEP
    GPS_ON
    SENSING
    BUILD_PACKET
    TX_DATA
    WAIT_TX
    ERROR_RETRY
    FALLO_CRITICO

VARIABLES_ESTADO:
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

DATOS_TRANSMISION:
    trama
    checksum


// =====================================================
// 3. FASE DE ARRANQUE
// =====================================================

funcion setup():

    estado_actual = INIT

    // -------------------------------------------------
    // ESTADO: INIT
    // -------------------------------------------------
    inicializar Serial
    inicializar bus I2C con SDA = GPIO21 y SCL = GPIO22
    inicializar bus SPI
    configurar GPIO26 como entrada de interrupcion para DIO0

    si ocurre error en alguna inicializacion:
        estado_actual = FALLO_CRITICO
        ejecutar fallo_critico()
        detener ejecucion

    estado_actual = LOAD_CONFIG

    // -------------------------------------------------
    // ESTADO: LOAD_CONFIG
    // -------------------------------------------------
    cargar parametros definidos en CONFIGURACION
    cargar parametros definidos en PARAMETROS_LORA

    contador_reintentos  = 0
    en_movimiento        = falso
    timer_proximo_ciclo  = INTERVALO_REPOSO

    estado_actual = CHECK_HW

    // -------------------------------------------------
    // ESTADO: CHECK_HW
    // -------------------------------------------------
    verificar AXP2101 mediante I2C
    verificar SX1276 mediante SPI

    configurar SX1276:
        frecuencia = LORA_FRECUENCIA
        sf         = LORA_SF
        bw         = LORA_BW
        cr         = LORA_CR
        potencia   = LORA_POTENCIA
        sync_word  = LORA_SYNC_WORD

    si AXP2101 o SX1276 fallan:
        estado_actual = FALLO_CRITICO
        ejecutar fallo_critico()
        detener ejecucion

    estado_actual = SLEEP


// =====================================================
// 4. CICLO PRINCIPAL
// =====================================================

funcion loop():

    segun estado_actual:

        caso SLEEP:
            ejecutar estado_sleep()

        caso GPS_ON:
            ejecutar estado_gps_on()

        caso SENSING:
            ejecutar estado_sensing()

        caso BUILD_PACKET:
            ejecutar estado_build_packet()

        caso TX_DATA:
            ejecutar estado_tx_data()

        caso WAIT_TX:
            ejecutar estado_wait_tx()

        caso ERROR_RETRY:
            ejecutar estado_error_retry()

        caso FALLO_CRITICO:
            ejecutar fallo_critico()

        caso otro:
            estado_actual = FALLO_CRITICO


// =====================================================
// 5. FUNCIONES POR ESTADO
// =====================================================

funcion estado_sleep():

    apagar LED
    contador_reintentos = 0

    configurar timer de Deep Sleep con timer_proximo_ciclo
    entrar en Deep Sleep

    // Al despertar por timer:
    estado_actual = GPS_ON


funcion estado_gps_on():

    enviar comando I2C a AXP2101 para energizar GPS
    esperar respuesta del GPS durante TIMEOUT_GPS

    si no hay respuesta antes del timeout:
        estado_fallo  = GPS_ON
        estado_actual = ERROR_RETRY
        retornar

    estado_actual = SENSING


funcion estado_sensing():

    leer sentencias NMEA por UART

    extraer:
        latitud
        longitud
        velocidad
        rumbo

    validar coherencia de datos GPS durante TIMEOUT_GPS

    si no hay datos validos antes del timeout:
        estado_fallo  = SENSING
        estado_actual = ERROR_RETRY
        retornar

    estado_actual = BUILD_PACKET


funcion estado_build_packet():

    // Formato propio para entrega parcial:
    // CALLSIGN,LAT,LON,VEL,RUM,*CHK

    trama = construir_trama_propia(
                CALLSIGN,
                latitud,
                longitud,
                velocidad,
                rumbo
            )

    checksum = calcular_checksum_xor(trama)
    agregar checksum a trama

    // Nota:
    // El formato APRS completo se implementara en una fase posterior.
    // Incluira destino APLT00, path WIDE1-1 y conversion de coordenadas
    // a grados y minutos decimales.

    si velocidad > UMBRAL_VELOCIDAD:
        timer_proximo_ciclo = INTERVALO_MOVIMIENTO
        en_movimiento       = verdadero
    sino:
        timer_proximo_ciclo = INTERVALO_REPOSO
        en_movimiento       = falso

    registrar estado de movimiento actual
    estado_actual = TX_DATA


funcion estado_tx_data():

    enviar comando I2C a AXP2101 para apagar GPS
    encender LED
    iniciar transmision LoRa por SX1276 en modo no bloqueante

    estado_actual = WAIT_TX


funcion estado_wait_tx():

    esperar interrupcion en GPIO26, DIO0, durante TIMEOUT_TX

    si DIO0 se activa antes del timeout:
        apagar LED
        estado_actual = SLEEP
        retornar

    si timeout expira sin interrupcion:
        estado_fallo  = WAIT_TX
        estado_actual = ERROR_RETRY
        retornar


funcion estado_error_retry():

    contador_reintentos = contador_reintentos + 1

    si contador_reintentos < MAX_REINTENTOS:
        estado_actual = estado_fallo
        retornar

    // Si se agotan los reintentos:
    timer_proximo_ciclo = INTERVALO_REPOSO
    estado_actual       = SLEEP


funcion fallo_critico():

    mientras verdadero:
        parpadear LED rapidamente
        mantener sistema detenido
```