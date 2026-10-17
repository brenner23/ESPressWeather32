#pragma once
// ESPressWeather32: alle Bildschirme. Layout ist 240x240; auf dem CYD (240x320)
// wird es mittig gesetzt (Versatz wx_ox / wx_oy, je nach Drehung 40 px).
// Nur loop() zeichnet (ein Task am Display).
#include <Adafruit_GFX.h>
#include <Adafruit_ST7789.h>
#include "Board.h"
#include "WxData.h"
#include "WxSettings.h"
#include "WxIcons.h"
#include "OCRA10pt7b.h"
#include "OCRA9pt7b.h"
#include "ArchivoNB15pt7b.h"
#include "ArchivoNB50pt7b.h"
#include "tux_icon.h"

#include "WxGfx.h"

#define C_BG      ST77XX_BLACK
#define C_ACCENT  0x055F   // Blau (0,170,255)
#define C_MUTED   0x8C71
#define C_LINE    0x528A
#define C_PANEL   0x18E3
#define C_MINUTE  0xE547   // Minuten-Orange (229,169,60)

static int wx_ox = 0, wx_oy = 0;   // Versatz des 240er-Layouts

static void wx_disp_offsets() {
  wx_ox = (tft.width()  - 240) / 2;
  wx_oy = (tft.height() - 240) / 2;
}

// ---------- Text-Helfer ----------
static const char* const WD_LONG_DE[7]  = {"SONNTAG","MONTAG","DIENSTAG","MITTWOCH","DONNERSTAG","FREITAG","SAMSTAG"};
static const char* const WD_LONG_EN[7]  = {"SUNDAY","MONDAY","TUESDAY","WEDNESDAY","THURSDAY","FRIDAY","SATURDAY"};
static const char* const WD_SHORT_DE[7] = {"So","Mo","Di","Mi","Do","Fr","Sa"};
static const char* const WD_SHORT_EN[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

static const char* wx_wd_short(int8_t d) {
  if (d < 0 || d > 6) return "---";
  return wx_lang_de() ? WD_SHORT_DE[d] : WD_SHORT_EN[d];
}

static bool wx_imperial() { return g_wx.owUnits == "imperial"; }

static void wx_gfx_center(const String& s, int cx, int baseY, const GFXfont* f, uint16_t c) {
  int16_t x1, y1; uint16_t w, h;
  tft.setFont(f); tft.setTextSize(1);
  tft.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  tft.setTextColor(c);
  tft.setCursor(cx - (int)w / 2 - x1, baseY);
  tft.print(s);
}

static void wx_txt_center(const String& s, int cx, int y, uint8_t size, uint16_t fg, uint16_t bg) {
  tft.setFont(NULL); tft.setTextSize(size);
  tft.setTextColor(fg, bg);
  tft.setCursor(cx - (int)(s.length() * 6 * size) / 2, y);
  tft.print(s);
}

// ---------- Startbildschirm + Boot-Log ----------
static int wx_boot_y = 0;

static void wx_boot_screen() {
  wx_disp_offsets();
  tft.fillScreen(C_BG);
  const int X = wx_ox, Y = wx_oy;
  tft.fillRoundRect(X + 8, Y + 6, 224, 46, 8, C_ACCENT);
  wx_gfx_center("ESPressWeather32", X + 120, Y + 31, &ArchivoNB15pt7b, ST77XX_BLACK);
  wx_txt_center(String(WX_BOARD) + "  |  v" WX_VERSION, X + 120, Y + 39, 1, ST77XX_BLACK, C_ACCENT);
  tft.drawRGBBitmap(X + 170, Y + 170, (uint16_t*)tux_icon, 64, 64);   // Tux (Larry Ewing / The GIMP)
  wx_boot_y = Y + 62;
}

static void wx_boot_log(const char* tag, const String& msg, uint16_t tagColor) {
  Serial.printf("%s %s\n", tag, msg.c_str());
  if (wx_boot_y > wx_oy + 228) return;
  tft.setFont(NULL); tft.setTextSize(1);
  char t[16];
  snprintf(t, sizeof(t), "[%3lu.%03lu] ", millis() / 1000, millis() % 1000);
  tft.setCursor(wx_ox + 4, wx_boot_y);
  tft.setTextColor(C_MUTED, C_BG);   tft.print(t);
  tft.setTextColor(tagColor, C_BG);  tft.print(tag); tft.print(" ");
  tft.setTextColor(ST77XX_WHITE, C_BG);
  tft.print(msg.substring(0, wx_boot_y + 8 >= wx_oy + 170 ? 9 : 22));   // unten rechts sitzt Tux
  wx_boot_y += 12;
}

// ---------- Einrichtungs-Portal ----------
static void wx_portal_screen(const String& ap, const char* pass, bool canClose) {
  wx_disp_offsets();
  tft.fillScreen(C_BG);
  const int X = wx_ox, Y = wx_oy;
  const bool de = wx_lang_de();
  tft.setFont(NULL);
  tft.setTextColor(ST77XX_CYAN, C_BG);
  tft.setTextSize(3);
  tft.setCursor(X + 10, Y + 16); tft.print("SETUP");
  tft.setTextSize(1);
  tft.setTextColor(C_MUTED, C_BG);
  tft.setCursor(X + 10, Y + 46); tft.print("ESPressWeather32  v" WX_VERSION);

  tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE, C_BG);  tft.setCursor(X + 10, Y + 72);  tft.print(de ? "WLAN:" : "Wi-Fi:");
  tft.setTextColor(ST77XX_GREEN, C_BG);  tft.setCursor(X + 10, Y + 92);  tft.print(ap);
  tft.setTextColor(ST77XX_WHITE, C_BG);  tft.setCursor(X + 10, Y + 118); tft.print(de ? "Passwort:" : "Password:");
  tft.setTextColor(ST77XX_GREEN, C_BG);  tft.setCursor(X + 10, Y + 138); tft.print(pass);
  tft.setTextColor(ST77XX_WHITE, C_BG);  tft.setCursor(X + 10, Y + 164); tft.print("192.168.4.1");

  tft.setTextSize(1);
  tft.setTextColor(C_MUTED, C_BG);
  tft.setCursor(X + 10, Y + 194);
  tft.print(de ? "Handy mit dem WLAN verbinden," : "Join this Wi-Fi with your phone,");
  tft.setCursor(X + 10, Y + 206);
  tft.print(de ? "die Seite oeffnet sich selbst." : "the page opens by itself.");
  if (canClose) {
    tft.setCursor(X + 10, Y + 222);
    tft.setTextColor(C_ACCENT, C_BG);
#if WX_HAS_TOUCH
    tft.print(de ? "Antippen = Setup beenden" : "Tap = close setup");
#else
    tft.print(de ? "Neustart = Setup beenden" : "Restart = close setup");
#endif
  }
}

