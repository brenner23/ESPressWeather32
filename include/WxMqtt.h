#pragma once
// ESPressWeather32: Energie-Ansicht. Dieses Geraet ist selbst der MQTT-Broker
// (PicoMQTT::Server). In der Tasmota-Steckdose: MQTT -> Host = IP dieses Displays.
#include <WiFi.h>
#include <PicoMQTT.h>
#include <ArduinoJson.h>
#include "WxData.h"
#include "WxSettings.h"

static WxEnergy      g_tas = { false, false, "", "", 0, 0, 0, 0, 0, 0, 0 };
static volatile bool g_tasDirty = false;

static const char* wx_energy_name() {
  if (g_wx.mqttName.length()) return g_wx.mqttName.c_str();
  if (g_tas.name[0])          return g_tas.name;
  return "Wait...";
}

// Geraetename aus <prefix>/<device>/<suffix>
static void wx_mqtt_device(const char* topic) {
  const char* p1 = strchr(topic, '/');  if (!p1) return;
  const char* p2 = strchr(p1 + 1, '/'); if (!p2) return;
  size_t n = (size_t)(p2 - (p1 + 1));
  if (n >= sizeof(g_tas.name)) n = sizeof(g_tas.name) - 1;
  strncpy(g_tas.name, p1 + 1, n);
  g_tas.name[n] = 0;
}

static void wx_mqtt_on_json(const char* topic, const char* payload) {
  g_tas.lastMsgMs = millis();
  wx_mqtt_device(topic);
  JsonDocument doc;
  if (deserializeJson(doc, payload)) return;
  const char* pw = doc["POWER"];
  if (pw) { strncpy(g_tas.powerState, pw, sizeof(g_tas.powerState) - 1); g_tas.powerState[sizeof(g_tas.powerState) - 1] = 0; }
  JsonVariantConst e = doc["ENERGY"];
  if (e.isNull()) e = doc["StatusSNS"]["ENERGY"];
  if (!e.isNull()) {
    g_tas.power     = e["Power"]     | g_tas.power;
    g_tas.today     = e["Today"]     | g_tas.today;
    g_tas.yesterday = e["Yesterday"] | g_tas.yesterday;
    g_tas.total     = e["Total"]     | g_tas.total;
    g_tas.voltage   = e["Voltage"]   | g_tas.voltage;
    g_tas.current   = e["Current"]   | g_tas.current;
    g_tas.valid     = true;
  }
  g_tas.online = true;
  g_tasDirty = true;
}

static void wx_mqtt_on_power(const char* topic, const char* payload) {
  g_tas.lastMsgMs = millis();
  wx_mqtt_device(topic);
  const bool on = !strcasecmp(payload, "ON");
  strcpy(g_tas.powerState, on ? "ON" : "OFF");
  g_tasDirty = true;
}

static void wx_mqtt_on_lwt(const char* topic, const char* payload) {
  g_tas.online = !strcmp(payload, "Online");
  g_tasDirty = true;
}

class WxMqttServer : public PicoMQTT::Server {
public:
  explicit WxMqttServer(uint16_t port) : PicoMQTT::Server(port) {}
protected:
  PicoMQTT::ConnectReturnCode auth(const char* client_id, const char* username, const char* password) override {
    if (g_wx.mqttUser.isEmpty()) return PicoMQTT::CRC_ACCEPTED;   // ohne Anmeldung
    if (username && password && g_wx.mqttUser == username && g_wx.mqttPass == password)
      return PicoMQTT::CRC_ACCEPTED;
    Serial.println("[MQTT] login rejected (user/password)");
    return PicoMQTT::CRC_BAD_USERNAME_OR_PASSWORD;
  }
  void on_connected(const char* client_id) override {
    Serial.printf("[MQTT] client connected: %s\n", client_id ? client_id : "");
  }
};

static WxMqttServer* g_mqtt = nullptr;

static void wx_mqtt_begin() {
  if (g_mqtt || !g_wx.mqttOn || WiFi.status() != WL_CONNECTED) return;
  g_mqtt = new WxMqttServer(g_wx.mqttPort ? g_wx.mqttPort : 1883);
  g_mqtt->subscribe("tele/+/SENSOR", wx_mqtt_on_json);
  g_mqtt->subscribe("tele/+/STATE",  wx_mqtt_on_json);
  g_mqtt->subscribe("stat/+/RESULT", wx_mqtt_on_json);
  g_mqtt->subscribe("stat/+/POWER",  wx_mqtt_on_power);
  g_mqtt->subscribe("tele/+/LWT",    wx_mqtt_on_lwt);
  g_mqtt->begin();
  Serial.printf("[MQTT] broker on port %u - Tasmota host = %s\n", g_wx.mqttPort, WiFi.localIP().toString().c_str());
}

static void wx_mqtt_loop() {
  if (!g_wx.mqttOn) return;
  if (!g_mqtt) { wx_mqtt_begin(); return; }
  g_mqtt->loop();
}
