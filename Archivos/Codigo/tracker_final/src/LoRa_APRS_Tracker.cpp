#include <APRS-Decoder.h>
#include <Arduino.h>
#include <LoRa.h>
#include <OneButton.h>
#include <TimeLib.h>
#include <TinyGPS++.h>
#include <WiFi.h>
#include <logger.h>

#include "BeaconManager.h"
#include "configuration.h"
#include "display.h"
#include "pins.h"
#include "power_management.h"

#define VERSION "23.36.01"

logging::Logger logger;

Configuration Config;
BeaconManager BeaconMan;

#ifdef TTGO_T_Beam_V1_0
AXP192           axp;
PowerManagement *powerManagement = &axp;
#endif
#ifdef TTGO_T_Beam_V1_2
AXP2101          axp;
PowerManagement *powerManagement = &axp;
#endif
OneButton userButton = OneButton(BUTTON_PIN, true, true);

HardwareSerial ss(1);
TinyGPSPlus    gps;

void setup_gps();
void load_config();
void setup_lora();

String create_lat_aprs(RawDegrees lat);
String create_long_aprs(RawDegrees lng);
String create_lat_aprs_dao(RawDegrees lat);
String create_long_aprs_dao(RawDegrees lng);
String create_dao_aprs(RawDegrees lat, RawDegrees lng);
String createDateString(time_t t);
String createTimeString(time_t t);
String getSmartBeaconState();
String padding(unsigned int number, unsigned int width);

static bool send_update          = true;
static bool display_toggle_value = true;

static void handle_tx_click() {
  send_update = true;
}

static void handle_next_beacon() {
  BeaconMan.loadNextBeacon();
  show_display(BeaconMan.getCurrentBeaconConfig()->callsign, BeaconMan.getCurrentBeaconConfig()->message, 2000);
}

static void toggle_display() {
  display_toggle_value = !display_toggle_value;
  display_toggle(display_toggle_value);
  if (display_toggle_value) {
    setup_display();
  }
}

// cppcheck-suppress unusedFunction
void setup() {
  Serial.begin(115200);

#if defined(TTGO_T_Beam_V1_0) || defined(TTGO_T_Beam_V1_2)
  Wire.begin(SDA, SCL);
  if (powerManagement->begin(Wire)) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "PMU", "init done!");
  } else {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "PMU", "init failed!");
  }
  powerManagement->activateLoRa();
  powerManagement->activateOLED();
  powerManagement->activateMeasurement();
#endif

  delay(500);
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "LoRa APRS Tracker by OE5BPA (Peter Buchegger)");
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "Version: " VERSION);
  setup_display();

  show_display("OE5BPA", "LoRa APRS Tracker", "by Peter Buchegger", "Version: " VERSION, 2000);
  load_config();

  setup_gps();
  setup_lora();

  if (Config.ptt.active) {
    pinMode(Config.ptt.io_pin, OUTPUT);
    digitalWrite(Config.ptt.io_pin, Config.ptt.reverse ? HIGH : LOW);
  }

  // make sure wifi and bt is off as we don't need it:
  WiFi.mode(WIFI_OFF);
  btStop();

  if (Config.button.tx) {
    // attach TX action to user button (defined by BUTTON_PIN)
    userButton.attachClick(handle_tx_click);
  }

  if (Config.button.alt_message) {
    userButton.attachLongPressStart(handle_next_beacon);
  }
  userButton.attachDoubleClick(toggle_display);

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "Smart Beacon is: %s", getSmartBeaconState());
  show_display("INFO", "Smart Beacon is " + getSmartBeaconState(), 1000);

  // Encender GPS al final del setup para que empiece a calentar
  // antes de que GPS_CALIBRATION lo necesite
  powerManagement->activateGPS();

  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "Main", "setup done...");
  delay(500);
}

// =====================================================
// FSM — Estados
// =====================================================
enum Estado {
  GPS_CALIBRATION,   // validacion inicial — se ejecuta una sola vez al arrancar
  GPS_ON,
  SENSING,
  BUILD_PACKET,
  TX_DATA,
  SLEEP,
  ERROR_RETRY
};