// Hinweisleiste unten (Countdown usw.). Weg damit per Neuzeichnen der Ansicht.
static void wx_info_bar(const String& msg) {
  const int X = wx_ox, Y = wx_oy;
  tft.fillRoundRect(X + 8, Y + 196, 224, 38, 7, C_PANEL);
  tft.drawRoundRect(X + 8, Y + 196, 224, 38, 7, C_ACCENT);
  wx_txt_center(msg.substring(0, 18), X + 120, Y + 208, 2, ST77XX_WHITE, C_PANEL);
}

// ---------- Ansicht 2: 3-Tage-Vorhersage ----------
static void wx_forecast_screen(const WxForecast& f) {
  wx_disp_offsets();
  wx_clear(0, 0, tft.width(), tft.height());
  const int X = wx_ox, Y = wx_oy;
  const bool de = wx_lang_de(), imp = wx_imperial();
  tft.setFont(NULL);
  tft.setTextColor(ST77XX_CYAN);
  tft.setTextSize(2);
  tft.setCursor(X + 15, Y + 12);
  tft.print(de ? "3-TAGE TREND" : "3-DAY TREND");
  tft.drawFastHLine(X + 10, Y + 34, 220, C_LINE);
  for (int i = 0; i < 3; i++) {
    const WxDay& d = f.days[i];
    const int y = Y + 44 + i * 64;
    tft.drawRoundRect(X + 8, y, 224, 58, 4, 0x2125);
    tft.setTextSize(2);
    tft.setTextColor(ST77XX_GREEN);
    tft.setCursor(X + 18, y + 20);
    tft.print(f.valid ? wx_wd_short(d.wday) : "---");
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(X + 65, y + 12);
    if (f.valid) tft.printf("%.0f/%.0f\xF7%c", d.tempMax, d.tempMin, imp ? 'F' : 'C');
    else tft.print("--/--");
    tft.setTextSize(1);
    tft.setTextColor(0x05BF);
    tft.setCursor(X + 65, y + 36);
    tft.printf(de ? "Regen: %d%%" : "Rain: %d%%", f.valid ? d.pop : 0);
    wx_draw_icon(f.valid ? d.conditionId : 803, false, X + 184, y + 13, 32);
  }
}

// ---------- Ansicht 3: Energie (Tasmota per MQTT) ----------
static int wx_energy_last_min = -99;

