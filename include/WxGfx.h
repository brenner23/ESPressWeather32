#pragma once
// ESPressWeather32: alle Zeichenfunktionen zeichnen ueber "tft" = *g_gfx.
// Normal zeigt g_gfx auf das echte Display (tft_hw). Fuer die Vorschau im Web
// zeigt es kurz auf WxCapture: dasselbe Bild wird in Streifen in den RAM gerendert.
#include <Adafruit_GFX.h>

extern Adafruit_GFX* g_gfx;
#define tft (*g_gfx)

// Rendert nur die Zeilen y0 .. y0+rows-1 in buf (RGB565). Alles andere wird verworfen.
class WxCapture : public Adafruit_GFX {
public:
  uint16_t* buf = nullptr;
  int16_t   y0 = 0, rows = 0;
  WxCapture(int16_t w, int16_t h) : Adafruit_GFX(w, h) {}
  void drawPixel(int16_t x, int16_t y, uint16_t c) override {
    if (x < 0 || x >= _width || y < y0 || y >= y0 + rows) return;
    buf[(y - y0) * _width + x] = c;
  }
  void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t c) override {
    int16_t xa = max<int16_t>(x, 0), xb = min<int16_t>(x + w, _width);
    int16_t ya = max<int16_t>(y, y0), yb = min<int16_t>(y + h, y0 + rows);
    for (int16_t yy = ya; yy < yb; yy++) {
      uint16_t* p = buf + (yy - y0) * _width;
      for (int16_t xx = xa; xx < xb; xx++) p[xx] = c;
    }
  }
  void drawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t c) override { fillRect(x, y, w, 1, c); }
  void drawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t c) override { fillRect(x, y, 1, h, c); }
};
