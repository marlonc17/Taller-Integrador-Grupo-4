# Tracker TI0TEC4-7 — T-Beam AXP2101 V1.2

Firmware propio del nodo tracker LoRa/APRS desarrollado para el Taller Integrador.  
Escuela de Ingeniería en Electrónica — Marlon Cordero Sandoval / Rolando Vega Marino

## Hardware

- Placa: LilyGo T-Beam AXP2101 V1.2
- Radio: SX1276 (LoRa 433 MHz)
- GPS: NEO-6M/M8N
- PMIC: AXP2101

## Pines utilizados

| Función | GPIO |
|---|---|
| GPS RX (ESP32 recibe) | GPIO34 |
| GPS TX (ESP32 envía) | GPIO12 |
| LoRa SCK | GPIO5 |
| LoRa MOSI | GPIO27 |
| LoRa MISO | GPIO19 |
| LoRa CS | GPIO18 |
| LoRa RST | GPIO23 |
| LoRa DIO0 (IRQ TX) | GPIO26 |
| AXP2101 SDA | GPIO21 |
| AXP2101 SCL | GPIO22 |

## Parámetros LoRa

| Parámetro | Valor |
|---|---|
| Frecuencia | 433.775 MHz |
| Spreading Factor | SF12 |
| Bandwidth | 125 kHz |
| Coding Rate | 4/5 |
| Potencia TX | 20 dBm |
| Sync Word | 0x12 |

## Formato de trama 

Se utiliza el formato estandar de APRS

Ejemplo:
```
TI0TEC4-7>APLT00,WIDE1-1:!0956.08N/08405.25W[270/000/A=000120 LoRa Tracker !w../
```

## Cómo cargar el firmware

1. Instalar VS Code
2. Instalar extensión PlatformIO IDE
3. Instalar driver CP210x (chip USB del T-Beam)
4. Clonar este repositorio
5. Abrir la carpeta en VS Code
6. Conectar el T-Beam por USB
7. Click en el botón Upload de PlatformIO (flecha →)

## Cómo ver el monitor serial

1. Click en el botón Monitor de PlatformIO (enchufe)
2. Velocidad: 115200 baud
3. Se verán los mensajes de cada estado de la FSM

## Arquitectura del firmware

El firmware implementa una máquina de estados finitos (FSM) con dos fases:

**Fase de arranque (setup):** INIT → LOAD_CONFIG → CHECK_HW → SLEEP

**Ciclo periódico (loop):** SLEEP → GPS_ON → SENSING → BUILD_PACKET → TX_DATA → WAIT_TX → SLEEP

**Ruta de error:** cualquier estado puede ir a ERROR_RETRY → reintento o SLEEP forzado
