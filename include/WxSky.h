#pragma once
// ESPressWeather32: Himmel-Hintergrund fuer Wetter 1 + 2.
// Tag: dunkelblauer Verlauf, um Sonnenauf-/-untergang ein Daemmerungsschimmer,
// nachts Sterne (blenden 30 min nach Sonnenuntergang ein und 30 min vor
// Sonnenaufgang aus). Sonnenzeiten kommen aus der OpenWeather-Antwort.
// Dazu Mondphase (Referenz-Neumond wie im Display-Projekt auf .99) und
// Mondauf-/-untergang (vereinfachte Mondbahn, Genauigkeit wenige Minuten).
#include <Adafruit_ST7789.h>
#include <math.h>
#include "WxData.h"
#include "WxSettings.h"

#include "WxGfx.h"

// ---------------------------------------------------------------- Zustand
static bool    g_skyActive  = false;  // aktuelle Ansicht zeichnet Himmel (Schalter an + Wetter 1/2)
static uint8_t g_skyDay     = 10;     // 0..10 Tageslicht
static uint8_t g_skyDusk    = 0;      // 0..10 Daemmerung
static uint8_t g_skyStars   = 0;      // 0..10 Sterne
static uint8_t g_skyPreview = 0;      // 0 = live, 1 = Tag, 2 = Daemmerung, 3 = Nacht (nur Vorschau)
static uint16_t g_skyRow[320];        // Hintergrundfarbe je Zeile

