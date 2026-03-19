## Diagrama de Flujo

El sistema se implementa mediante una máquina de estados finitos (FSM) que controla el flujo de operación de un nodo basado en ESP32 con capacidades de adquisición GPS y comunicación LoRa.

Esta arquitectura permite:

-Comportamiento determinístico

-Diseño modular

-Optimización del consumo energético

---
El sistema inicia en el estado INIT, donde se realiza la inicialización general del hardware y periféricos.

Posteriormente, pasa al estado LOAD_CONFIG, en el cual se cargan los parámetros necesarios para la operación, tales como:

Intervalos de transmisión
---
# Configuración del módulo LoRa

Parámetros del GPS

Luego, el sistema entra en CHECK_HW, donde se verifica el correcto funcionamiento de:

-Módulo GPS

-Transceptor LoRa

-Sistema de gestión de energía
---
# Estado de espera

Una vez validado el hardware, el sistema transiciona al estado IDLE, que corresponde a un estado de espera de bajo consumo.

En este estado:

-El sistema permanece inactivo

-Se reduce el consumo energético

-Se espera un evento de temporización (timer)

Cuando el temporizador se activa, se inicia el siguiente ciclo de operación.
---
# Adquisición de datos GPS

Al activarse el temporizador, el sistema entra en el estado GET_GPS, donde se realiza:

-Lectura de datos del GPS

-Validación de la información obtenida

-Se evalúa si los datos son válidos (por ejemplo, si existe un fix de posición).
---
# Procesamiento de datos

Dependiendo de la validez de los datos GPS:

-Si los datos son válidos, el sistema pasa a ENCODE_APRS, donde:

-Se procesa la información de posición

-Se codifica en formato APRS

Si los datos son inválidos, el sistema pasa a ERROR, donde:

-Se gestiona el fallo

-Se permiten reintentos o recuperación
---
# Transmisión LoRa

Una vez codificados los datos, el sistema entra en el estado TX_LORA, donde:

Se transmite el paquete de datos

Se utiliza el módulo LoRa para comunicación de largo alcance
---
# Recepción de datos

Después de la transmisión, el sistema pasa al estado RX_LORA, donde:

Se habilita la recepción de datos

Se permite comunicación bidireccional

Si se reciben datos, el sistema transiciona a PROCESS_RX, donde:

Se interpretan los mensajes recibidos

Se ejecutan acciones según el contenido
---
# Ahorro de energía

Finalmente, el sistema entra en el estado POWER_SAVE, donde se implementan estrategias de bajo consumo, tales como:

Reducción de frecuencia del microcontrolador

Modos de suspensión (sleep / deep sleep)

Después de este estado, el sistema regresa a IDLE, reiniciando el ciclo de operación.
