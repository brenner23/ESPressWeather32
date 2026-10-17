/*
  ESPressWeather32 - Wetteranzeige fuer ESP32-Displays
  CYD (ESP32-2432S028), NM-TV-154 und MiniTV ESP32-C2 in einem Projekt.

  Uhr + aktuelles Wetter, 3-Tage-Vorhersage (OpenWeather) und Energie-Ansicht
  (Tasmota-Steckdose per MQTT, dieses Geraet ist der Broker). Einrichtungs-Portal,
  Web-Dashboard, Einstellungen im NVS, optional secrets.ini.

  Aufbau: Ein Netz-Task holt die Wetterdaten (blockierende HTTP-Abrufe), loop()
  zeichnet und bedient Web, OTA, MQTT, Touch und Licht. So laufen Webseite, OTA und
  Uhr auch auf dem einkernigen ESP32-C2 waehrend eines Abrufs weiter.

  https://github.com/brenner23/ESPressWeather32 - MIT-Lizenz
*/
#define WX_VERSION "1.0"
#define WX_PORTAL_PASSWORD "espressweather"
#ifndef WX_WEB_PREVIEW
  #define WX_WEB_PREVIEW 0   // 1 = Displaybild als Debug-Vorschau im Web (/api/screen), 0 = aus
#endif

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <ArduinoOTA.h>
#include <Preferences.h>
#include <SPI.h>
#include <time.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>

#include "pins.h"
#include "WxSettings.h"
#include "WxData.h"

#if WX_HAS_XPT
  #include <XPT2046_Touchscreen.h>
#endif

#ifndef OTA_PASSWORD
  #define OTA_PASSWORD ""
#endif

// ---------------------------------------------------------------- Display
#if defined(CONFIG_IDF_TARGET_ESP32C2)
  SPIClass wx_tft_spi(FSPI);   // ESP32-C2 hat nur einen freien SPI-Host
#else
  SPIClass wx_tft_spi(HSPI);
#endif
Adafruit_ST7789 tft_hw(&wx_tft_spi, TFT_CS, TFT_DC, TFT_RST);
Adafruit_GFX*   g_gfx = &tft_hw;   // siehe WxGfx.h (Vorschau im Web rendert ueber WxCapture)
#include "WxGfx.h"

#include "WxDisplay.h"
#include "WxViews.h"
#include "WxMqtt.h"
#include "WxWebPage.h"

WebServer server(80);

// ---------------------------------------------------------------- gemeinsame Daten
static SemaphoreHandle_t g_mtx = nullptr;
struct WxLock { WxLock() { xSemaphoreTake(g_mtx, portMAX_DELAY); } ~WxLock() { xSemaphoreGive(g_mtx); } };

static WxNow      g_now = {};
static WxForecast g_fc  = {};
static char       g_wxErr[56] = "";
static volatile bool g_nowDirty = false, g_fcDirty = false, g_forceFetch = false, g_otaBusy = false;

static const uint32_t WX_FC_INTERVAL_S  = 2 * 60 * 60;

static bool wx_time_valid() { return time(nullptr) > 1700000000; }

// ================================================================ Licht (Backlight, LDR)
static const uint16_t BL_FREQ = 1000;
static const uint8_t  BL_RES  = 8;
static bool g_lightOn = true;            // Touch: Licht an/aus (Anzeige laeuft weiter)

// LDR wie bei ESPressDuino: GPIO34, 12 Bit, 11 dB, 10 Messungen. CYD: HELL = niedrig, DUNKEL = hoch.
#define WX_LDR_BRIGHT    0
#define WX_LDR_DARK    900
#define WX_DIM_MEASURE 500UL
#define WX_DIM_STEP     15
static unsigned long g_previewUntil = 0;   // Web-Vorschau hat bis dahin Vorrang vor dem LDR
static uint16_t g_ldrRaw = 0;
static uint8_t  g_ldrOutPercent = 100;
static int      g_ldrCurPwm = 255, g_ldrTargetPwm = 255;

static void wx_bl_write(int duty) { ledcWrite(TFT_BACKLIGHT, WX_BL_PWM(constrain(duty, 0, 255))); }
static int  wx_user_pwm() { return map(g_disp.brightness, 0, 100, 0, 255); }

static void wx_apply_light() {
  if (!g_lightOn) { wx_bl_write(0); return; }
  wx_bl_write(g_disp.autoLight ? g_ldrCurPwm : wx_user_pwm());
}

static void wx_backlight_setup() {
  ledcAttach(TFT_BACKLIGHT, BL_FREQ, BL_RES);
  wx_bl_write(255);   // Start, WLAN und Portal immer mit voller Helligkeit
}

static void wx_ldr_setup() {
#if WX_HAS_LDR
  analogReadResolution(12);
  pinMode(LDR_PIN, INPUT);
  analogSetPinAttenuation(LDR_PIN, ADC_11db);
  (void)analogRead(LDR_PIN);   // ersten ADC-Zyklus verwerfen
#endif
  g_ldrCurPwm = g_ldrTargetPwm = wx_user_pwm();
  g_ldrOutPercent = g_disp.brightness;
}

static void wx_ldr_tick() {
#if WX_HAS_LDR
  static unsigned long lastRead = 0, lastStep = 0;
  const unsigned long now = millis();
  if (now - lastRead >= WX_DIM_MEASURE) {
    lastRead = now;
    pinMode(LDR_PIN, INPUT);
    (void)analogRead(LDR_PIN);
    uint32_t sum = 0;
    for (uint8_t i = 0; i < 10; ++i) { sum += analogRead(LDR_PIN); delay(2); }
    g_ldrRaw = (uint16_t)(sum / 10U);
    if (g_disp.autoLight) {
      const int bMin = max(1, wx_user_pwm()), bMax = 255;
      g_ldrTargetPwm = constrain((int)map((long)g_ldrRaw, WX_LDR_BRIGHT, WX_LDR_DARK, bMax, bMin), bMin, bMax);
    } else {
      g_ldrTargetPwm = wx_user_pwm();
    }
  }
  // Sanft zum Ziel: alle 50 ms um 15 PWM-Schritte
  if (g_disp.autoLight && (long)(now - g_previewUntil) >= 0 && now - lastStep >= 50UL) {
    lastStep = now;
    if (g_ldrCurPwm < g_ldrTargetPwm)      g_ldrCurPwm = min(g_ldrTargetPwm, g_ldrCurPwm + WX_DIM_STEP);
    else if (g_ldrCurPwm > g_ldrTargetPwm) g_ldrCurPwm = max(g_ldrTargetPwm, g_ldrCurPwm - WX_DIM_STEP);
    wx_bl_write(g_lightOn ? g_ldrCurPwm : 0);   // Touch-AUS hat Vorrang vor dem LDR
  }
  g_ldrOutPercent = (uint8_t)constrain((g_ldrCurPwm * 100 + 127) / 255, 0, 100);
#endif
}

