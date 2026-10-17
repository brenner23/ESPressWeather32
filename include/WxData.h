#pragma once
// ESPressWeather32: gemeinsame Daten. Der Netz-Task schreibt Wetter/Vorhersage,
// loop() zeichnet und bedient das Web. Zugriff immer ueber g_dataMtx.
#include <Arduino.h>

struct WxNow {
  bool     valid;
  float    temp;
  float    feels;
  int      humidity;
  int      pressure;
  float    wind;         // m/s (metric) bzw. mph (imperial)
  uint32_t sunrise;      // Sonnenaufgang heute (Unix), 0 = unbekannt
  uint32_t sunset;       // Sonnenuntergang heute (Unix)
  float    lat, lon;     // Koordinaten des Ortes (kommen mit der Wetterantwort)
  int      conditionId;
  char     icon[4];      // OpenWeather-Iconcode, "n" = Nacht
  char     desc[40];
  uint32_t epoch;        // Zeitpunkt des Abrufs (Unix)
};

struct WxDay {
  int8_t  wday;          // 0 = Sonntag ... 6 = Samstag, -1 = unbekannt
  float   tempMax;
  float   tempMin;
  int     conditionId;
  int     pop;           // Regenwahrscheinlichkeit in %
};

struct WxForecast {
  bool     valid;
  WxDay    days[3];
  uint32_t epoch;
};

// Energie-Daten einer Tasmota-Steckdose (per MQTT, dieses Geraet ist der Broker)
struct WxEnergy {
  bool     valid;        // schon Energiedaten empfangen?
  bool     online;       // LWT
  char     name[24];     // aus dem Topic
  char     powerState[4];
  float    power, today, yesterday, total, voltage, current;
  uint32_t lastMsgMs;
};

#define VIEW_LIVE     0
#define VIEW_FORECAST 1
#define VIEW_ENERGY   2
#define VIEW_MOON     3   // Variante von Screen 1: Uhr + Wetter + Mondphase + Wind
#define VIEW_AUTO     4
#define VIEW_CLASSIC  5   // Wetter 1: erste Version (schwarz, ohne Himmel/Bogen/Zusatzflaeche)