// Variables de la FSM
static Estado   estadoActual       = GPS_CALIBRATION;  // arranca en calibracion
static Estado   estadoFallo        = GPS_ON;
static int      contadorReintentos = 0;
static bool     enMovimiento       = false;
static uint32_t timerProximoCiclo  = 0;
static uint32_t timerInicio        = 0;

// Variables de calibracion inicial
static int      calLecturas        = 0;
static uint32_t calTimerEspera     = 0;
static bool     calEsperandoFix    = true;
static bool     calEsperandoPausa  = false;
static uint32_t calTiempoTotal     = 0;

// Intervalos de pausa entre lecturas de calibracion (ms)
// lectura 1 -> 3 min -> lectura 2 -> 2 min -> lectura 3 -> 1 min -> lectura 4
static const uint32_t calIntervalos[] = { 180000, 120000, 60000 };
#define CAL_LECTURAS_TOTAL  4
#define CAL_TIMEOUT_FIX     120000    // 2 min max esperando cada fix individual
#define CAL_TIMEOUT_TOTAL   900000    // 15 min max para toda la calibracion

// Parametros operativos (desde tracker.json via BeaconManager)
// UMBRAL_VELOCIDAD: 5 km/h
// INTERVALO_MOVIMIENTO: slow_rate del smart beacon (300s por defecto)
// INTERVALO_REPOSO: slow_rate cuando esta detenido (3600s)
// TIMEOUT_GPS: 60000 ms
// TIMEOUT_TX: 5000 ms
// MAX_REINTENTOS: 3
#define UMBRAL_VELOCIDAD      5
#define TIMEOUT_GPS           60000
#define TIMEOUT_TX            5000
#define MAX_REINTENTOS        3
#define INTERVALO_MOVIMIENTO  300000   // 5 min en ms
#define INTERVALO_REPOSO      3600000  // 1 hora en ms

