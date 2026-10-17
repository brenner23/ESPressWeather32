#pragma once
// ESPressWeather32: Wetter 1 (Uhr + Wetter) und Wetter 2 (mit Mond, Layout nach dem
// Wetterbild auf .99). Beide im 240x240-Feld oben links; auf dem CYD kommt die freie
// Flaeche dazu: Querformat rechts eine 80-px-Spalte, Hochformat unten ein 80-px-Streifen.
// Alle Flaechen werden mit wx_clear() geleert (Himmel-Hintergrund oder schwarz),
// Texte sind transparent.
#include "WxDisplay.h"
#include "WxSky.h"
#include "ArchivoNB30pt7b.h"

static const char* const WD_MIX_DE[7] = {"Sonntag","Montag","Dienstag","Mittwoch","Donnerstag","Freitag","Samstag"};
static const char* const WD_MIX_EN[7] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};

static bool wx_has_panel() { return tft.width() > 240 || tft.height() > 240; }

static void wx_txt(const String& s, int x, int y, uint8_t size, uint16_t c) {
  tft.setFont(NULL); tft.setTextSize(size); tft.setTextColor(c);
  tft.setCursor(x, y); tft.print(s);
}
static void wx_txt_c(const String& s, int cx, int y, uint8_t size, uint16_t c) {
  wx_txt(s, cx - (int)(s.length() * 6 * size) / 2, y, size, c);
}
static void wx_gfx(const String& s, int x, int baseY, const GFXfont* f, uint16_t c) {
  tft.setFont(f); tft.setTextSize(1); tft.setTextColor(c);
  tft.setCursor(x, baseY); tft.print(s);
}
static int wx_gfx_w(const String& s, const GFXfont* f) {
  int16_t x1, y1; uint16_t w, h;
  tft.setFont(f); tft.setTextSize(1);
  tft.getTextBounds(s, 0, 0, &x1, &y1, &w, &h);
  return w + x1;
}

// Temperatur mit gezeichnetem Gradzeichen (die GFX-Schriften haben keins)
static int wx_temp_gfx(float v, int x, int baseY, const GFXfont* f, uint16_t c, bool dec) {
  char b[12]; snprintf(b, sizeof(b), dec ? "%.1f" : "%.0f", v);
  wx_gfx(b, x, baseY, f, c);
  x += wx_gfx_w(b, f) + 4;
  tft.drawCircle(x + 2, baseY - 12, 2, c);
  wx_gfx(wx_imperial() ? "F" : "C", x + 7, baseY, f, c);
  return x + 7 + wx_gfx_w("C", f);
}

static uint16_t wx_temp_color(float t) {
  if (wx_imperial()) t = (t - 32) / 1.8f;
  if (t <= 0)  return wx_rgb(0, 180, 255);
  if (t <= 10) return wx_rgb(0, 220, 160);
  if (t <= 20) return wx_rgb(180, 240, 0);
  if (t <= 30) return wx_rgb(255, 210, 0);
  if (t <= 38) return wx_rgb(255, 140, 0);
  return wx_rgb(255, 50, 0);
}

static int wx_wind_disp(float w) { return (int)lroundf(wx_imperial() ? w : w * 3.6f); }
static const char* wx_wind_unit() { return wx_imperial() ? "mph" : "km/h"; }

// OpenWeather-Beschreibung fuer die ASCII-Schriften (Umlaute umschreiben)
static String wx_ascii(const char* s) {
  String o;
  for (const unsigned char* p = (const unsigned char*)s; *p; p++) {
    if (*p == 0xC3 && p[1]) {
      switch (p[1]) {
        case 0xA4: o += "ae"; break; case 0xB6: o += "oe"; break; case 0xBC: o += "ue"; break;
        case 0x84: o += "Ae"; break; case 0x96: o += "Oe"; break; case 0x9C: o += "Ue"; break;
        case 0x9F: o += "ss"; break; default: o += '?';
      }
      p++;
    } else if (*p < 0x80) o += (char)*p;
  }
  if (o.length()) o.setCharAt(0, toupper(o[0]));
  return o;
}

static String wx_hhmm(time_t t) {
  if (!t) return "--:--";
  struct tm tm; localtime_r(&t, &tm);
  char b[8]; strftime(b, sizeof(b), "%H:%M", &tm);
  return b;
}

// ================================================================ Zusatzflaeche (nur CYD)
// Drei Zellen a 80x80: Querformat untereinander (x 240), Hochformat nebeneinander (y 240).
static void wx_cell_pos(int i, int& x, int& y) {
  if (tft.width() > 240) { x = 240; y = i * 80; } else { x = i * 80; y = 240; }
}