static void wx_toggle_light() {
  g_lightOn = !g_lightOn;
  wx_apply_light();
  Serial.printf("[Touch] light %s\n", g_lightOn ? "on" : "off");
}

// ================================================================ Ansichten
static uint8_t  g_shown = 255;           // gerade gezeigte Ansicht (VIEW_LIVE/FORECAST/ENERGY)
static bool     g_needFull = true;
static bool     g_overlay = false;       // Hinweisleiste (Countdown) liegt ueber der Ansicht
static bool     g_portalShown = false;
static uint8_t  g_cycleIdx = 0;
static unsigned long g_cycleAt = 0;
static int      g_lastMin = -1, g_lastDay = -1;

static uint8_t wx_views(uint8_t* list) {
  uint8_t n = 0;
  list[n++] = VIEW_CLASSIC;
  list[n++] = VIEW_LIVE;
  list[n++] = VIEW_MOON;
  list[n++] = VIEW_FORECAST;
  if (g_wx.mqttOn) list[n++] = VIEW_ENERGY;
  return n;
}

static uint8_t wx_wanted_view() {
  uint8_t list[5]; const uint8_t n = wx_views(list);
  if (g_disp.view != VIEW_AUTO) return (g_disp.view == VIEW_ENERGY && !g_wx.mqttOn) ? VIEW_LIVE : g_disp.view;
  if (millis() - g_cycleAt >= g_disp.cycleSec * 1000UL) { g_cycleAt = millis(); g_cycleIdx++; }
  return list[g_cycleIdx % n];
}

static const char* wx_view_name(uint8_t v) {
  switch (v) {
    case VIEW_FORECAST: return "3-day forecast";
    case VIEW_ENERGY:   return "Energy";
    case VIEW_MOON:     return "Weather 3 (moon)";
    case VIEW_CLASSIC:  return "Weather 1";
    default:            return "Weather 2";
  }
}

// Doppeltippen: naechste Ansicht
static void wx_next_view() {
  uint8_t list[5]; const uint8_t n = wx_views(list);
  if (g_disp.view == VIEW_AUTO) { g_cycleIdx++; g_cycleAt = millis(); return; }
  uint8_t i = 0;
  while (i < n && list[i] != g_disp.view) i++;
  g_disp.view = list[(i + 1) % n];
  wx_display_save();
}

static unsigned long g_previewUntil2 = 0;   // Himmel-Vorschau endet (60 s)

// Himmel-Stufen, Mondphase, Mondzeiten jede Minute aktualisieren
static void wx_sky_tick(bool force) {
  static unsigned long last = 0;
  if (!force && millis() - last < 20000UL) return;
  last = millis();
  if (g_skyPreview && (long)(millis() - g_previewUntil2) >= 0) { g_skyPreview = 0; force = true; }
  WxNow w; { WxLock l; w = g_now; }
  const time_t now = time(nullptr);
  g_moonPhase = wx_moon_phase(now);
  if (w.valid && (w.lat != 0 || w.lon != 0) && wx_time_valid()) wx_moon_times(now, w.lat, w.lon);
  const bool changed = wx_sky_update(w, now);
  if (changed || force) {
    wx_sky_rows();
    if (g_shown != 255) g_needFull = true;
  }
}

static unsigned long g_lastFullMs = 0;
static void wx_draw_full() {
  g_lastFullMs = millis();
  g_nowDirty = g_fcDirty = g_tasDirty = false;
  g_skyActive = true;   // ein Hintergrund fuer alle Ansichten: erst Himmel, dann Inhalt
  const time_t now = time(nullptr); struct tm t; localtime_r(&now, &t);
  g_lastMin = wx_time_valid() ? t.tm_min : -1;
  g_lastDay = wx_time_valid() ? t.tm_mday : -1;
  switch (g_shown) {
    case VIEW_FORECAST: { WxForecast f; { WxLock l; f = g_fc; } wx_forecast_screen(f); break; }
    case VIEW_ENERGY:   wx_energy_screen(g_tas, wx_energy_name(), true); break;
    case VIEW_MOON:     { WxNow w; { WxLock l; w = g_now; } wx_w2_full(w, now); break; }
    case VIEW_CLASSIC:  { WxNow w; { WxLock l; w = g_now; } wx_w1_full(w, now, true); break; }
    default:            { WxNow w; { WxLock l; w = g_now; } wx_w1_full(w, now, false); break; }
  }
}

#if WX_WEB_PREVIEW
// Vorschau fuer das Web: zeichnet die aktuelle Ansicht ohne Nebenwirkungen
static void wx_render_view() {
  const time_t now = time(nullptr);
  switch (g_shown) {
    case VIEW_FORECAST: { WxForecast f; { WxLock l; f = g_fc; } wx_forecast_screen(f); break; }
    case VIEW_ENERGY:   { const int keep = wx_energy_last_min; char lastRows[4][24]; memcpy(lastRows, wx_energy_last, sizeof(lastRows));
                          wx_energy_screen(g_tas, wx_energy_name(), true); wx_energy_last_min = keep; memcpy(wx_energy_last, lastRows, sizeof(lastRows)); break; }
    case VIEW_MOON:     { WxNow w; { WxLock l; w = g_now; } wx_w2_full(w, now); break; }
    case VIEW_CLASSIC:  { WxNow w; { WxLock l; w = g_now; } wx_w1_full(w, now, true); break; }
    default:            { WxNow w; { WxLock l; w = g_now; } wx_w1_full(w, now, false); break; }
  }
}
#endif
static void wx_view_tick() {
  if (g_portalShown || g_overlay || g_otaBusy) return;
  wx_sky_tick(false);
  const uint8_t want = wx_wanted_view();
  if (want != g_shown) { g_shown = want; g_needFull = true; }
  if (g_needFull) { g_needFull = false; wx_draw_full(); return; }

  const time_t now = time(nullptr); struct tm t; localtime_r(&now, &t);
  const bool valid = wx_time_valid();
  switch (g_shown) {
    case VIEW_CLASSIC:
    case VIEW_LIVE:
    case VIEW_MOON:
      // Neues Wetter oder neuer Tag: alles neu (Mond, Sonnenzeiten, Datum)
      if (g_nowDirty || (valid && t.tm_mday != g_lastDay)) { g_nowDirty = false; g_needFull = true; break; }
      if (valid && t.tm_min != g_lastMin) {
        g_lastMin = t.tm_min;
        WxNow w; { WxLock l; w = g_now; }
        if (g_shown == VIEW_CLASSIC)   wx_w1_clock(&t, true);
        else if (g_shown == VIEW_LIVE) { wx_w1_clock(&t, true); wx_w1_arc(w, now); }
        else                      { wx_w2_clock(&t, true); wx_w2_arc(w, now); }
        if (t.tm_min == 0 && g_shown != VIEW_CLASSIC) { if (g_shown == VIEW_MOON) wx_w2_top(w); wx_panel(w, g_shown == VIEW_MOON); }   // Mond stuendlich
      }
      break;
    case VIEW_FORECAST:
      if (g_fcDirty) { g_fcDirty = false; WxForecast f; { WxLock l; f = g_fc; } wx_forecast_screen(f); }
      break;
    case VIEW_ENERGY:
      if (g_tasDirty) { g_tasDirty = false; wx_energy_screen(g_tas, wx_energy_name(), false); }
      else wx_energy_clock(false);
      break;
  }
}