// Nur den Zahlenbereich neu zeichnen, und nur wenn sich die Zahl geaendert hat (full = alles)
static char wx_energy_last[4][24];
static void wx_energy_row(int i, int y, const char* label, const char* value, bool full) {
  if (!full && !strcmp(wx_energy_last[i], value)) return;
  strlcpy(wx_energy_last[i], value, sizeof(wx_energy_last[i]));
  const int X = wx_ox, Y = wx_oy;
  tft.setFont(&OCRA10pt7b); tft.setTextSize(1);
  wx_clear(X + 90, Y + y - 15, 148, 20);
  tft.setTextColor(0x05BF);  tft.setCursor(X + 12, Y + y); tft.print(label);
  tft.setTextColor(ST77XX_WHITE); tft.setCursor(X + 95, Y + y); tft.print(value);
}

static void wx_energy_clock(bool force) {
  time_t now = time(nullptr);
  const bool valid = now > 1600000000;
  struct tm* t = localtime(&now);
  const int curMin = valid ? t->tm_min : -1;
  if (!force && curMin == wx_energy_last_min) return;
  wx_energy_last_min = curMin;
  const int X = wx_ox, Y = wx_oy;
  wx_clear(X, Y + 129, 240, 111);
  char clk[8] = "--:--", line[24] = "";
  if (valid) {
    strftime(clk, sizeof(clk), "%H:%M", t);
    char d[16];
    strftime(d, sizeof(d), wx_lang_de() ? "%d.%m.%Y" : "%Y-%m-%d", t);
    snprintf(line, sizeof(line), "%s %s", wx_wd_short(t->tm_wday), d);
  }
  wx_gfx_center(clk, X + 120, Y + 210, &ArchivoNB50pt7b, ST77XX_WHITE);
  if (line[0]) wx_gfx_center(line, X + 120, Y + 232, &OCRA9pt7b, ST77XX_GREEN);
}

static void wx_energy_screen(const WxEnergy& e, const char* name, bool full) {
  const int X = wx_ox, Y = wx_oy;
  const bool de = wx_lang_de();
  if (full) { wx_disp_offsets(); wx_clear(0, 0, tft.width(), tft.height()); }
  char nm[17];
  strncpy(nm, name, sizeof(nm)); nm[sizeof(nm) - 1] = 0;
  wx_clear(X, Y + 6, 174, 26);
  tft.setFont(&OCRA10pt7b); tft.setTextSize(1);
  tft.setTextColor(ST77XX_CYAN);
  tft.setCursor(X + 10, Y + 24); tft.print(nm);

  const bool on = strcmp(e.powerState, "ON") == 0;
  tft.fillRoundRect(X + 178, Y + 6, 54, 22, 4, on ? 0x0647 : 0x4208);
  tft.setFont(NULL); tft.setTextSize(2);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(X + (on ? 193 : 187), Y + 10);
  tft.print(e.powerState[0] ? e.powerState : "---");
  tft.drawFastHLine(X + 10, Y + 34, 220, C_LINE);

  char v[24];
  snprintf(v, sizeof(v), "%6.0f  W ", e.power);      wx_energy_row(0, 50,  de ? "Jetzt" : "Power", v, full);
  snprintf(v, sizeof(v), "%6.3f kWh", e.today);      wx_energy_row(1, 72,  de ? "Heute" : "Today", v, full);
  snprintf(v, sizeof(v), "%6.3f kWh", e.yesterday);  wx_energy_row(2, 94,  de ? "Gest." : "Yest.", v, full);
  snprintf(v, sizeof(v), e.total >= 10000.0f ? "%6.0f kWh" : "%6.1f kWh", e.total);
  wx_energy_row(3, 116, "Total", v, full);
  tft.drawFastHLine(X + 10, Y + 128, 220, C_LINE);
  wx_energy_clock(full);
}

// ---------- OTA-Update ----------
static void wx_ota_screen() {
  wx_disp_offsets();
  tft.fillScreen(C_BG);
  const int X = wx_ox, Y = wx_oy;
  wx_txt_center("OTA", X + 120, Y + 70, 3, ST77XX_GREEN, C_BG);
  wx_txt_center("UPDATE", X + 120, Y + 110, 3, ST77XX_GREEN, C_BG);
  tft.drawRect(X + 28, Y + 160, 184, 18, ST77XX_WHITE);
}

static void wx_ota_progress(int percent) {
  const int X = wx_ox, Y = wx_oy;
  tft.fillRect(X + 30, Y + 162, (percent * 180) / 100, 14, ST77XX_GREEN);
  tft.fillRect(X + 75, Y + 195, 90, 20, C_BG);
  wx_txt_center(String(percent) + "%", X + 120, Y + 195, 2, ST77XX_WHITE, C_BG);
}

static void wx_ota_message(const String& msg, uint16_t color) {
  const int X = wx_ox, Y = wx_oy;
  tft.fillRect(X, Y + 192, 240, 26, C_BG);
  wx_txt_center(msg, X + 120, Y + 195, 2, color, C_BG);
}