static void wx_cell_label(int x, int y, const char* de, const char* en) {
  wx_txt_c(wx_lang_de() ? de : en, x + 40, y + 6, 1, C_MUTED);
}

static void wx_cell_moon(int x, int y) {
  wx_cell_label(x, y, "MOND", "MOON");
  wx_moon_disc(x + 40, y + 32, 13, g_moonPhase);
  const int pct = wx_moon_pct(g_moonPhase);
  wx_txt_c(String(wx_moon_waxing(g_moonPhase) ? ">" : "<") + pct + "%", x + 40, y + 50, 2, wx_rgb(255, 210, 0));
  wx_txt_c(String(wx_moon_days(g_moonPhase)) + (wx_lang_de() ? (wx_moon_waxing(g_moonPhase) ? "d>Voll" : "d>Neu")
                                                              : (wx_moon_waxing(g_moonPhase) ? "d>Full" : "d>New")),
           x + 40, y + 69, 1, C_MUTED);
}

static void wx_cell_wind(int x, int y, const WxNow& w) {
  wx_cell_label(x, y, "WIND", "WIND");
  const int v = wx_wind_disp(w.wind);
  const uint16_t c = v > 60 ? wx_rgb(255, 50, 0) : v > 30 ? wx_rgb(255, 210, 0) : wx_rgb(80, 150, 255);
  wx_txt_c(w.valid ? String(v) : String("--"), x + 40, y + 26, 3, c);
  wx_txt_c(wx_wind_unit(), x + 40, y + 56, 1, C_MUTED);
}

static void wx_cell_sun(int x, int y, const WxNow& w) {
  wx_cell_label(x, y, "SONNE", "SUN");
  wx_txt(String("\x18") + wx_hhmm(w.sunrise), x + 4, y + 26, 2, wx_rgb(255, 210, 40));
  wx_txt(String("\x19") + wx_hhmm(w.sunset),  x + 4, y + 52, 2, wx_rgb(255, 140, 40));
}

static void wx_cell_moonrs(int x, int y) {
  wx_cell_label(x, y, "MOND", "MOON");
  wx_txt(String("\x18") + wx_hhmm(g_moonRise), x + 4, y + 26, 2, wx_rgb(220, 220, 235));
  wx_txt(String("\x19") + wx_hhmm(g_moonSet),  x + 4, y + 52, 2, C_MUTED);
}

static void wx_cell_feels(int x, int y, const WxNow& w) {
  wx_cell_label(x, y, "GEFUEHLT", "FEELS");
  wx_txt_c(w.valid ? String((int)lroundf(w.feels)) : String("--"), x + 34, y + 26, 3, w.valid ? wx_temp_color(w.feels) : C_MUTED);
  tft.drawCircle(x + 34 + (w.valid ? String((int)lroundf(w.feels)).length() : 2) * 9 + 3, y + 28, 2, ST77XX_WHITE);
  wx_txt_c(w.valid ? String(w.humidity) + "% rel" : String(""), x + 40, y + 58, 1, C_MUTED);
}

// moonView: true = Zellen fuer Wetter 2
static void wx_panel(const WxNow& w, bool moonView) {
  if (!wx_has_panel()) return;
  for (int i = 0; i < 3; i++) {
    int x, y; wx_cell_pos(i, x, y);
    wx_clear(x, y, 80, 80);
    if (i) { if (tft.width() > 240) tft.drawFastHLine(x + 8, y, 64, C_LINE); else tft.drawFastVLine(x, y + 8, 64, C_LINE); }
    if (!moonView) { if (i == 0) wx_cell_moon(x, y); else if (i == 1) wx_cell_wind(x, y, w); else wx_cell_sun(x, y, w); }
    else           { if (i == 0) wx_cell_sun(x, y, w); else if (i == 1) wx_cell_moonrs(x, y); else wx_cell_feels(x, y, w); }
  }
  if (tft.width() > 240) tft.drawFastVLine(240, 8, 224, C_LINE); else tft.drawFastHLine(8, 240, 224, C_LINE);
}

// ================================================================ Wetter 1 / Wetter 2
// Gleiches Layout: Wetter 1 = erste Version (schwarz, auf dem CYD mittig),
// Wetter 2 = mit Himmel, Sonnenbogen und CYD-Zusatzflaeche (oben links).
static int wx_vx = 0, wx_vy = 0;   // Versatz des 240er-Feldes