static void wx_apply_rotation() {
  tft.setRotation((g_disp.rotation / 90) & 3);
  wx_disp_offsets();
  wx_sky_rows();
  if (g_portalShown) g_portalShown = false;   // Portal-Bild wird unten neu gezeichnet
  g_needFull = true;
}

// ================================================================ OpenWeather (Netz-Task)
static String wx_urlenc(const String& s) {
  String o; char b[4];
  for (unsigned char c : s) {
    if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') o += (char)c;
    else { snprintf(b, sizeof(b), "%%%02X", c); o += b; }
  }
  return o;
}

static String wx_owm_url(const char* api, const char* extra) {
  return String("http://api.openweathermap.org/data/2.5/") + api + "?q=" + wx_urlenc(g_wx.owCity) +
         (g_wx.owCountry.length() ? "," + wx_urlenc(g_wx.owCountry) : String("")) +
         "&units=" + g_wx.owUnits + "&lang=" + wx_urlenc(g_wx.owLang) + extra + "&appid=" + wx_urlenc(g_wx.owKey);
}

static volatile int g_lastHttp = 0;

// Abruf-Bremse: hoechstens WX_MAX_CALLS_DAY OpenWeather-Abrufe pro Tag und Geraet
#define WX_MAX_CALLS_DAY 1000
static uint16_t g_callsToday = 0;
static int      g_callDay = -1;

static void wx_set_error(const char* what, int code) {
  g_lastHttp = code;
  WxLock l;
  if (code == 401)      snprintf(g_wxErr, sizeof(g_wxErr), "OpenWeather: API key rejected (401)");
  else if (code == 429) snprintf(g_wxErr, sizeof(g_wxErr), "OpenWeather: too many calls (429)");
  else if (code == 404) snprintf(g_wxErr, sizeof(g_wxErr), "OpenWeather: city not found (404)");
  else if (code < 0)    snprintf(g_wxErr, sizeof(g_wxErr), "%s: no connection (%d)", what, code);
  else                  snprintf(g_wxErr, sizeof(g_wxErr), "%s: HTTP %d", what, code);
  Serial.printf("[Weather] %s\n", g_wxErr);
}

static bool wx_call_allowed() {
  const time_t n = time(nullptr); struct tm t; localtime_r(&n, &t);
  if (t.tm_yday != g_callDay) { g_callDay = t.tm_yday; g_callsToday = 0; }
  if (g_callsToday >= WX_MAX_CALLS_DAY) {
    WxLock l; snprintf(g_wxErr, sizeof(g_wxErr), "Daily limit reached (%d calls)", WX_MAX_CALLS_DAY);
    return false;
  }
  g_callsToday++;
  return true;
}

// Nach Fehlern wachsender Abstand: 1, 2, 4 ... 15 min; Key ungueltig / zu viele Abrufe: 1 h
struct WxBackoff { unsigned long next = 0; uint32_t wait = 0; };
static void wx_backoff(WxBackoff& b, bool ok) {
  if (ok) { b.wait = 0; b.next = 0; return; }
  if (g_lastHttp == 401 || g_lastHttp == 429) b.wait = 3600000UL;
  else b.wait = b.wait ? min<uint32_t>(b.wait * 2, 900000UL) : 60000UL;
  b.next = millis() + b.wait;
  Serial.printf("[Weather] next try in %lu s\n", (unsigned long)(b.wait / 1000));
}

static void wx_cache_save(const char* key, const void* data, size_t len) {
  Preferences p; p.begin("wx_cache", false); p.putBytes(key, data, len); p.end();
}

static void wx_cache_load() {
  Preferences p; p.begin("wx_cache", true);
  if (p.getBytesLength("now") == sizeof(WxNow)) p.getBytes("now", &g_now, sizeof(WxNow));
  if (p.getBytesLength("fc") == sizeof(WxForecast)) p.getBytes("fc", &g_fc, sizeof(WxForecast));
  p.end();
}

static bool wx_fetch_now() {
  WiFiClient client; HTTPClient http;
  http.setTimeout(8000);
  http.useHTTP10(true);   // kein Chunked-Encoding -> direkt aus dem Stream lesen
  if (!wx_call_allowed()) { g_lastHttp = 429; return false; }
  if (!http.begin(client, wx_owm_url("weather", ""))) return false;
  const int code = http.GET();
  bool ok = false;
  if (code == HTTP_CODE_OK) {
    JsonDocument filter;
    filter["main"]["temp"] = true; filter["main"]["feels_like"] = true; filter["wind"]["speed"] = true;
    filter["sys"]["sunrise"] = true; filter["sys"]["sunset"] = true;
    filter["coord"]["lat"] = true; filter["coord"]["lon"] = true;
    filter["main"]["humidity"] = true; filter["main"]["pressure"] = true;
    filter["weather"][0]["id"] = true; filter["weather"][0]["icon"] = true; filter["weather"][0]["description"] = true;
    JsonDocument doc;
    if (!deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter))) {
      WxNow w = {};
      w.valid       = true;
      w.temp        = doc["main"]["temp"] | 0.0f;
      w.feels       = doc["main"]["feels_like"] | w.temp;
      w.wind        = doc["wind"]["speed"] | 0.0f;
      w.sunrise     = doc["sys"]["sunrise"] | 0;
      w.sunset      = doc["sys"]["sunset"] | 0;
      w.lat         = doc["coord"]["lat"] | 0.0f;
      w.lon         = doc["coord"]["lon"] | 0.0f;
      w.humidity    = doc["main"]["humidity"] | 0;
      w.pressure    = doc["main"]["pressure"] | 1013;
      w.conditionId = doc["weather"][0]["id"] | 800;
      strlcpy(w.icon, doc["weather"][0]["icon"] | "01d", sizeof(w.icon));
      strlcpy(w.desc, doc["weather"][0]["description"] | "", sizeof(w.desc));
      w.epoch = (uint32_t)time(nullptr);
      { WxLock l; g_now = w; g_wxErr[0] = 0; }
      wx_cache_save("now", &w, sizeof(w));
      g_nowDirty = true;
      ok = true;
      Serial.printf("[Weather] %.1f, %d %%, %d hPa, id %d\n", w.temp, w.humidity, w.pressure, w.conditionId);
    } else wx_set_error("Weather", -100);
  } else wx_set_error("Weather", code);
  http.end();
  return ok;
}