// cppcheck-suppress unusedFunction
void loop() {

  // Leer GPS continuamente en todos los estados
  while (ss.available() > 0) {
    gps.encode(ss.read());
  }

  userButton.tick();

  switch (estadoActual) {

    // ── GPS_CALIBRATION ──
    // Validacion inicial del GPS — se ejecuta una sola vez al arrancar.
    // Obtiene 4 lecturas validas con pausas decrecientes entre ellas:
    // fix1 -> 3 min -> fix2 -> 2 min -> fix3 -> 1 min -> fix4 -> ciclo normal
    // Si supera CAL_TIMEOUT_TOTAL continua de todas formas con advertencia.
    case GPS_CALIBRATION: {
      static bool calIniciado = false;

      if (!calIniciado) {
        calIniciado     = true;
        calTiempoTotal  = millis();
        calTimerEspera  = millis();
        calLecturas     = 0;
        calEsperandoFix = true;
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "CAL", "Iniciando validacion GPS — 4 lecturas requeridas");
        show_display("CAL GPS", "Validando GPS...", "Lect: 0/" + String(CAL_LECTURAS_TOTAL), "");
      }

      // Verificar timeout total
      if (millis() - calTiempoTotal > CAL_TIMEOUT_TOTAL) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "CAL", "Timeout total — continuando sin calibracion completa (%d/%d lecturas)", calLecturas, CAL_LECTURAS_TOTAL);
        show_display("CAL GPS", "Timeout", String(calLecturas) + "/" + String(CAL_LECTURAS_TOTAL) + " lect.", 2000);
        calIniciado       = false;
        estadoActual      = SLEEP;
        timerProximoCiclo = INTERVALO_REPOSO;
        return;
      }

      // Esperando fix individual
      if (calEsperandoFix) {
        show_display("CAL GPS",
                     "Lect " + String(calLecturas + 1) + "/" + String(CAL_LECTURAS_TOTAL),
                     "Esperando fix...",
                     "");

        if (gps.location.isValid() && gps.location.isUpdated()) {
          calLecturas++;
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "CAL",
                     "Lectura %d/%d OK — Lat=%.6f Lon=%.6f",
                     calLecturas, CAL_LECTURAS_TOTAL,
                     gps.location.lat(), gps.location.lng());

          show_display("CAL GPS",
                       "Lect " + String(calLecturas) + "/" + String(CAL_LECTURAS_TOTAL) + " OK",
                       String(gps.location.lat(), 5),
                       String(gps.location.lng(), 5),
                       2000);

          // Si ya tenemos todas las lecturas -> ciclo normal
          if (calLecturas >= CAL_LECTURAS_TOTAL) {
            logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "CAL", "Calibracion completada — iniciando ciclo normal");
            show_display("CAL GPS", "Completa!", String(CAL_LECTURAS_TOTAL) + " lect. OK", 2000);
            calIniciado       = false;
            estadoActual      = SLEEP;
            timerProximoCiclo = INTERVALO_REPOSO;
            return;
          }

          // Hay mas lecturas — iniciar pausa
          calEsperandoFix   = false;
          calEsperandoPausa = true;
          calTimerEspera    = millis();

          uint32_t pausaSeg = calIntervalos[calLecturas - 1] / 1000;
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "CAL", "Pausa de %d seg antes de siguiente lectura", pausaSeg);
        }

        // Timeout del fix individual — reiniciar timer
        if (millis() - calTimerEspera > CAL_TIMEOUT_FIX) {
          calTimerEspera = millis();
        }
        return;
      }

      // En pausa entre lecturas — mostrar cuenta regresiva
      if (calEsperandoPausa) {
        uint32_t intervalo    = calIntervalos[calLecturas - 1];
        uint32_t transcurrido = millis() - calTimerEspera;
        uint32_t restante     = (transcurrido < intervalo) ? (intervalo - transcurrido) / 1000 : 0;

        show_display("CAL GPS",
                     "Pausa " + String(restante) + "s",
                     "Sig lect: " + String(calLecturas + 1),
                     String(calLecturas) + "/" + String(CAL_LECTURAS_TOTAL) + " OK");

        if (millis() - calTimerEspera >= intervalo) {
          calEsperandoPausa = false;
          calEsperandoFix   = true;
          calTimerEspera    = millis();
          logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "CAL", "Pausa terminada — esperando lectura %d", calLecturas + 1);
        }
      }
      break;
    }

    // ── GPS_ON ──
    // Activa el GPS via AXP2101 y espera que responda
    case GPS_ON: {
      static bool gpsActivado = false;
      if (!gpsActivado) {
        powerManagement->activateGPS();
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "FSM", "GPS_ON: GPS energizado");
        show_display("GPS_ON", "Esperando fix...", "");
        timerInicio  = millis();
        gpsActivado  = true;
      }

      if (gps.location.isValid() && gps.location.isUpdated()) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "FSM", "GPS_ON: GPS responde -> SENSING");
        gpsActivado  = false;
        estadoActual = SENSING;
        return;
      }

      if (millis() - timerInicio > TIMEOUT_GPS) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "FSM", "GPS_ON: timeout -> ERROR_RETRY");
        gpsActivado  = false;
        estadoFallo  = GPS_ON;
        estadoActual = ERROR_RETRY;
      }
      break;
    }

    // ── SENSING ──
    // Valida datos GPS y extrae posicion
    case SENSING: {
      if (!gps.location.isValid()) {
        estadoFallo  = SENSING;
        estadoActual = ERROR_RETRY;
        return;
      }

      double velocidad = gps.speed.kmph();
      double rumbo     = gps.course.deg();
      double lat       = gps.location.lat();
      double lng       = gps.location.lng();

      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "FSM",
                 "SENSING: Lat=%.6f Lon=%.6f Vel=%.1f Rum=%.1f",
                 lat, lng, velocidad, rumbo);

      show_display("SENSING",
                   "Lat: " + String(lat, 6),
                   "Lon: " + String(lng, 6),
                   "Vel: " + String(velocidad, 1) + " km/h");

      estadoActual = BUILD_PACKET;
      break;
    }

    // ── BUILD_PACKET ──
    // Construye la trama APRS estandar usando las funciones del repositorio.
    // Formato: CALLSIGN>APLT00,WIDE1-1:!LAT/LON[curso/vel/mensaje
    case BUILD_PACKET: {
      double velocidad = gps.speed.kmph();
      double rumbo     = gps.course.deg();

      // Construir mensaje APRS usando APRSMessage
      APRSMessage msg;
      msg.setSource(BeaconMan.getCurrentBeaconConfig()->callsign);
      msg.setPath(BeaconMan.getCurrentBeaconConfig()->path);
      msg.setDestination("APLT00");

      // Coordenadas en formato APRS con precision extendida (DAO)
      String lat = create_lat_aprs_dao(gps.location.rawLat());
      String lng = create_long_aprs_dao(gps.location.rawLng());
      String dao = create_dao_aprs(gps.location.rawLat(), gps.location.rawLng());

      // Altitud
      String alt     = "";
      int    alt_int = max(-99999, min(999999, (int)gps.altitude.feet()));
      if (alt_int < 0) {
        alt = "/A=-" + padding(alt_int * -1, 5);
      } else {
        alt = "/A=" + padding(alt_int, 6);
      }

      // Curso y velocidad en nudos
      String curso_vel = "";
      int    speed_int = max(0, min(999, (int)gps.speed.knots()));
      int    course_int = max(0, min(360, (int)rumbo));
      if (course_int == 0) course_int = 360;
      curso_vel = padding(course_int, 3) + "/" + padding(speed_int, 3);

      // Armar cuerpo del mensaje APRS
      String cuerpo = "!" + lat +
                      BeaconMan.getCurrentBeaconConfig()->overlay +
                      lng +
                      BeaconMan.getCurrentBeaconConfig()->symbol +
                      curso_vel + alt +
                      BeaconMan.getCurrentBeaconConfig()->message +
                      " " + dao;

      msg.getBody()->setData(cuerpo);
      String trama = msg.encode();

      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "FSM", "BUILD_PACKET: %s", trama.c_str());
      show_display("BUILD", trama.substring(0, 21), trama.substring(21, 42), "");

      // Guardar trama para TX_DATA
      static String tramaAPRS = "";
      tramaAPRS = trama;

      // Determinar proximo intervalo segun movimiento
      if (velocidad > UMBRAL_VELOCIDAD) {
        timerProximoCiclo = INTERVALO_MOVIMIENTO;
        enMovimiento      = true;
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "FSM", "En movimiento -> 5 min");
      } else {
        timerProximoCiclo = INTERVALO_REPOSO;
        enMovimiento      = false;
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "FSM", "Detenido -> 1 hora");
      }

      estadoActual = TX_DATA;
      break;
    }

    // ── TX_DATA ──
    // Apaga GPS y transmite la trama APRS por LoRa
    case TX_DATA: {
      powerManagement->deactivateGPS();
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "FSM", "TX_DATA: GPS apagado, transmitiendo...");

      // Reconstruir trama APRS
      APRSMessage msg;
      msg.setSource(BeaconMan.getCurrentBeaconConfig()->callsign);
      msg.setPath(BeaconMan.getCurrentBeaconConfig()->path);
      msg.setDestination("APLT00");

      String lat      = create_lat_aprs_dao(gps.location.rawLat());
      String lng      = create_long_aprs_dao(gps.location.rawLng());
      String dao      = create_dao_aprs(gps.location.rawLat(), gps.location.rawLng());
      String alt      = "/A=" + padding(max(0, min(999999, (int)gps.altitude.feet())), 6);
      int    speed_int  = max(0, min(999, (int)gps.speed.knots()));
      int    course_int = max(0, min(360, (int)gps.course.deg()));
      if (course_int == 0) course_int = 360;
      String curso_vel  = padding(course_int, 3) + "/" + padding(speed_int, 3);

      String cuerpo = "!" + lat +
                      BeaconMan.getCurrentBeaconConfig()->overlay +
                      lng +
                      BeaconMan.getCurrentBeaconConfig()->symbol +
                      curso_vel + alt +
                      BeaconMan.getCurrentBeaconConfig()->message +
                      " " + dao;
      msg.getBody()->setData(cuerpo);
      String trama = msg.encode();

      show_display("<< TX >>", trama.substring(0, 21), trama.substring(21, 42), "");

      LoRa.beginPacket();
      LoRa.write('<');
      LoRa.write(0xFF);
      LoRa.write(0x01);
      LoRa.write((const uint8_t *)trama.c_str(), trama.length());
      int resultado = LoRa.endPacket();

      if (resultado) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "FSM", "TX_DATA: TX OK -> SLEEP");
        show_display("TX OK", trama.substring(0, 21), trama.substring(21, 42), "");
        estadoActual = SLEEP;
      } else {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "FSM", "TX_DATA: TX fallo -> ERROR_RETRY");
        estadoFallo  = TX_DATA;
        estadoActual = ERROR_RETRY;
      }
      break;
    }

    // ── SLEEP ──
    // Espera el intervalo definido antes del proximo ciclo
    case SLEEP: {
      static bool sleepIniciado = false;
      if (!sleepIniciado) {
        timerInicio   = millis();
        sleepIniciado = true;
        contadorReintentos = 0;
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "FSM",
                   "SLEEP: esperando %d segundos", timerProximoCiclo / 1000);
        show_display("SLEEP",
                     "Proximo ciclo:",
                     String(timerProximoCiclo / 1000) + " seg",
                     enMovimiento ? "Estado: movimiento" : "Estado: detenido");
      }

      if (millis() - timerInicio >= timerProximoCiclo) {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "FSM", "SLEEP: timer expirado -> GPS_ON");
        sleepIniciado = false;
        estadoActual  = GPS_ON;
      }
      break;
    }

    // ── ERROR_RETRY ──
    // Reintenta hasta MAX_REINTENTOS, luego fuerza SLEEP
    case ERROR_RETRY: {
      contadorReintentos++;
      logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "FSM",
                 "ERROR_RETRY: intento %d de %d", contadorReintentos, MAX_REINTENTOS);
      show_display("ERROR_RETRY",
                   "Intento " + String(contadorReintentos) + "/" + String(MAX_REINTENTOS),
                   "");

      if (contadorReintentos < MAX_REINTENTOS) {
        estadoActual = estadoFallo;
      } else {
        logger.log(logging::LoggerLevel::LOGGER_LEVEL_WARN, "FSM", "Reintentos agotados -> SLEEP");
        timerProximoCiclo  = INTERVALO_REPOSO;
        contadorReintentos = 0;
        estadoActual       = SLEEP;
      }
      break;
    }
  }
}