static void wx_w1_date(struct tm* t, bool valid) {
  const int X = wx_vx, Y = wx_vy;
  wx_clear(X, Y, 164, 62);
  char d[16]; String w;
  if (valid) {
    strftime(d, sizeof(d), wx_lang_de() ? "%d.%m.%Y" : "%Y-%m-%d", t);
    w = wx_lang_de() ? WD_LONG_DE[t->tm_wday] : WD_LONG_EN[t->tm_wday];
  } else { strcpy(d, "--.--.----"); w = wx_lang_de() ? "WARTE NTP" : "WAIT NTP"; }
  wx_gfx(d, X + 5, Y + 20, &OCRA10pt7b, ST77XX_GREEN);
  wx_gfx(w, X + 5, Y + 45, &OCRA10pt7b, ST77XX_GREEN);
}

static void wx_w1_clock(struct tm* t, bool valid) {
  const int X = wx_vx, Y = wx_vy;
  wx_clear(X, Y + 72, 240, 80);
  char h[4] = "--", m[4] = "--";
  if (valid) { strftime(h, sizeof(h), "%H", t); strftime(m, sizeof(m), "%M", t); }
  const int wh = wx_gfx_w(h, &ArchivoNB50pt7b), wc = wx_gfx_w(":", &ArchivoNB50pt7b), wm = wx_gfx_w(m, &ArchivoNB50pt7b);
  int x = X + (240 - (wh + 4 + wc + 4 + wm)) / 2;
  wx_gfx(h, x, Y + 148, &ArchivoNB50pt7b, ST77XX_WHITE);   x += wh + 4;
  wx_gfx(":", x, Y + 148, &ArchivoNB50pt7b, ST77XX_WHITE); x += wc + 4;
  wx_gfx(m, x, Y + 148, &ArchivoNB50pt7b, C_MINUTE);
}

// nur Wetter 2
static void wx_w1_arc(const WxNow& w, time_t now) {
  wx_clear(wx_vx, wx_vy + 152, 240, 9);
  wx_sun_arc(wx_vx + 12, wx_vx + 228, wx_vy + 159, 6, w, now);
}

static void wx_w1_weather(const WxNow& w) {
  const int X = wx_vx, Y = wx_vy;
  wx_clear(X + 166, Y, 74, 70);
  wx_draw_icon(w.valid ? w.conditionId : 803, wx_is_night(w), X + 168, Y + 2, 64);

  wx_clear(X, Y + 162, 240, 78);
  const uint16_t bar = 0x053F;
  const bool imp = wx_imperial();
  char buf[32];
  wx_draw_thermometer(X + 12, Y + 172);
  tft.drawRect(X + 24, Y + 168, 96, 9, ST77XX_WHITE);
  if (w.valid) tft.fillRect(X + 25, Y + 169, constrain(map((int)w.temp, imp ? 14 : -10, imp ? 104 : 40, 0, 94), 0, 94), 7, bar);
  if (w.valid) snprintf(buf, sizeof(buf), "%.0f\xF7%c", w.temp, imp ? 'F' : 'C'); else snprintf(buf, sizeof(buf), "--\xF7%c", imp ? 'F' : 'C');
  wx_txt(buf, X + 130, Y + 165, 2, ST77XX_WHITE);

  wx_draw_drop(X + 12, Y + 193);
  tft.drawRect(X + 24, Y + 189, 96, 9, ST77XX_WHITE);
  if (w.valid) tft.fillRect(X + 25, Y + 190, constrain(map(w.humidity, 0, 100, 0, 94), 0, 94), 7, bar);
  if (w.valid) snprintf(buf, sizeof(buf), "%d%%", w.humidity); else strcpy(buf, "--%");
  wx_gfx(buf, X + 130, Y + 198, &OCRA10pt7b, ST77XX_WHITE);

  // Unten: Luftdruck in hPa und cmHg (1 hPa = 0,0750062 cmHg)
  if (w.valid) snprintf(buf, sizeof(buf), "%dhPa %.1fcmHg", w.pressure, w.pressure * 0.0750062f);
  else         strcpy(buf, "----hPa --.-cmHg");
  wx_gfx_center(buf, X + 120, Y + 228, &OCRA10pt7b, ST77XX_WHITE);
}

