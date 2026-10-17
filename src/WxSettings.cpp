#include "WxSettings.h"
#include "Board.h"
#include <Preferences.h>

// Zugangsdaten wahlweise aus secrets.ini (use_secrets = 1, siehe platformio.ini)
#ifndef WX_USE_SECRETS
  #define WX_USE_SECRETS 0
#endif
#ifndef WX_S_SSID
  #define WX_S_NAME    "ESPressWeather32"
  #define WX_S_HOST    "espressweather"
  #define WX_S_SSID    ""
  #define WX_S_WPASS   ""
  #define WX_S_OWKEY   ""
  #define WX_S_OWCITY  ""
  #define WX_S_OWCTRY  "DE"
  #define WX_S_OWLANG  "de"
  #define WX_S_OWUNITS "metric"
  #define WX_S_TZ      "CET-1CEST,M3.5.0,M10.5.0/3"
  #define WX_S_MQPORT  "1883"
  #define WX_S_MQUSER  ""
  #define WX_S_MQPASS  ""
#endif

#define WX_DEFAULT_TZ "CET-1CEST,M3.5.0,M10.5.0/3"

WxSettings     g_wx;
WxDisplayPrefs g_disp;

static const char* NVS_NS       = "espressweather";
static bool        s_configured = false;

static String or_default(const char* v, const char* def) { return strlen(v) ? String(v) : String(def); }

bool wx_settings_load()
{
    Preferences p;
    p.begin(NVS_NS, true);
    s_configured     = p.getBool("cfg", false);
    g_wx.deviceName  = p.getString("name", "ESPressWeather32");
    g_wx.hostname    = p.getString("host", "espressweather");
    g_wx.wifiSsid    = p.getString("ssid", "");
    g_wx.wifiPass    = p.getString("wpass", "");
    g_wx.owKey       = p.getString("owkey", "");
    g_wx.owCity      = p.getString("owcity", "");
    g_wx.owCountry   = p.getString("owctry", "DE");
    g_wx.owLang      = p.getString("owlang", "de");
    g_wx.owUnits     = p.getString("owunits", "metric");
    g_wx.tz          = p.getString("tz", WX_DEFAULT_TZ);
    g_wx.mqttOn      = p.getBool("mqon", true);
    g_wx.mqttPort    = p.getUShort("mqport", 1883);
    g_wx.mqttUser    = p.getString("mquser", "");
    g_wx.mqttPass    = p.getString("mqpass", "");
    g_wx.mqttName    = p.getString("mqname", "");
    p.end();
#if WX_USE_SECRETS
    // secrets.ini hat Vorrang: kein Portal, Werte bei jedem Start aus der Firmware
    if (strlen(WX_S_SSID)) {
        g_wx.deviceName = or_default(WX_S_NAME, "ESPressWeather32");
        g_wx.hostname   = or_default(WX_S_HOST, "espressweather");
        g_wx.wifiSsid   = WX_S_SSID;
        g_wx.wifiPass   = WX_S_WPASS;
        g_wx.owKey      = WX_S_OWKEY;
        g_wx.owCity     = WX_S_OWCITY;
        g_wx.owCountry  = or_default(WX_S_OWCTRY, "DE");
        g_wx.owLang     = or_default(WX_S_OWLANG, "de");
        g_wx.owUnits    = or_default(WX_S_OWUNITS, "metric");
        g_wx.tz         = or_default(WX_S_TZ, WX_DEFAULT_TZ);
        g_wx.mqttPort   = atoi(WX_S_MQPORT) > 0 ? atoi(WX_S_MQPORT) : 1883;
        g_wx.mqttUser   = WX_S_MQUSER;
        g_wx.mqttPass   = WX_S_MQPASS;
        s_configured = true;
    }
#endif
    return s_configured;
}

bool wx_settings_from_secrets()
{
#if WX_USE_SECRETS
    return strlen(WX_S_SSID) > 0;
#else
    return false;
#endif
}

bool wx_settings_configured() { return s_configured; }

bool wx_settings_save(const WxSettings& s)
{
    Preferences p;
    if (!p.begin(NVS_NS, false)) return false;
    p.putString("name", s.deviceName);
    p.putString("host", s.hostname);
    p.putString("ssid", s.wifiSsid);
    p.putString("wpass", s.wifiPass);
    p.putString("owkey", s.owKey);
    p.putString("owcity", s.owCity);
    p.putString("owctry", s.owCountry);
    p.putString("owlang", s.owLang);
    p.putString("owunits", s.owUnits);
    p.putString("tz", s.tz);
    p.putBool("mqon", s.mqttOn);
    p.putUShort("mqport", s.mqttPort);
    p.putString("mquser", s.mqttUser);
    p.putString("mqpass", s.mqttPass);
    p.putString("mqname", s.mqttName);
    p.putBool("cfg", true);
    p.end();
    g_wx = s;
    s_configured = true;
    return true;
}

static void assign_trimmed(String& dst, JsonVariantConst v)
{
    if (v.isNull()) return;
    dst = v.as<const char*>() ? v.as<const char*>() : "";
    dst.trim();
}