void load_config() {
  ConfigurationManagement confmg("/tracker.json");
  Config = confmg.readConfiguration();
  BeaconMan.loadConfig(Config.beacons);
  if (BeaconMan.getCurrentBeaconConfig()->callsign == "NOCALL-7") {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR,
               "Config",
               "You have to change your settings in 'data/tracker.json' and "
               "upload it via \"Upload File System image\"!");
    show_display("ERROR",
                 "You have to change your settings in 'data/tracker.json' and "
                 "upload it via \"Upload File System image\"!");
    while (true) {
    }
  }
}

void setup_lora() {
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "Set SPI pins!");
  SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "Set LoRa pins!");
  LoRa.setPins(LORA_CS, LORA_RST, LORA_IRQ);

  long freq = Config.lora.frequencyTx;
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "frequency: %d", freq);
  if (!LoRa.begin(freq)) {
    logger.log(logging::LoggerLevel::LOGGER_LEVEL_ERROR, "LoRa", "Starting LoRa failed!");
    show_display("ERROR", "Starting LoRa failed!");
    while (true) {
    }
  }
  LoRa.setSpreadingFactor(Config.lora.spreadingFactor);
  LoRa.setSignalBandwidth(Config.lora.signalBandwidth);
  LoRa.setCodingRate4(Config.lora.codingRate4);
  LoRa.enableCrc();

  LoRa.setTxPower(Config.lora.power);
  logger.log(logging::LoggerLevel::LOGGER_LEVEL_INFO, "LoRa", "LoRa init done!");
  show_display("INFO", "LoRa init done!", 2000);
}