static bool wx_fetch_forecast() {
  WiFiClient client; HTTPClient http;
  http.setTimeout(10000);
  http.useHTTP10(true);
  if (!wx_call_allowed()) { g_lastHttp = 429; return false; }
  if (!http.begin(client, wx_owm_url("forecast", "&cnt=32"))) return false;
  const int code = http.GET();
  bool ok = false;
  if (code == HTTP_CODE_OK) {
    JsonDocument filter;
    filter["list"][0]["dt"] = true;
    filter["list"][0]["main"]["temp_max"] = true;
    filter["list"][0]["main"]["temp_min"] = true;
    filter["list"][0]["weather"][0]["id"] = true;
    filter["list"][0]["pop"] = true;
    JsonDocument doc;
    if (!deserializeJson(doc, http.getStream(), DeserializationOption::Filter(filter))) {
      WxForecast f = {};
      for (auto& d : f.days) { d.wday = -1; d.tempMax = -999; d.tempMin = 999; d.conditionId = 803; }
      const time_t now = time(nullptr);
      struct tm cur; localtime_r(&now, &cur);
      int idx = -1, lastDay = -1;
      for (JsonObject e : doc["list"].as<JsonArray>()) {
        const time_t dt = e["dt"] | 0;
        struct tm t; localtime_r(&dt, &t);
        if (t.tm_mday == cur.tm_mday) continue;   // heute ueberspringen
        if (t.tm_mday != lastDay) {
          if (++idx >= 3) break;
          lastDay = t.tm_mday;
          f.days[idx].wday = t.tm_wday;
          f.days[idx].conditionId = e["weather"][0]["id"] | 803;
        }
        WxDay& d = f.days[idx];
        const float mx = e["main"]["temp_max"] | 0.0f, mn = e["main"]["temp_min"] | 0.0f;
        if (mx > d.tempMax) d.tempMax = mx;
        if (mn < d.tempMin) d.tempMin = mn;
        const int pop = (int)((e["pop"] | 0.0f) * 100);
        if (pop > d.pop) d.pop = pop;
        if (t.tm_hour >= 11 && t.tm_hour <= 14) d.conditionId = e["weather"][0]["id"] | d.conditionId;   // Mittagswetter
      }
      for (auto& d : f.days) if (d.wday < 0) { d.tempMax = d.tempMin = 0; }
      f.valid = idx >= 0;
      f.epoch = (uint32_t)now;
      { WxLock l; g_fc = f; }
      wx_cache_save("fc", &f, sizeof(f));
      g_fcDirty = true;
      ok = f.valid;
      Serial.printf("[Forecast] %d days\n", idx + 1 > 3 ? 3 : idx + 1);
    } else wx_set_error("Forecast", -100);
  } else wx_set_error("Forecast", code);
  http.end();
  return ok;
}

static void wx_net_task(void*) {
  WxBackoff bNow, bFc;
  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(500));
    if (g_otaBusy || WiFi.status() != WL_CONNECTED || !wx_time_valid()) continue;
    if (g_wx.owKey.isEmpty() || g_wx.owCity.isEmpty()) {
      WxLock l; strlcpy(g_wxErr, "No OpenWeather API key / city set", sizeof(g_wxErr));
      continue;
    }
    const bool force = g_forceFetch;
    g_forceFetch = false;
    const uint32_t ep = (uint32_t)time(nullptr);
    uint32_t nowEp, fcEp; { WxLock l; nowEp = g_now.valid ? g_now.epoch : 0; fcEp = g_fc.valid ? g_fc.epoch : 0; }
    if ((force && !bNow.wait) || (ep - nowEp >= g_disp.wxMin * 60UL && (!bNow.wait || (long)(millis() - bNow.next) >= 0)))
      wx_backoff(bNow, wx_fetch_now());
    if ((force && !bFc.wait) || (ep - fcEp >= WX_FC_INTERVAL_S && (!bFc.wait || (long)(millis() - bFc.next) >= 0)))
      wx_backoff(bFc, wx_fetch_forecast());
  }
}

// ================================================================ Web, Portal, WLAN
static const IPAddress WX_PORTAL_IP(192, 168, 4, 1);
static DNSServer g_dns;
static bool      g_portal = false, g_portalCanClose = false, g_serverStarted = false;
static String    g_apName;
static uint32_t  g_wifiDrops = 0, g_wifiReconnects = 0;

static void wx_restart(const char* why) {
  Serial.printf("[System] restart: %s\n", why);
  if (!g_portalShown) wx_info_bar(wx_lang_de() ? "Neustart ..." : "Restarting ...");
  delay(300);
  ESP.restart();
}

