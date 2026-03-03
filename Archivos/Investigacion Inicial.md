#  Fundamentos de LoRa/APRS

##  ¿Qué es LoRa?

LoRa (Long Range) es una tecnología de comunicación inalámbrica diseñada para:

- Comunicación de larga distancia  
- Bajo consumo energético  
- Aplicaciones IoT (Internet of Things)

Es ideal para dispositivos que necesitan transmitir pequeñas cantidades de datos a grandes distancias, como:

- Seguimiento de vehículos  
- Agricultura inteligente  
- Logística  
- Sensores remotos  

---

##  ¿Qué es APRS?

APRS (Automatic Packet Reporting System) es un sistema de comunicación digital utilizado principalmente por radioaficionados para el intercambio de información en tiempo real.

###  Información que puede transmitir:

- Ubicación GPS  
- Datos meteorológicos  
- Mensajes cortos  
- Alertas de emergencia  
- Telemetría  

APRS utiliza:

- Modulación AFSK (Audio Frequency-Shift Keying) en FM  
- Velocidad de 1,200 baudios  
- Tonos de 1,200 Hz y 2,200 Hz  
- Protocolo AX.25  

---

##  Funcionamiento de la Red APRS

Una red APRS tradicional puede estar compuesta por:

- Estaciones transmisoras  
- Digipeaters (repetidores digitales)  
- iGate (puerta de enlace a Internet)  

Cuando se conecta a Internet, se integra a APRS-IS (Internet System), permitiendo visualización global.

En Internet existen múltiples plataformas donde se pueden visualizar mapas con estaciones APRS activas alrededor del mundo.

---

##  Frecuencias de Operación

###  APRS Tradicional (Packet Radio)

- Frecuencia en Europa: 144.800 MHz (VHF)  
- Protocolo: AX.25  

###  LoRa APRS

- Frecuencia adoptada en Europa: 433.775 MHz (UHF)  
- Utiliza el protocolo LoRa en lugar de AX.25 tradicional  

---

##  ¿Qué es LoRa APRS?

LoRa APRS combina ambas tecnologías:

- La eficiencia energética y largo alcance de LoRa  
- El sistema de posicionamiento y mensajería de APRS  

Esto permite crear una red de comunicación:

- De bajo consumo  
- De largo alcance  
- Robusta y eficiente  

Ideal para experimentación en radioafición y proyectos técnicos.

---

##  Uso Exclusivo para Radioaficionados

Tanto APRS como LoRa APRS:

- Requieren licencia de radioaficionado  
- Exigen identificación mediante indicativo  
- Utilizan frecuencias asignadas al servicio de radioaficionados  

---

## Arquitectura de red:

- Nodo (Cliente/Estación): Es el dispositivo de usuario final. Puede ser un rastreador GPS en un vehículo, una estación meteorológica o un dispositivo móvil. Su función es transmitir datos (posición, telemetría, mensajes) y recibir datos de otros nodos. En LoRa/APRS, un nodo típico podría ser un ESP32 con un módulo LoRa y un GPS .

- Digipeater (Repetidor Digital): Es una estación que recibe paquetes de datos por radiofrecuencia (RF) y los retransmite en la misma frecuencia para extender el alcance de la red. Su objetivo es que los paquetes lleguen más lejos, de "salto en salto". Opera según reglas para evitar bucles infinitos (como el algoritmo "New-N") .

- iGate (Puerta de Enlace a Internet): Es una estación que cumple una doble función: recibe paquetes por RF y los reenvía a Internet (APRS-IS), y viceversa. Actúa como un puente entre la red de radio local y la red global de datos, permitiendo que las estaciones locales sean visibles en mapas mundiales como aprs.fi .

- Gateway LoRa: En el contexto de una red LoRaWAN (más amplia que solo APRS), el gateway es un dispositivo que escucha en múltiples canales y frecuencias para recibir mensajes de todos los nodos LoRa a su alcance. Luego, empaqueta estos mensajes y los reenvía a un servidor de red a través de una conexión IP (Ethernet, WiFi, Celular). A diferencia de un iGate de APRS, un gateway LoRaWAN es "tonto" (solo pasa datos) y la inteligencia de la red reside en el servidor.

