El firmware del sistema se organiza utilizando una arquitectura en capas, lo que permite separar claramente las responsabilidades de cada módulo y facilitar el mantenimiento, escalabilidad y reutilización del código.

Esta estructura divide el sistema en niveles que van desde la lógica de aplicación hasta el hardware físico.

# Capa de aplicación

En la capa superior se encuentra la máquina de estados finitos (FSM), encargada de controlar el comportamiento global del sistema.

Sus funciones principales incluyen:

Control del flujo de operación

Gestión de estados (GPS, transmisión, recepción)

Manejo de errores

# Capa de procesamiento de datos

Esta capa se encarga de transformar la información obtenida de los sensores en datos útiles para transmisión.

Incluye:

Parser GPS: interpreta las tramas NMEA para obtener latitud, longitud y estado del fix

Encoder APRS: codifica los datos en el formato requerido para transmisión

Message Handler: procesa los datos recibidos desde LoRa

# Capa de servicios

Proporciona funcionalidades transversales necesarias para el sistema:

Gestión de energía: control del consumo mediante el AXP2101

Gestión de tiempo: temporizadores y eventos periódicos

Gestión de configuración: parámetros del sistema

Gestión de eventos: coordinación entre módulos

# Capa de drivers

Esta capa permite la comunicación directa con los dispositivos hardware:

GPS (UART): recepción de datos de posicionamiento

LoRa (SPI): transmisión y recepción de paquetes

PMU (I2C): monitoreo y control de energía

# Capa Perifericos

La capa de abstracción de hardware  proporciona acceso a los periféricos del ESP32, tales como:

UART

SPI

I2C

GPIO

Timers

RTC

# Hardware

Finalmente, el sistema físico está compuesto por:

ESP32 T-Beam

Módulo GPS

Transceptor LoRa

Sistema de gestión de energía (AXP2101)

Antenas