// classic = Wetter 1 (erste Version), sonst Wetter 2
static void wx_w1_full(const WxNow& w, time_t now, bool classic) {
  struct tm t; localtime_r(&now, &t);
  const bool valid = now > 1700000000;
  wx_vx = classic ? (tft.width() - 240) / 2 : 0;
  wx_vy = classic ? (tft.height() - 240) / 2 : 0;
  wx_clear(0, 0, tft.width(), tft.height());
  wx_w1_date(&t, valid);
  wx_w1_clock(&t, valid);
  if (!classic) wx_w1_arc(w, now);
  wx_w1_weather(w);
  if (!classic) wx_panel(w, false);
}
// ================================================================ Wetter 3 (mit Mond)
static void wx_w2_top(const WxNow& w) {
  wx_clear(0, 0, 240, 55);
  wx_moon_disc(28, 28, 20, g_moonPhase);
  const int pct = wx_moon_pct(g_moonPhase);
  const bool wax = wx_moon_waxing(g_moonPhase), de = wx_lang_de();
  String p = pct < 5 ? String(de ? "N(" : "N(") + pct + "%)" : pct > 95 ? String(de ? "V(" : "F(") + pct + "%)"
           : String(wax ? ">(" : "<(") + pct + "%)";
  wx_gfx(p, 54, 22, &OCRA9pt7b, wx_rgb(255, 210, 0));
  String d = String(wx_moon_days(g_moonPhase)) + "d > " + (de ? (wax ? "Voll" : "Neu") : (wax ? "Full" : "New"));
  wx_gfx(d, 54, 42, &OCRA9pt7b, C_MUTED);
  wx_draw_icon(w.valid ? w.conditionId : 803, wx_is_night(w), 199, 10, 32);
  tft.drawFastHLine(0, 56, 240, wx_rgb(35, 35, 40));
}

static void wx_w2_temp(const WxNow& w) {
  wx_clear(0, 58, 240, 22);
  if (!w.valid) { wx_gfx("--", 8, 76, &OCRA10pt7b, C_MUTED); return; }
  const int x = wx_temp_gfx(w.temp, 8, 76, &OCRA10pt7b, wx_temp_color(w.temp), true);
  wx_gfx(String(w.humidity) + "%", x + 10, 76, &OCRA10pt7b, ST77XX_WHITE);
}

static void wx_w2_clock(struct tm* t, bool valid) {
  wx_clear(0, 80, 240, 46);
  char c[8] = "--:--";
  if (valid) strftime(c, sizeof(c), "%H:%M", t);
  wx_gfx_center(c, 120, 124, &ArchivoNB30pt7b, ST77XX_WHITE);
}

static void wx_w2_date(struct tm* t, bool valid) {
  wx_clear(0, 127, 240, 40);
  if (!valid) return;
  char d[16]; strftime(d, sizeof(d), wx_lang_de() ? "%d.%m.%Y" : "%Y-%m-%d", t);
  wx_gfx_center(d, 120, 144, &OCRA10pt7b, wx_rgb(0, 220, 100));
  wx_gfx_center(wx_lang_de() ? WD_MIX_DE[t->tm_wday] : WD_MIX_EN[t->tm_wday], 120, 163, &OCRA10pt7b, wx_rgb(0, 220, 100));
}

static void wx_w2_arc(const WxNow& w, time_t now) {
  wx_clear(0, 167, 240, 9);
  wx_sun_arc(8, 232, 174, 6, w, now);
}

static void wx_w2_bottom(const WxNow& w) {
  wx_clear(0, 176, 240, 64);
  if (!w.valid) return;
  const int v = wx_wind_disp(w.wind);
  const uint16_t wc = v > 60 ? wx_rgb(255, 50, 0) : v > 30 ? wx_rgb(255, 210, 0) : wx_rgb(80, 150, 255);
  wx_gfx(String("Wind ") + v + " " + wx_wind_unit(), 5, 192, &OCRA9pt7b, wc);
  String d = wx_ascii(w.desc);
  if (d.length() > 17) d = d.substring(0, 17);
  wx_gfx(d, 5, 212, &OCRA9pt7b, ST77XX_WHITE);
  char b[32]; snprintf(b, sizeof(b), "%d hPa %.1f cmHg", w.pressure, w.pressure * 0.0750062f);
  wx_gfx(b, 5, 232, &OCRA9pt7b, ST77XX_WHITE);
}

static void wx_w2_full(const WxNow& w, time_t now) {
  struct tm t; localtime_r(&now, &t);
  const bool valid = now > 1700000000;
  wx_clear(0, 0, tft.width(), tft.height());
  wx_w2_top(w);
  wx_w2_temp(w);
  wx_w2_clock(&t, valid);
  wx_w2_date(&t, valid);
  wx_w2_arc(w, now);
  wx_w2_bottom(w);
  wx_panel(w, true);
}