static void wx_start_portal(bool keepSta) {
  if (g_portal) return;
  uint64_t mac = ESP.getEfuseMac();
  char ssid[32];
  snprintf(ssid, sizeof(ssid), "ESPressWeather-%02X%02X", (uint8_t)(mac >> 32), (uint8_t)(mac >> 40));
  g_apName = ssid;
  WiFi.mode(keepSta ? WIFI_AP_STA : WIFI_AP);
  WiFi.softAPConfig(WX_PORTAL_IP, WX_PORTAL_IP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(ssid, WX_PORTAL_PASSWORD);
  g_dns.setErrorReplyCode(DNSReplyCode::NoError);
  g_dns.start(53, "*", WX_PORTAL_IP);
  g_portal = true;
  g_portalCanClose = keepSta && wx_settings_configured();
  g_overlay = false;
  wx_bl_write(255);   // Portal immer mit voller Helligkeit
  wx_portal_screen(g_apName, WX_PORTAL_PASSWORD, g_portalCanClose);
  g_portalShown = true;
  Serial.println();
  Serial.println("==============================================");
  Serial.println("  SETUP PORTAL ACTIVE");
  Serial.printf("  Wi-Fi:    %s\n", ssid);
  Serial.printf("  Password: %s\n", WX_PORTAL_PASSWORD);
  Serial.println("  Page:     http://192.168.4.1/");
  Serial.println("==============================================");
}

static void wx_stop_portal() {
  if (!g_portal) return;
  g_dns.stop();
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_STA);
  g_portal = false;
  g_portalShown = false;
  g_needFull = true;
  wx_apply_light();
  Serial.println("[Web] setup portal closed");
}

static void wx_send_json(JsonDocument& doc) {
  String out; serializeJson(doc, out);
  server.sendHeader("Cache-Control", "no-store");
  server.send(200, "application/json", out);
}

static void wx_handle_root() {
  server.sendHeader("Cache-Control", "no-store");   // nach Firmware-Updates immer die neue Seite
  server.send_P(200, "text/html; charset=utf-8", WX_WEB_PAGE);
}

static void wx_handle_stats() {
  JsonDocument d;
  WxNow w; WxForecast f; String err;
  { WxLock l; w = g_now; f = g_fc; err = g_wxErr; }
  const uint32_t ep = wx_time_valid() ? (uint32_t)time(nullptr) : 0;
  d["name"]    = g_wx.deviceName;
  d["version"] = String("ESPressWeather32 v") + WX_VERSION + " (" + WX_BOARD + ")";
  d["units"]   = g_wx.owUnits;
  d["city"]    = g_wx.owCity;
  d["country"] = g_wx.owCountry;
  JsonObject jw = d["weather"].to<JsonObject>();
  jw["valid"] = w.valid;
  if (w.valid) {
    jw["temp"] = w.temp; jw["feels"] = w.feels; jw["hum"] = w.humidity; jw["press"] = w.pressure; jw["wind"] = w.wind;
    jw["cond"] = w.conditionId; jw["night"] = strchr(w.icon, 'n') != nullptr; jw["desc"] = w.desc;
    jw["age"] = (ep && w.epoch) ? (int32_t)(ep - w.epoch) : -1;
  }
  if (err.length()) d["wxError"] = err;
  d["apiCalls"] = g_callsToday;
  d["apiMax"]   = WX_MAX_CALLS_DAY;
  JsonArray jf = d["forecast"].to<JsonArray>();
  if (f.valid) for (const WxDay& x : f.days) {
    if (x.wday < 0) continue;
    JsonObject o = jf.add<JsonObject>();
    o["day"] = wx_wd_short(x.wday); o["max"] = x.tempMax; o["min"] = x.tempMin; o["cond"] = x.conditionId; o["pop"] = x.pop;
  }
  JsonObject je = d["energy"].to<JsonObject>();
  je["on"] = g_wx.mqttOn;
  je["port"] = g_wx.mqttPort;
  je["valid"] = g_tas.valid;
  je["name"] = wx_energy_name();
  je["state"] = g_tas.powerState;
  je["power"] = g_tas.power; je["today"] = g_tas.today; je["yesterday"] = g_tas.yesterday;
  je["total"] = g_tas.total; je["voltage"] = g_tas.voltage; je["current"] = g_tas.current;
  je["age"] = g_tas.lastMsgMs ? (int32_t)((millis() - g_tas.lastMsgMs) / 1000UL) : -1;
  d["viewName"]       = g_shown != 255 ? wx_view_name(g_shown) : "-";
  d["lightOn"]        = g_lightOn;
  d["ip"]             = WiFi.localIP().toString();
  d["rssi"]           = WiFi.RSSI();
  d["wifiDrops"]      = g_wifiDrops;
  d["wifiReconnects"] = g_wifiReconnects;
  d["uptime"]         = (uint32_t)(millis() / 1000UL);
  d["heap"]           = ESP.getFreeHeap();
  d["portal"]         = g_portal;
  d["build"]          = __DATE__ " " __TIME__;   // Seite laedt sich nach einem Firmware-Update selbst neu
  wx_send_json(d);
}

static void wx_handle_config_get() {
  JsonDocument d;
  wx_settings_to_json(d.to<JsonObject>());
  wx_send_json(d);
}

static void wx_handle_config_post() {
  if (wx_settings_from_secrets()) { server.send(403, "text/plain", "Settings come from secrets.ini (use_secrets = 1)"); return; }
  JsonDocument d;
  if (deserializeJson(d, server.arg("plain"))) { server.send(400, "text/plain", "Invalid JSON"); return; }
  WxSettings s = g_wx;
  wx_settings_apply_json(d.as<JsonVariantConst>(), s);
  if (const char* err = wx_settings_validate(s)) { server.send(400, "text/plain", err); return; }
  if (!wx_settings_save(s)) { server.send(500, "text/plain", "Saving failed"); return; }
  // Neue Stadt/Einheit: alten Wetter-Zwischenspeicher verwerfen
  { Preferences p; p.begin("wx_cache", false); p.clear(); p.end(); }
  server.send(200, "text/plain", "Saved - restarting ...");
  delay(500);
  wx_restart("settings saved");
}

static void wx_handle_scan() {
  const int n = WiFi.scanNetworks();
  JsonDocument d;
  JsonArray nets = d["nets"].to<JsonArray>();
  for (int i = 0; i < n && nets.size() < 30; i++) {
    String ssid = WiFi.SSID(i);
    if (ssid.isEmpty()) continue;
    bool dup = false;
    for (JsonObject o : nets) if (ssid == o["ssid"].as<const char*>()) { dup = true; break; }
    if (dup) continue;
    JsonObject o = nets.add<JsonObject>();
    o["ssid"] = ssid;
    o["rssi"] = WiFi.RSSI(i);
    o["lock"] = WiFi.encryptionType(i) != WIFI_AUTH_OPEN;
  }
  WiFi.scanDelete();
  wx_send_json(d);
}

