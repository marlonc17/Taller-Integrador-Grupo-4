# Tracker LoRa/APRS basado en T-Beam AXP2101 V1.2

Sistema de rastreo vehicular de bajo consumo basado en tecnología LoRa y GPS, desarrollado como proyecto del curso Taller Integrador.

El nodo adquiere información de posicionamiento geográfico mediante GPS y la transmite utilizando LoRa en la banda de 433 MHz, permitiendo la integración con redes APRS sin depender de infraestructura celular.

---

## Características principales

- Adquisición de posición mediante GPS.
- Comunicación LoRa de largo alcance.
- Arquitectura compatible con APRS.
- Gestión inteligente de energía mediante AXP2101.
- Máquina de estados finitos (FSM).
- Modos de bajo consumo utilizando Deep Sleep.
- Intervalos de transmisión adaptativos según movimiento.

---

## Arquitectura interna

El firmware se divide en tres subsistemas:

### Gestión de energía
Control del AXP2101 para:

- Encendido/apagado de periféricos.
- Gestión de batería.
- Optimización del consumo.

### Adquisición de datos

Obtención de:

- Latitud
- Longitud
- Velocidad
- Rumbo

mediante sentencias NMEA.

### Comunicación RF

Transmisión de datos mediante:

- SX1276
- LoRa 433 MHz

---

## Máquina de estados

El sistema utiliza una FSM para coordinar la operación.

Estados principales:

```
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
```

La FSM permite:

- Operación determinista.
- Manejo de errores.
- Reducción del consumo energético.

---

## Parámetros LoRa

| Parámetro | Valor |
|------------|---------|
| Frecuencia | 433.775 MHz |
| Spreading Factor | SF12 |
| Bandwidth | 125 kHz |
| Coding Rate | 4/5 |
| Potencia TX | 20 dBm |
| Sync Word | 0x12 |

---

## Estrategia de ahorro energético

El sistema utiliza:

- Deep Sleep del ESP32.
- Apagado dinámico del GPS.
- Apagado dinámico del módulo LoRa.
- Intervalos de transmisión adaptativos.

## Resultados estimados

| Escenario | Autonomía |
|------------|------------|
| Movimiento continuo | ~10.8 días |
| Reposo continuo | ~128 días |
| Operación mixta | ~27 días |

---


## Objetivo del proyecto

Desarrollar un nodo tracker LoRa/APRS basado en la plataforma T-Beam AXP2101 V1.2 que permita adquirir, procesar y transmitir información de posicionamiento geográfico, optimizando el consumo energético mediante técnicas de gestión inteligente de energía.

---

## Autores

- Marlon Cordero Sandoval
- Rolando Vega Marino

Instituto Tecnológico de Costa Rica

Curso: Taller Integrador

