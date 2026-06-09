# Tracker LoRa/APRS

Este repositorio presenta el desarrollo de un **tracker GPS basado en un T-Beam V1.2**, orientado a pruebas de **posicionamiento, telemetría y transmisión inalámbrica de datos**. La idea del proyecto es combinar el uso de **GPS** para obtener ubicación con **LoRa** como medio de comunicación de largo alcance y bajo consumo, tomando como referencia la lógica de reporte automático utilizada en sistemas **APRS**. Y se planea utilizarlo en la implementacion de un sistema de monitoreo de camiones de gas con el objetivo de tener un mayor control de donde se encuentran los camiones y de tener las estaciones listas y preparadas antes de que estos lleguen para optimizar la carga o descarga de estos

## ¿Qué es APRS?

**APRS (Automatic Packet Reporting System)** es un sistema de comunicación digital utilizado para compartir información en tiempo real dentro de una red. Entre los datos que puede transmitir se encuentran la **ubicación GPS**, mensajes cortos, telemetría, alertas y otros datos de interés operativo. En su forma tradicional, APRS se asocia al uso de **AX.25**, modulación **AFSK** y estaciones como **digipeaters** e **iGate**, que permiten repetir paquetes o conectarlos a Internet mediante **APRS-IS**.

## ¿Qué es LoRa?

**LoRa (Long Range)** es una tecnología de comunicación inalámbrica diseñada para transmitir pequeñas cantidades de datos a largas distancias con **bajo consumo energético**. Utiliza una modulación de espectro ensanchado tipo **Chirp Spread Spectrum (CSS)**, lo que le da buena robustez frente al ruido y la hace especialmente útil en aplicaciones de **IoT**, monitoreo remoto, logística y telemetría.

## ¿Qué es un tracker?

Un **tracker** es un dispositivo que obtiene la ubicación de un objeto, vehículo o persona mediante **GPS** y la transmite por un medio de comunicación inalámbrico. En este proyecto, el tracker será una estación compacta que:

- obtiene coordenadas desde el GPS,
- procesa la información con el microcontrolador,
- construye el paquete de datos,
- y lo transmite usando LoRa.

En otras palabras, el tracker es el nodo que reporta automáticamente su posición y otra telemetría relevante.

## Relación entre APRS, LoRa y este proyecto

La idea del proyecto es aprovechar la lógica de reporte automático de **APRS** y combinarla con las ventajas de **LoRa**. Es decir, mantener el concepto de un nodo que envía su posición y datos útiles, pero usando una plataforma moderna de bajo consumo basada en **ESP32 + GPS + LoRa**. Para ello, el hardware seleccionado es un **T-Beam V1.2**, ya que integra en una sola tarjeta el microcontrolador, el módulo LoRa y el sistema necesario para desarrollar un tracker de este tipo.

## Legislación y uso de frecuencia

Como se trata de un sistema inalámbrico, su operación debe ajustarse a la normativa vigente. En Costa Rica, el uso del espectro radioeléctrico está regulado por el **Plan Nacional de Atribución de Frecuencias (PNAF)**. Según la investigación realizada, las bandas usadas para sistemas tipo LoRa pueden operar bajo condiciones técnicas específicas de uso libre, mientras que el uso de **APRS en bandas de radioaficionado** requiere licencia, permiso de uso del espectro e identificación mediante indicativo oficial. Por ello, cualquier implementación final debe revisarse de acuerdo con la regulación aplicable antes de ponerse en operación.