static void wx_handle_display_get() {
  JsonDocument d;
  d["board"]       = WX_BOARD;
  d["brightness"]  = g_disp.brightness;
  d["hasLdr"]      = WX_HAS_LDR ? true : false;
  d["autoLight"]   = g_disp.autoLight;
  d["hasRotation"] = WX_HAS_ROTATION ? true : false;
  d["rotation"]    = g_disp.rotation;
  d["view"]        = g_disp.view;
  d["cycleSec"]    = g_disp.cycleSec;
  d["wxMin"]       = g_disp.wxMin;
  d["skyPreview"]  = g_skyPreview;
  String st = g_disp.autoLight ? String("Auto light on · display now ") + g_ldrOutPercent + " %" : String("");
  if (!g_lightOn) st += (st.length() ? " · " : "") + String("light switched off by touch");
  d["status"] = st;
  wx_send_json(d);
}

static void wx_handle_display_post() {
  JsonDocument j;
  if (deserializeJson(j, server.arg("plain"))) { server.send(400, "text/plain", "Invalid JSON"); return; }
  if (!j["rotation"].isNull()) {
    const int r = j["rotation"].as<int>();
    if (r != 0 && r != 90 && r != 180 && r != 270) { server.send(400, "text/plain", "Rotation: 0, 90, 180 or 270"); return; }
#if WX_HAS_ROTATION
    if (r != g_disp.rotation) { g_disp.rotation = r; wx_apply_rotation(); }
#endif
  }
  if (!j["view"].isNull()) {
    const int v = j["view"].as<int>();
    if (v < 0 || v > 5) { server.send(400, "text/plain", "View: 0 to 5"); return; }
    g_disp.view = v; g_cycleAt = millis();
  }
  if (!j["cycleSec"].isNull()) g_disp.cycleSec = constrain(j["cycleSec"].as<int>(), 5, 3600);
  if (!j["wxMin"].isNull()) g_disp.wxMin = constrain(j["wxMin"].as<int>(), 2, 60);
  if (!j["skyPreview"].isNull()) {   // Vorschau Tag/Daemmerung/Nacht: nicht gespeichert, endet nach 60 s
    g_skyPreview = constrain(j["skyPreview"].as<int>(), 0, 3);
    g_previewUntil2 = millis() + 60000UL;
    wx_sky_tick(true);
    g_needFull = true;
  }
  if (!j["brightness"].isNull()) g_disp.brightness = constrain(j["brightness"].as<int>(), 1, 100);
#if WX_HAS_LDR
  if (!j["autoLight"].isNull()) g_disp.autoLight = j["autoLight"].as<bool>();
#endif
  wx_display_save();
  if (!g_disp.autoLight) g_ldrCurPwm = wx_user_pwm();
  if (!g_portal) wx_apply_light();
  server.send(200, "text/plain", "Saved");
}

static void wx_handle_display_preview() {
  g_previewUntil = millis() + 4000UL;   // Auto-Licht pausiert kurz, damit die Vorschau sichtbar bleibt
  const int pct = constrain(server.arg("value").toInt(), 1, 100);
  if (g_lightOn) wx_bl_write(map(pct, 0, 100, 0, 255));
  server.send(200, "text/plain", "OK");
}

#if WX_WEB_PREVIEW
// GET /api/screen: aktuelles Displaybild als BMP (RGB565, so gross und gedreht wie das Display).
// Gerendert in Streifen a 40 Zeilen ueber WxCapture -> braucht nur W*40*2 Byte RAM.
static void wx_handle_screen() {
  if (g_shown == 255 || g_portalShown || g_otaBusy) { server.send(503, "text/plain", "No view yet"); return; }
  const int W = tft_hw.width(), H = tft_hw.height(), R = 40;
  uint16_t* buf = (uint16_t*)malloc(W * R * 2);
  if (!buf) { server.send(503, "text/plain", "Not enough memory"); return; }
  const uint32_t data = (uint32_t)W * H * 2, off = 14 + 40 + 12;
  uint8_t hd[66] = {0};
  auto put32 = [&](int o, uint32_t v) { hd[o] = v; hd[o + 1] = v >> 8; hd[o + 2] = v >> 16; hd[o + 3] = v >> 24; };
  hd[0] = 'B'; hd[1] = 'M'; put32(2, off + data); put32(10, off);
  put32(14, 40); put32(18, W); put32(22, (uint32_t)(-H));   // negative Hoehe = von oben nach unten
  hd[26] = 1; hd[28] = 16; put32(30, 3); put32(34, data);   // BI_BITFIELDS
  put32(54, 0xF800); put32(58, 0x07E0); put32(62, 0x001F);
  server.sendHeader("Cache-Control", "no-store");
  server.setContentLength(off + data);
  server.send(200, "image/bmp", "");
  server.sendContent((const char*)hd, sizeof(hd));
  WxCapture cap(W, H);
  cap.buf = buf;
  for (int y0 = 0; y0 < H; y0 += R) {
    cap.y0 = y0; cap.rows = min(R, H - y0);
    memset(buf, 0, W * R * 2);
    g_gfx = &cap;
    wx_render_view();
    g_gfx = &tft_hw;
    server.sendContent((const char*)buf, W * cap.rows * 2);
  }
  free(buf);
}
#endif
static void wx_handle_refresh() {
  static unsigned long last = 0;
  if (last && millis() - last < 60000UL) { server.send(429, "text/plain", "Please wait a minute"); return; }
  last = millis();
  g_forceFetch = true;
  server.send(200, "text/plain", "OK");
}

// Im Portal leiten unbekannte URLs (Captive-Portal-Pruefung der Handys) auf die Seite um
static void wx_handle_not_found() {
  if (g_portal && server.hostHeader() != WX_PORTAL_IP.toString()) {
    server.sendHeader("Location", "http://192.168.4.1/", true);
    server.send(302, "text/plain", "");
    return;
  }
  server.send(404, "text/plain", "Not found");
}

static void wx_web_begin() {
  if (g_serverStarted) return;
  server.on("/", wx_handle_root);
  server.on("/api/stats", HTTP_GET, wx_handle_stats);
  server.on("/api/config", HTTP_GET, wx_handle_config_get);
  server.on("/api/config", HTTP_POST, wx_handle_config_post);
  server.on("/api/scan", HTTP_GET, wx_handle_scan);
  server.on("/api/display", HTTP_GET, wx_handle_display_get);
  server.on("/api/display", HTTP_POST, wx_handle_display_post);
  server.on("/api/display/preview", HTTP_GET, wx_handle_display_preview);
  server.on("/api/refresh", HTTP_POST, wx_handle_refresh);
#if WX_WEB_PREVIEW
  server.on("/api/screen", HTTP_GET, wx_handle_screen);
#endif
  server.on("/restart", HTTP_POST, [] { server.send(200, "text/plain", "Restarting"); delay(150); wx_restart("web"); });
  server.onNotFound(wx_handle_not_found);
  server.begin();
  g_serverStarted = true;
}

