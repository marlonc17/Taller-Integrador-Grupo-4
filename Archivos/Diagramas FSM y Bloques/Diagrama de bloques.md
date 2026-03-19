-----------------------------------------------------------
|                    CAPA DE APLICACIÓN                    |
|----------------------------------------------------------|
|   FSM (Control del sistema)                              |
|   - Lógica de tracking                                   |
|   - Control de estados                                   |
|   - Manejo de errores                                    |
|             ## CAPA DE PROCESAMIENTO DE DATOS              |
|----------------------------------------------------------|
|   GPS Parser (NMEA)        |   APRS Encoder              |
|   - Latitud / Longitud     |   - Formato de paquete      |
|   - Validación de fix      |   - Codificación            |
|                                                          |
|           Message Handler (datos recibidos)              |

|                    CAPA DE SERVICIOS                     |
|----------------------------------------------------------|
|   Power Management (AXP2101)   |   Time Management       |
|   - Batería                    |   - Timers              |
|   - Control energético         |   - Wake-up             |
|                                                          |
|   Config Manager               |   Event Manager         |

|                      CAPA DE DRIVERS                     |
|----------------------------------------------------------|
|   LoRa Driver (SPI)                                      |
|   GPS Driver (UART)                                      |
|   PMU Driver (I2C)                                       |

|                CAPA HAL (ESP32 PERIFÉRICOS)              |
|----------------------------------------------------------|
|   UART | SPI | I2C | GPIO | RTC | Timers                 |

|                         HARDWARE                         |
|----------------------------------------------------------|
|   ESP32 T-Beam                                           |
|   GPS | LoRa | AXP2101 | Antenas                         |

