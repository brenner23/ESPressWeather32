#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>

// ESPressWeather32: Laufzeit-Einstellungen im NVS (Namespace "espressweather").
// Solange nichts gespeichert ist, startet das Einrichtungs-Portal. Im Quellcode
// stehen keine Zugangsdaten.

struct WxSettings {
    String   deviceName;   // Anzeigename (Dashboard-Titel)
    String   hostname;     // Netzwerkname (mDNS, OTA)
    String   wifiSsid;
    String   wifiPass;
    String   owKey;        // OpenWeather API-Key (wird nie an den Browser geschickt)
    String   owCity;
    String   owCountry;    // z. B. DE
    String   owLang;       // de / en (auch Sprache der Anzeige)
    String   owUnits;      // metric / imperial
    String   tz;           // Zeitzone (POSIX), z. B. CET-1CEST,M3.5.0,M10.5.0/3
    bool     mqttOn;       // Energie-Ansicht: Tasmota schickt per MQTT an dieses Geraet (Broker)
    uint16_t mqttPort;
    String   mqttUser;     // leer = Broker ohne Anmeldung
    String   mqttPass;
    String   mqttName;     // Anzeigename der Steckdose (leer = Name aus dem Topic)
};

// Anzeige-Einstellungen: aendern sich live (ohne Neustart), eigener Namespace "wx_display"
struct WxDisplayPrefs {
    uint8_t  brightness;   // 1..100 %
    bool     autoLight;    // nur CYD (LDR)
    uint16_t rotation;     // 0/90/180/270, nur CYD
    uint8_t  view;         // 0 = Wetter, 1 = 3 Tage, 2 = Energie, 3 = Wetter + Mond, 4 = automatisch wechseln
    uint16_t cycleSec;     // Wechselzeit fuer view = 3
    uint8_t  wxMin;        // aktuelles Wetter alle N Minuten holen (2..60, Standard 5)
};

extern WxSettings     g_wx;
extern WxDisplayPrefs g_disp;

bool        wx_settings_load();            // true = Konfiguration vorhanden
bool        wx_settings_configured();
bool        wx_settings_from_secrets();    // true = Werte kommen aus secrets.ini (use_secrets = 1)
bool        wx_settings_save(const WxSettings& s);
void        wx_settings_apply_json(JsonVariantConst j, WxSettings& s);
const char* wx_settings_validate(const WxSettings& s);
void        wx_settings_to_json(JsonObject o);   // ohne Passwoerter und ohne API-Key

void        wx_display_load();
void        wx_display_save();
bool        wx_lang_de();                   // Anzeige auf Deutsch?