// WLAN-Waechter: alle 10 s pruefen, bei Ausfall neu verbinden, nach 5 min ohne WLAN neu starten
static void wx_wifi_watchdog() {
  static unsigned long lastCheck = 0, downSince = 0;
  static bool wasUp = true;
  const unsigned long now = millis();
  if (now - lastCheck < 10000UL) return;
  lastCheck = now;
  const bool up = WiFi.status() == WL_CONNECTED;
  if (up) {
    if (!wasUp) { g_wifiReconnects++; Serial.println("[WiFi] reconnected: " + WiFi.localIP().toString()); }
    wasUp = true; downSince = 0;
    return;
  }
  if (wasUp) { g_wifiDrops++; downSince = now; wasUp = false; Serial.println("[WiFi] lost - reconnecting"); }
  if (!g_portal) WiFi.reconnect();
  if (now - downSince > 300000UL) wx_restart("Wi-Fi lost for 5 min");
}

static void wx_web_service() {
  if (!g_serverStarted) return;
  server.handleClient();
  if (g_portal) g_dns.processNextRequest();
}

// ================================================================ Touch + BOOT-Knopf
static uint8_t       g_taps = 0;
static unsigned long g_tapAt = 0;
static unsigned long g_capDownSince = 0;   // NM-TV: seit wann die Taste gehalten wird (0 = nicht)

static void wx_on_tap() {
  if (g_portal) { if (g_portalCanClose && WiFi.status() == WL_CONNECTED) wx_stop_portal(); return; }
  if (!g_lightOn) { wx_toggle_light(); g_taps = 0; return; }   // dunkel: jedes Tippen macht Licht an
  g_taps++;
  g_tapAt = millis();
}

// Einmal tippen = Licht aus, zweimal schnell = naechste Ansicht
static void wx_tap_tick() {
  if (!g_taps || millis() - g_tapAt < 400UL) return;
  if (g_taps >= 2) wx_next_view(); else wx_toggle_light();
  g_taps = 0;
}

#if WX_HAS_XPT
static SPIClass            wx_touch_spi(VSPI);
// Ohne IRQ-Pin: GPIO36 liefert bei aktivem WLAN/ADC Schein-Interrupts (ESP32-Errata) -> Abfrage alle 20 ms
static XPT2046_Touchscreen wx_touch(TOUCH_CS);
static void wx_touch_setup() {
  wx_touch_spi.begin(TOUCH_SCK, TOUCH_MISO, TOUCH_MOSI, TOUCH_CS);
  wx_touch.begin(wx_touch_spi);
}
static void wx_touch_tick() {
  static bool pressed = false;
  static unsigned long lastDown = 0, lastPoll = 0;
  const unsigned long now = millis();
  if (now - lastPoll < 20UL) return;
  lastPoll = now;
  if (wx_touch.touched()) {
    lastDown = now;
    if (!pressed) { pressed = true; wx_on_tap(); }
  } else if (pressed && now - lastDown > 80UL) {
    pressed = false;   // erst nach 80 ms ohne Beruehrung gilt der Finger als weg
  }
}
#elif WX_HAS_CAP_TOUCH
static void wx_touch_setup() {}
static void wx_touch_tick() {
  static unsigned long rawStart = 0;
  static bool wasDown = false;
  const bool raw = touchRead(TOUCH_CAP_PIN) < TOUCH_THRESH;
  const unsigned long now = millis();
  bool down = false;
  if (raw) { if (!rawStart) rawStart = now; down = now - rawStart >= TOUCH_CONFIRM_MS; }
  else rawStart = 0;
  if (down && !wasDown) g_capDownSince = now;
  if (!down && wasDown) {
    const unsigned long held = g_capDownSince ? now - g_capDownSince : 0;
    g_capDownSince = 0;
    if (held < 2000UL) wx_on_tap();
  }
  wasDown = down;
}
#else
static void wx_touch_setup() {}
static void wx_touch_tick() {}
#endif

// BOOT 5 s halten (NM-TV: Touch 10 s) -> Einrichtungs-Portal, mit Countdown auf dem Display
static void wx_hold_tick() {
  static bool init = false;
  static unsigned long bootSince = 0;
  static int lastShown = -1;
  if (!init) { if (WX_BOOT_PIN >= 0) pinMode(WX_BOOT_PIN, INPUT_PULLUP); init = true; }
  if (g_portal || g_otaBusy) return;
  const unsigned long now = millis();
  if (WX_BOOT_PIN >= 0 && digitalRead(WX_BOOT_PIN) == LOW) { if (!bootSince) bootSince = now; } else bootSince = 0;

  int remain = -1, showFrom = 1;   // Sekunden bis zum Portal, -1 = nichts gehalten
  if (bootSince) remain = 5 - (int)((now - bootSince) / 1000UL);
  if (g_capDownSince) {
    const int r = 10 - (int)((now - g_capDownSince) / 1000UL);
    if (remain < 0 || r < remain) { remain = r; showFrom = 3; }
  }
  if (remain < 0) {
    if (lastShown >= 0) { lastShown = -1; g_overlay = false; g_needFull = true; }
    return;
  }
  if (remain <= 0) {
    lastShown = -1; bootSince = 0; g_capDownSince = 0;
    wx_start_portal(true);
    return;
  }
  const int held = (showFrom == 3 ? 10 : 5) - remain;
  if (remain != lastShown && held >= showFrom) {
    lastShown = remain;
    if (!g_lightOn) { g_lightOn = true; wx_apply_light(); }
    g_overlay = true;
    wx_info_bar((wx_lang_de() ? "Setup in " : "Setup in ") + String(remain) + " s");
    Serial.printf("[Setup] held - portal in %d s\n", remain);
  }
}

// ================================================================ OTA
static void wx_ota_setup() {
  ArduinoOTA.setHostname(g_wx.hostname.c_str());
  ArduinoOTA.setPort(3232);
  if (strlen(OTA_PASSWORD)) ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([] {
    g_otaBusy = true;
    Serial.println("[OTA] start");
    wx_bl_write(255);
    wx_ota_screen();
  });
  ArduinoOTA.onProgress([](unsigned int p, unsigned int t) {
    static int last = -1;
    const int pct = t ? (int)((uint64_t)p * 100 / t) : 0;
    if (pct >= last + 5 || pct == 100 || pct < last) { last = pct; wx_ota_progress(pct); }
  });
  ArduinoOTA.onEnd([] { wx_ota_message("REBOOT...", ST77XX_GREEN); Serial.println("[OTA] done"); });
  ArduinoOTA.onError([](ota_error_t e) {
    Serial.printf("[OTA] error %u\n", e);
    wx_ota_message(e == OTA_AUTH_ERROR ? "AUTH ERROR" : "OTA ERROR", ST77XX_RED);
    delay(2000);
    ESP.restart();
  });
  ArduinoOTA.begin();
}