void setup_gps() {
  ss.begin(9600, SERIAL_8N1, GPS_TX, GPS_RX);
}

char *s_min_nn(uint32_t min_nnnnn, int high_precision) {
  /* min_nnnnn: RawDegrees billionths is uint32_t by definition and is n'telth
   * degree (-> *= 6 -> nn.mmmmmm minutes) high_precision: 0: round at decimal
   * position 2. 1: round at decimal position 4. 2: return decimal position 3-4
   * as base91 encoded char
   */

  static char buf[6];
  min_nnnnn = min_nnnnn * 0.006;

  if (high_precision) {
    if ((min_nnnnn % 10) >= 5 && min_nnnnn < 6000000 - 5) {
      // round up. Avoid overflow (59.999999 should never become 60.0 or more)
      min_nnnnn = min_nnnnn + 5;
    }
  } else {
    if ((min_nnnnn % 1000) >= 500 && min_nnnnn < (6000000 - 500)) {
      // round up. Avoid overflow (59.9999 should never become 60.0 or more)
      min_nnnnn = min_nnnnn + 500;
    }
  }

  if (high_precision < 2)
    sprintf(buf, "%02u.%02u", (unsigned int)((min_nnnnn / 100000) % 100), (unsigned int)((min_nnnnn / 1000) % 100));
  else
    sprintf(buf, "%c", (char)((min_nnnnn % 1000) / 11) + 33);
  // Like to verify? type in python for i.e. RawDegrees billions 566688333: i =
  // 566688333; "%c" % (int(((i*.0006+0.5) % 100)/1.1) +33)
  return buf;
}