static inline uint16_t wx_rgb(int r, int g, int b) {
  r = constrain(r, 0, 255); g = constrain(g, 0, 255); b = constrain(b, 0, 255);
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Stufen aus Sonnenauf-/-untergang (oder Vorschau) bestimmen. true = geaendert.
static bool wx_sky_update(const WxNow& w, time_t now) {
  uint8_t day = 10, dusk = 0, stars = 0;
  if (g_skyPreview == 1)      { day = 10; dusk = 0;  stars = 0; }
  else if (g_skyPreview == 2) { day = 4;  dusk = 10; stars = 3; }
  else if (g_skyPreview == 3) { day = 0;  dusk = 0;  stars = 10; }
  else if (w.valid && w.sunrise && w.sunset) {
    const long F = 1800;   // 30 min Ein-/Ausblenden
    const long t = (long)now, rise = (long)w.sunrise, set = (long)w.sunset;
    long s;   // Sterne in Sekunden-Anteil 0..F
    if (t >= set && t < set + F)        s = t - set;
    else if (t > rise - F && t <= rise) s = rise - t;
    else if (t >= set + F || t <= rise - F) s = F;
    else s = 0;
    stars = (uint8_t)((s * 10 + F / 2) / F);
    day   = 10 - stars;
    const long D = 2700;   // Daemmerung +-45 min
    const long dd = min(labs(t - set), labs(t - rise));
    dusk = dd < D ? (uint8_t)(((D - dd) * 10) / D) : 0;
  } else if (w.valid && strchr(w.icon, 'n')) { day = 0; stars = 10; }
  const bool changed = day != g_skyDay || dusk != g_skyDusk || stars != g_skyStars;
  g_skyDay = day; g_skyDusk = dusk; g_skyStars = stars;
  return changed;
}

// Zeilenfarben neu berechnen
static void wx_sky_rows() {
  const int H = tft.height();
  auto mix = [](float a, float b, float t) { return a + (b - a) * t; };
  const float d = g_skyDay / 10.0f, k = g_skyDusk / 10.0f * 0.85f;
  for (int y = 0; y < H && y < 320; y++) {
    const float f = (float)y / (H - 1);
    // Nacht: fast schwarzes Nachtblau
    float r = mix(0, 0, f), g = mix(4, 0, f), b = mix(22, 2, f);
    // Tag: Himmelblau oben, nach unten dunkler (Schrift bleibt gut lesbar)
    r = mix(r, mix(25, 5, f), d); g = mix(g, mix(80, 25, f), d); b = mix(b, mix(160, 60, f), d);
    // Daemmerung: oben violett-blau, unten Orange-Glut
    r = mix(r, mix(35, 170, f * f), k); g = mix(g, mix(30, 70, f * f), k); b = mix(b, mix(85, 30, f * f), k);
    g_skyRow[y] = wx_rgb((int)r, (int)g, (int)b);
  }
}

// Sterne: feste Positionen (Startwert fest), damit nichts springt
static inline uint32_t wx_hash(uint32_t x) {
  x ^= x >> 16; x *= 0x7feb352d; x ^= x >> 15; x *= 0x846ca68b; x ^= x >> 16;
  return x;
}

static void wx_stars_in(int x0, int y0, int w, int h) {
  if (!g_skyStars) return;
  const int W = tft.width(), H = tft.height();
  const int n = (W * H) / 380;
  const int vis = n * g_skyStars / 10;
  static const uint8_t tint[4][3] = {{255, 255, 255}, {200, 215, 255}, {255, 235, 200}, {220, 220, 235}};
  for (int i = 0; i < vis; i++) {
    const uint32_t a = wx_hash(i * 2 + 1), b = wx_hash(i * 2 + 2);
    const int x = a % W, y = b % H;
    const uint8_t kind = (a >> 24) % 12;          // 0 = Funkelstern, 1-3 = hell 2x2, sonst klein
    const uint8_t* c = tint[(b >> 24) & 3];
    const int m = kind == 0 ? 2 : kind <= 3 ? 1 : 0;   // Rand, den der Stern braucht
    if (x - m < x0 || y - m < y0 || x + m >= x0 + w || y + m >= y0 + h) continue;
    if (kind == 0) {          // grosser Stern mit kleinem Kreuz
      tft.drawPixel(x, y, wx_rgb(c[0], c[1], c[2]));
      const uint16_t s = wx_rgb(c[0] * 6 / 10, c[1] * 6 / 10, c[2] * 6 / 10);
      tft.drawPixel(x - 1, y, s); tft.drawPixel(x + 1, y, s); tft.drawPixel(x, y - 1, s); tft.drawPixel(x, y + 1, s);
      const uint16_t f = wx_rgb(c[0] / 4, c[1] / 4, c[2] / 4);
      tft.drawPixel(x - 2, y, f); tft.drawPixel(x + 2, y, f); tft.drawPixel(x, y - 2, f); tft.drawPixel(x, y + 2, f);
    } else if (kind <= 3) {   // heller Stern 2x2
      tft.fillRect(x, y, 2, 2, wx_rgb(c[0] * 8 / 10, c[1] * 8 / 10, c[2] * 8 / 10));
    } else {                  // kleiner Stern, unterschiedlich hell
      const int k = 45 + (int)((a >> 16) % 40);   // 45..84 %
      tft.drawPixel(x, y, wx_rgb(c[0] * k / 100, c[1] * k / 100, c[2] * k / 100));
    }
  }
}

// Bereich mit Hintergrund (Himmel oder schwarz) fuellen
static void wx_clear(int x, int y, int w, int h) {
  if (!g_skyActive) { tft.fillRect(x, y, w, h, ST77XX_BLACK); return; }
  const int H = tft.height();
  for (int r = max(0, y); r < y + h && r < H; r++) tft.drawFastHLine(x, r, w, g_skyRow[r]);
  wx_stars_in(x, y, w, h);
}

// ---------------------------------------------------------------- Mond
// Referenz-Neumond und synodische Periode wie im Wetterbild auf dem .99
static const double WX_LUNATION_S  = 29.530588853 * 86400.0;
static const time_t WX_KNOWN_NEW   = 1773886980;
static float g_moonPhase = 0.0f;      // 0 = Neumond, 0,5 = Vollmond

static float wx_moon_phase(time_t now) {
  double p = fmod((double)(now - WX_KNOWN_NEW), WX_LUNATION_S) / WX_LUNATION_S;
  if (p < 0) p += 1.0;
  return (float)p;
}
static int  wx_moon_pct(float p)     { return (int)((p <= 0.5f ? p * 200.0f : (1.0f - p) * 200.0f) + 0.5f); }
static bool wx_moon_waxing(float p)  { return p < 0.5f; }
static int  wx_moon_days(float p)    { return (int)ceil(((p < 0.5f) ? (0.5f - p) : (1.0f - p)) * 29.530588853f); }

// Mondscheibe mit Ellipsen-Terminator (Methode aus dem eigenen Moon.h / weather2.php)
static void wx_moon_disc(int cx, int cy, int r, float phase) {
  const uint16_t lit = wx_rgb(240, 236, 215), dark = wx_rgb(28, 30, 38);
  tft.fillCircle(cx, cy, r, dark);
  if (phase >= 0.02f && phase <= 0.98f) {
    const float cosT = cosf(phase * (float)M_PI);
    for (int dy = -r; dy <= r; dy++) {
      const int rr = r * r - dy * dy;
      if (rr < 0) continue;
      const int dx = (int)sqrtf((float)rr);
      const int tx = cx + (int)lroundf(cosT * dx);
      int xs, xe;
      if (phase <= 0.5f) { xs = max(tx, cx - dx); xe = cx + dx; }   // zunehmend: rechts hell
      else               { xs = cx - dx; xe = min(tx, cx + dx); }   // abnehmend: links hell
      if (xe > xs) tft.drawFastHLine(xs, cy + dy, xe - xs + 1, lit);
    }
  }
  tft.drawCircle(cx, cy, r + 1, wx_rgb(110, 110, 120));
}

// Mondhoehe (rad) zur Zeit t am Ort lat/lon - vereinfachte Mondbahn
static double wx_moon_alt(double t, double lat, double lon) {
  const double rad = M_PI / 180.0, d = (t - 946728000.0) / 86400.0;
  const double L = rad * (218.316 + 13.176396 * d), M = rad * (134.963 + 13.064993 * d), F = rad * (93.272 + 13.229350 * d);
  const double l = L + rad * 6.289 * sin(M), b = rad * 5.128 * sin(F), e = rad * 23.4397;
  const double ra  = atan2(sin(l) * cos(e) - tan(b) * sin(e), cos(l));
  const double dec = asin(sin(b) * cos(e) + cos(b) * sin(e) * sin(l));
  const double H   = rad * (280.16 + 360.9856235 * d) + rad * lon - ra;
  const double phi = rad * lat;
  return asin(sin(phi) * sin(dec) + cos(phi) * cos(dec) * cos(H));
}

// Mondauf-/-untergang des heutigen Tages (lokal). 0 = findet heute nicht statt.
static time_t g_moonRise = 0, g_moonSet = 0;
static void wx_moon_times(time_t now, float lat, float lon) {
  static int lastDay = -1; static float lastLat = 999, lastLon = 999;
  struct tm t; localtime_r(&now, &t);
  if (t.tm_yday == lastDay && lat == lastLat && lon == lastLon) return;
  lastDay = t.tm_yday; lastLat = lat; lastLon = lon;
  t.tm_hour = 0; t.tm_min = 0; t.tm_sec = 0;
  const time_t day0 = mktime(&t);
  const double h0 = 0.133 * M_PI / 180.0;
  g_moonRise = g_moonSet = 0;
  double prev = wx_moon_alt(day0, lat, lon) - h0;
  for (int i = 1; i <= 144; i++) {
    const double tt = day0 + i * 600.0, cur = wx_moon_alt(tt, lat, lon) - h0;
    if (prev < 0 && cur >= 0 && !g_moonRise) g_moonRise = (time_t)(tt - 600.0 + 600.0 * (-prev) / (cur - prev));
    if (prev >= 0 && cur < 0 && !g_moonSet)  g_moonSet  = (time_t)(tt - 600.0 + 600.0 * prev / (prev - cur));
    prev = cur;
  }
}

// Nacht? (Vorschau hat Vorrang vor dem OpenWeather-Tag/Nacht-Kennzeichen)
static bool wx_is_night(const WxNow& w) {
  if (g_skyPreview == 3) return true;
  if (g_skyPreview) return false;
  return w.valid && strchr(w.icon, 'n') != nullptr;
}

// ---------------------------------------------------------------- Sonnenbogen
// Flacher Bogen von Sonnenauf- bis -untergang mit Sonne an der aktuellen Stelle,
// nachts wandert der Mond von Sonnenunter- bis -aufgang.
static void wx_sun_arc(int x0, int x1, int yBase, int height, const WxNow& w, time_t now) {
  if (!g_skyActive || !w.valid || !w.sunrise || !w.sunset) {
    tft.drawFastHLine(x0, yBase, x1 - x0, wx_rgb(45, 45, 50));   // ohne Himmel: schlichte Linie
    return;
  }
  const uint16_t dot = wx_rgb(150, 160, 190);
  for (int x = x0; x <= x1; x += 3) {
    const float f = (float)(x - x0) / (x1 - x0);
    tft.drawPixel(x, yBase - (int)(sinf(f * (float)M_PI) * height), dot);
  }
  const long t = (long)now, rise = (long)w.sunrise, set = (long)w.sunset;
  float f; bool day;
  if (t >= rise && t <= set) { f = (float)(t - rise) / (float)(set - rise); day = true; }
  else {
    const long s0 = (t > set) ? set : set - 86400, r1 = (t > set) ? rise + 86400 : rise;
    f = (float)(t - s0) / (float)(r1 - s0); day = false;
  }
  if (g_skyPreview) { day = g_skyPreview != 3; f = g_skyPreview == 2 ? 0.93f : 0.5f; }   // Vorschau
  f = constrain(f, 0.0f, 1.0f);
  const int px = x0 + (int)(f * (x1 - x0)), py = yBase - (int)(sinf(f * (float)M_PI) * height);
  if (day) { tft.fillCircle(px, py, 4, wx_rgb(120, 90, 0)); tft.fillCircle(px, py, 3, wx_rgb(255, 210, 40)); }
  else     wx_moon_disc(px, py, 3, g_moonPhase);
}