// ================================================================ Setup / Loop
static void wx_screen_setup() {
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, WX_BL_ACTIVE_LOW ? HIGH : LOW);   // waehrend der Initialisierung dunkel
#if defined(TFT_EN_PIN)
  pinMode(TFT_EN_PIN, OUTPUT);
  digitalWrite(TFT_EN_PIN, LOW);   // NM-TV: Display-Versorgung an
  delay(50);
#endif
  if (TFT_CS >= 0) { pinMode(TFT_CS, OUTPUT); digitalWrite(TFT_CS, HIGH); }
  wx_tft_spi.begin(TFT_SCK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft_hw.init(WX_TFT_W, WX_TFT_H, WX_TFT_SPI_MODE);
  tft_hw.setSPISpeed(20000000);
  tft.setRotation((g_disp.rotation / 90) & 3);
#if WX_TFT_INVERT_OFF
  tft.invertDisplay(false);
#endif
  tft.setTextWrap(false);
  tft.fillScreen(ST77XX_BLACK);
  wx_disp_offsets();
  wx_backlight_setup();
}

static void wx_console_id() {
  Serial.printf("[ESPressWeather32 v%s | %s] %s  %s  RSSI %d  heap %u  view %s\n", WX_VERSION, WX_BOARD,
                g_wx.deviceName.c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI(), ESP.getFreeHeap(),
                g_shown != 255 ? wx_view_name(g_shown) : "-");
}

void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.printf("\n\nESPressWeather32 v%s (%s)\n", WX_VERSION, WX_BOARD);
  g_mtx = xSemaphoreCreateMutex();

  wx_settings_load();
  wx_display_load();
  wx_cache_load();
  Serial.println(wx_settings_configured() ? "Settings: " + g_wx.deviceName + " / " + g_wx.wifiSsid
                                          : String("Settings: none stored - starting setup portal"));

  wx_screen_setup();
  wx_touch_setup();
  wx_ldr_setup();   // LDR erst nach Display- und Touch-SPI
  wx_boot_screen();
  wx_boot_log("[ OK ]", "Display " WX_BOARD, ST77XX_GREEN);

  if (!wx_settings_configured()) {
    wx_start_portal(false);
    wx_web_begin();
    for (;;) { wx_web_service(); delay(5); }   // nach dem Speichern startet das Geraet neu
  }

  WiFi.mode(WIFI_STA);
  WiFi.setSleep(false);
  WiFi.setAutoReconnect(true);

  WiFi.setHostname(g_wx.hostname.c_str());
  wx_boot_log("[INFO]", "WLAN " + g_wx.wifiSsid, ST77XX_CYAN);
  WiFi.begin(g_wx.wifiSsid.c_str(), g_wx.wifiPass.c_str());
  const unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED) {
    // Nach 30 s ohne WLAN zusaetzlich das Portal oeffnen (WLAN-Versuch laeuft weiter)
    if (millis() - t0 > 30000UL && !g_portal) { wx_start_portal(true); wx_web_begin(); }
    wx_web_service();
    delay(20);
  }
  if (g_portal) wx_stop_portal();
  wx_boot_log("[ OK ]", "IP " + WiFi.localIP().toString(), ST77XX_GREEN);

  configTzTime(g_wx.tz.c_str(), "pool.ntp.org", "time.nist.gov");
  for (int i = 0; i < 40 && !wx_time_valid(); i++) delay(100);
  if (wx_time_valid()) {
    time_t now = time(nullptr); struct tm t; localtime_r(&now, &t);
    char b[24]; strftime(b, sizeof(b), "Time %H:%M:%S", &t);
    wx_boot_log("[ OK ]", b, ST77XX_GREEN);
  } else wx_boot_log("[WARN]", "NTP later", ST77XX_YELLOW);

  wx_ota_setup();
  wx_boot_log("[ OK ]", "OTA " + g_wx.hostname, ST77XX_GREEN);
  if (MDNS.begin(g_wx.hostname.c_str())) MDNS.addService("http", "tcp", 80);
  wx_web_begin();
  wx_boot_log("[ OK ]", "Web " + g_wx.hostname + ".local", ST77XX_GREEN);
  if (g_wx.mqttOn) { wx_mqtt_begin(); wx_boot_log("[ OK ]", "MQTT :" + String(g_wx.mqttPort), ST77XX_GREEN); }
  if (g_wx.owKey.isEmpty() || g_wx.owCity.isEmpty()) wx_boot_log("[WARN]", "No API key/city", ST77XX_YELLOW);
  else wx_boot_log("[INFO]", "Weather " + g_wx.owCity, ST77XX_CYAN);

#if CONFIG_FREERTOS_UNICORE
  xTaskCreate(wx_net_task, "wx_net", 8192, nullptr, 1, nullptr);
#else
  xTaskCreatePinnedToCore(wx_net_task, "wx_net", 8192, nullptr, 1, nullptr, 0);
#endif
  Serial.printf("Dashboard: http://%s.local (or http://%s)\n", g_wx.hostname.c_str(), WiFi.localIP().toString().c_str());
  delay(5000);   // Startbild (Tux, IP) gut lesbar stehen lassen
  wx_apply_light();   // Boot war mit voller Helligkeit, jetzt die eingestellte
  g_cycleAt = millis();
  wx_sky_tick(true);
  g_needFull = true;
  wx_console_id();
}

void loop() {
  ArduinoOTA.handle();
  wx_web_service();
  wx_wifi_watchdog();
  wx_mqtt_loop();
  wx_touch_tick();
  wx_tap_tick();
  wx_hold_tick();
  wx_ldr_tick();
  if (g_portal && !g_portalShown) { wx_portal_screen(g_apName, WX_PORTAL_PASSWORD, g_portalCanClose); g_portalShown = true; }
  wx_view_tick();

  static unsigned long lastId = 0;
  if (millis() - lastId >= 60000UL) { lastId = millis(); wx_console_id(); }
  delay(2);
}