// Leere Passwort-/Key-Felder = bisherigen Wert behalten (Feld "...Keep")
void wx_settings_apply_json(JsonVariantConst j, WxSettings& s)
{
    assign_trimmed(s.deviceName, j["deviceName"]);
    assign_trimmed(s.hostname, j["hostname"]);
    assign_trimmed(s.wifiSsid, j["wifiSsid"]);
    const char* wpass = j["wifiPass"] | "";
    if (wpass[0]) s.wifiPass = wpass;
    if (!(j["owKeyKeep"] | false)) assign_trimmed(s.owKey, j["owKey"]);
    assign_trimmed(s.owCity, j["owCity"]);
    assign_trimmed(s.owCountry, j["owCountry"]);
    assign_trimmed(s.owLang, j["owLang"]);
    assign_trimmed(s.owUnits, j["owUnits"]);
    assign_trimmed(s.tz, j["tz"]);
    if (s.tz.isEmpty()) s.tz = WX_DEFAULT_TZ;
    if (!j["mqttOn"].isNull()) s.mqttOn = j["mqttOn"].as<bool>();
    if (!j["mqttPort"].isNull()) s.mqttPort = j["mqttPort"].as<uint16_t>();
    assign_trimmed(s.mqttUser, j["mqttUser"]);
    if (!(j["mqttPassKeep"] | false)) assign_trimmed(s.mqttPass, j["mqttPass"]);
    if (s.mqttUser.isEmpty()) s.mqttPass = "";
    assign_trimmed(s.mqttName, j["mqttName"]);
}

static bool valid_host(const String& h)
{
    if (h.isEmpty() || h.length() > 31) return false;
    for (char c : h) if (!isalnum((unsigned char)c) && c != '-') return false;
    return true;
}

const char* wx_settings_validate(const WxSettings& s)
{
    if (s.deviceName.isEmpty())             return "Device name missing";
    if (!valid_host(s.hostname))            return "Hostname: only letters, digits and '-'";
    if (s.wifiSsid.isEmpty())               return "Wi-Fi name (SSID) missing";
    if (s.owUnits != "metric" && s.owUnits != "imperial") return "Units: metric or imperial";
    if (s.mqttPort == 0)                    return "MQTT port: 1 to 65535";
    return nullptr;
}

void wx_settings_to_json(JsonObject o)
{
    o["deviceName"]  = g_wx.deviceName;
    o["hostname"]    = g_wx.hostname;
    o["wifiSsid"]    = g_wx.wifiSsid;
    o["wifiPassSet"] = g_wx.wifiPass.length() > 0;
    o["owKeySet"]    = g_wx.owKey.length() > 0;
    o["owCity"]      = g_wx.owCity;
    o["owCountry"]   = g_wx.owCountry;
    o["owLang"]      = g_wx.owLang;
    o["owUnits"]     = g_wx.owUnits;
    o["tz"]          = g_wx.tz;
    o["mqttOn"]      = g_wx.mqttOn;
    o["mqttPort"]    = g_wx.mqttPort;
    o["mqttUser"]    = g_wx.mqttUser;
    o["mqttPassSet"] = g_wx.mqttPass.length() > 0;
    o["mqttName"]    = g_wx.mqttName;
    o["configured"]  = s_configured;
    o["fromSecrets"] = wx_settings_from_secrets();
}

void wx_display_load()
{
    Preferences p;
    p.begin("wx_display", true);
    g_disp.brightness = p.getUChar("bright", 100);
    g_disp.autoLight  = p.getBool("auto_ldr", false);
    g_disp.rotation   = p.getUShort("rotation", 90);
    g_disp.view       = p.getUChar("view2", 4);
    g_disp.cycleSec   = p.getUShort("cycle", 15);
    g_disp.wxMin      = p.getUChar("wxmin", 5);
    p.end();
    g_disp.wxMin = constrain(g_disp.wxMin, 2, 60);
    if (g_disp.brightness < 1 || g_disp.brightness > 100) g_disp.brightness = 100;
    if (g_disp.rotation != 0 && g_disp.rotation != 90 && g_disp.rotation != 180 && g_disp.rotation != 270) g_disp.rotation = 90;
    if (g_disp.view > 5) g_disp.view = 4;
    g_disp.cycleSec = constrain(g_disp.cycleSec, 5, 3600);
#if !WX_HAS_LDR
    g_disp.autoLight = false;
#endif
#if !WX_HAS_ROTATION
    g_disp.rotation = WX_FIXED_ROTATION * 90;
#endif
}

void wx_display_save()
{
    Preferences p;
    p.begin("wx_display", false);
    p.putUChar("bright", g_disp.brightness);
    p.putBool("auto_ldr", g_disp.autoLight);
    p.putUShort("rotation", g_disp.rotation);
    p.putUChar("view2", g_disp.view);
    p.putUShort("cycle", g_disp.cycleSec);
    p.putUChar("wxmin", g_disp.wxMin);
    p.end();
}

bool wx_lang_de() { return g_wx.owLang.startsWith("de"); }