String create_lat_aprs(RawDegrees lat) {
  char str[20];
  char n_s = 'N';
  if (lat.negative) {
    n_s = 'S';
  }
  // we like sprintf's float up-rounding.
  // but sprintf % may round to 60.00 -> 5360.00 (53° 60min is a wrong notation
  // ;)
  sprintf(str, "%02d%s%c", lat.deg, s_min_nn(lat.billionths, 0), n_s);
  String lat_str(str);
  return lat_str;
}

String create_lat_aprs_dao(RawDegrees lat) {
  // round to 4 digits and cut the last 2
  char str[20];
  char n_s = 'N';
  if (lat.negative) {
    n_s = 'S';
  }
  // we need sprintf's float up-rounding. Must be the same principle as in
  // aprs_dao(). We cut off the string to two decimals afterwards. but sprintf %
  // may round to 60.0000 -> 5360.0000 (53° 60min is a wrong notation ;)
  sprintf(str, "%02d%s%c", lat.deg, s_min_nn(lat.billionths, 1 /* high precision */), n_s);
  String lat_str(str);
  return lat_str;
}

String create_long_aprs(RawDegrees lng) {
  char str[20];
  char e_w = 'E';
  if (lng.negative) {
    e_w = 'W';
  }
  sprintf(str, "%03d%s%c", lng.deg, s_min_nn(lng.billionths, 0), e_w);
  String lng_str(str);
  return lng_str;
}

String create_long_aprs_dao(RawDegrees lng) {
  // round to 4 digits and cut the last 2
  char str[20];
  char e_w = 'E';
  if (lng.negative) {
    e_w = 'W';
  }
  sprintf(str, "%03d%s%c", lng.deg, s_min_nn(lng.billionths, 1 /* high precision */), e_w);
  String lng_str(str);
  return lng_str;
}

String create_dao_aprs(RawDegrees lat, RawDegrees lng) {
  // !DAO! extension, use Base91 format for best precision
  // /1.1 : scale from 0-99 to 0-90 for base91, int(... + 0.5): round to nearest
  // integer https://metacpan.org/dist/Ham-APRS-FAP/source/FAP.pm
  // http://www.aprs.org/aprs12/datum.txt
  //

  char str[10];
  sprintf(str, "!w%s", s_min_nn(lat.billionths, 2));
  sprintf(str + 3, "%s!", s_min_nn(lng.billionths, 2));
  String dao_str(str);
  return dao_str;
}

String createDateString(time_t t) {
  return String(padding(day(t), 2) + "." + padding(month(t), 2) + "." + padding(year(t), 4));
}

String createTimeString(time_t t) {
  return String(padding(hour(t), 2) + ":" + padding(minute(t), 2) + ":" + padding(second(t), 2));
}

String getSmartBeaconState() {
  if (BeaconMan.getCurrentBeaconConfig()->smart_beacon.active) {
    return "On";
  }
  return "Off";
}

String padding(unsigned int number, unsigned int width) {
  String result;
  String num(number);
  if (num.length() > width) {
    width = num.length();
  }
  for (unsigned int i = 0; i < width - num.length(); i++) {
    result.concat('0');
  }
  result.concat(num);
  return result;
}