- Servidor: Es el "cerebro" de la red. En el ecosistema APRS-IS, son servidores centrales que reciben, filtran y redistribuyen los paquetes de todo el mundo. En una red LoRaWAN privada, el servidor se encarga de la autenticación, el control de acceso, el almacenamiento de datos y la gestión de la red. En el contexto de APRS, también puede referirse al software que corre en un iGate para manejar los datos .

- Cliente Final: Es la aplicación o interfaz que utiliza un usuario humano para visualizar o interactuar con la red. Puede ser un programa como APRSISCE/32, Xastir en una computadora, o una app móvil como APRSDroid, que muestra los datos en un mapa

## Capa Física y de Enlace (Modelo OSI) de LoRa

- Spreading Factor (SF) - Factor de Dispersión:
En LoRa, el SF indica cuántos bits se transmiten por símbolo utilizando modulación Chirp Spread Spectrum (CSS).
El Spreading Factor es el número de bits codificados por símbolo y determina cuántos chips se usan para representar cada símbolo, la duracion del simbolo y la velocidad de transmision de los datos.  
Rango: Generalmente de SF7 a SF12.  
Efecto: Un SF más alto significa que cada bit de datos se codifica en más "chips", lo que hace la señal más resistente al ruido y permite decodificarla a niveles de señal más bajos (mayor sensibilidad y alcance). La desventaja es que el "Time on Air" (tiempo de transmisión) es mucho mayor, lo que consume más batería y reduce el tiempo disponible para que otros dispositivos transmitan. Un SF bajo es más rápido pero menos sensible .

- Bandwidth (BW) - Ancho de Banda:
El rango de frecuencias que utiliza la señal LoRa. Los valores típicos son 125 kHz, 250 kHz y 500 kHz.  
Un ancho de banda mayor permite una velocidad de datos más alta, pero reduce la sensibilidad del receptor (menor alcance).  
Un ancho de banda menor concentra la energía en un espacio más reducido, mejorando la sensibilidad y el alcance .

- Coding Rate (CR) - Tasa de Codificación:
Es la tasa de corrección de errores hacia adelante (FEC). Se expresa como una relación (como 4/5, 4/6, 4/7, 4/8).
Un CR de 4/8 significa que por cada 4 bits de datos de informacion, se transmiten 8 bits (más redundancia), ofreciendo la máxima protección contra interferencias y errores. Un CR de 4/5 tiene menos redundancia, es más eficiente pero menos robusto.  

- Sync Word - Palabra de Sincronización:
Un valor configurable que actúa como una dirección de red a nivel de radio. Los receptores LoRa solo escucharán las transmisiones que tengan la Sync Word correcta. Se utiliza para separar redes. Por ejemplo, las redes públicas de LoRaWAN suelen usar un valor (0x34), mientras que las redes privadas o experimentales pueden usar otro para no interferir.  

- ADR (Adaptive Data Rate):
Un mecanismo de control que optimiza la velocidad de datos, la potencia de transmisión y otros parámetros de un nodo. El servidor de red (o un iGate en sistemas más simples) analiza la calidad de las transmisiones recientes de un nodo (relación señal/ruido) y le ordena que ajuste sus parámetros (usar un SF más bajo y menos potencia si tiene buena cobertura) para maximizar la eficiencia energética y la capacidad total de la red.  

# Legislación Costarricense Aplicable a Sistemas LoRa / APRS

---

# Marco Normativo

La regulación del espectro radioeléctrico en Costa Rica se rige por:

- **Plan Nacional de Atribución de Frecuencias (PNAF)**  
  Decreto Ejecutivo Nº 44010-MICITT  
  Publicado en el Alcance N°99 a La Gaceta N°95 del 30 de mayo de 2023.

- Reglamento General para la Regulación de los Trámites del Servicio de Radioaficionados  
  Decreto Ejecutivo Nº 40639-MICITT.

- Ley General de Telecomunicaciones Nº 8642.

El PNAF define:

- Atribución de bandas de frecuencia.
- Servicios autorizados en cada banda.
- Condiciones técnicas.
- Límites de potencia radiada (PIRE/EIRP).
- Clasificación de uso libre o concesionado.

---

# Bandas de Frecuencia Aplicables en Costa Rica

## Para Sistemas LoRa (Bandas de Uso Libre – Corto Alcance)

El PNAF contempla bandas para sistemas de baja potencia y corto alcance, utilizadas comúnmente para LoRa:

| Banda (MHz)         | Tipo de Uso        |
|---------------------|-------------------|
| 433.05 – 434.79     | Uso libre (SRD)   |
| 902 – 915           | Uso libre         |
| 915 – 921           | Uso libre         |
| 921 – 960           | Uso libre         |
| 2400 – 2500         | Uso libre (ISM)   |

Estas bandas pueden utilizarse sin concesión individual siempre que se respeten los límites técnicos establecidos.

---

## Para APRS (Servicio de Radioaficionados)

APRS tradicional opera en bandas atribuidas al Servicio de Radioaficionados:

- Banda de 2 metros: **144 – 148 MHz**
- Frecuencia práctica utilizada en América: **144.390 MHz**

En este caso, la operación requiere:

- Licencia de radioaficionado vigente.
- Permiso de uso del espectro (título habilitante).
- Identificación mediante indicativo oficial.

---

# PIRE Permitida por Banda

Según el PNAF vigente, los límites máximos de Potencia Isotrópica Radiada Equivalente (PIRE/EIRP) son:

| Banda (MHz)         | PIRE Máxima | Equivalente en Watts |
|---------------------|------------|----------------------|
| 433.05 – 434.79     | 30 dBm     | 1 W |
| 902 – 915           | 30 dBm     | 1 W |
| 915 – 921           | 36 dBm     | ≈ 4 W |
| 921 – 960           | 30 dBm     | 1 W |
| 2400 – 2500         | 36 dBm     | ≈ 4 W |

Conversión:

\[
P(W) = 10^{\frac{dBm - 30}{10}}
\]

Ejemplo:

\[
30 \, dBm = 10^{\frac{30-30}{10}} = 1 \, W
\]

El cumplimiento de estos límites es obligatorio incluso en bandas de uso libre.

---

# Permisos Requeridos

La exigencia depende de la banda utilizada.

---

## Caso A: APRS en Banda de Radioaficionado

Requisitos:

1. Aprobar examen técnico ante SUTEL.
2. Obtener licencia de radioaficionado.
3. Solicitar permiso de uso del espectro.
4. Operar bajo indicativo asignado.

---

## Caso B: LoRa en Banda de Uso Libre

No requiere concesión individual si:

- Se respeta la banda autorizada.
- No se excede la PIRE máxima.
- Se cumple con condiciones técnicas del PNAF.
- El equipo está homologado según normativa nacional.

---

# Mapa de Tiempo de Trámites (Servicio de Radioaficionados)

Según Decreto 40639-MICITT:

### Proceso de Licencia

| Etapa | Tiempo Máximo |
|-------|--------------|
| Traslado de solicitud a SUTEL | 10–20 días hábiles |
| Resolución de SUTEL | Hasta 20 días hábiles |
| Proceso completo de licencia | Hasta 60 días hábiles |

---

### Proceso de Permiso de Uso del Espectro

| Etapa | Tiempo Máximo |
|-------|--------------|
| Solicitud de criterio técnico | 5 días hábiles |
| Recomendación técnica SUTEL | Hasta 45 días hábiles |
| Resolución final | Hasta 60 días hábiles |

Flujo simplificado:

Solicitud → Evaluación técnica → Resolución SUTEL → Otorgamiento de permiso → Operación

## Clases de Emisión (Modulación)

Las clases de emisión se definen según el Reglamento de Radiocomunicaciones de la UIT (Apéndice 1).

Describen:

1. Ancho de banda necesario.
2. Tipo de modulación.
3. Naturaleza de la señal moduladora.
4. Tipo de información transmitida.

Formato general:

```
[AnchoBanda][TipoModulación][TipoSeñal][TipoInformación]
```

---

### APRS Tradicional (AFSK sobre FM)

Ejemplo representativo:

```
16K0F2D
```

Donde:

- 16K0 → 16 kHz de ancho de banda
- F → Modulación en frecuencia
- 2 → Una señal digital
- D → Transmisión de datos

---

### LoRa (Chirp Spread Spectrum)

LoRa utiliza modulación digital de espectro ensanchado.

Ejemplo conceptual:

```
125K0G1D
```

- 125K0 → 125 kHz de ancho de banda
- G → Espectro ensanchado
- 1 → Señal digital sin subportadora
- D → Datos

La designación exacta depende del ancho de banda configurado y de los parámetros reales del sistema.
