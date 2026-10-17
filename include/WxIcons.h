#pragma once
// ESPressWeather32: Wetter-Icons als Vektorgrafik (eigener Code, keine Bilddateien).
// Entworfen auf einem 32er-Raster; k = 1 -> 32x32, k = 2 -> 64x64.
#include <Adafruit_ST7789.h>
#include "WxSky.h"

#include "WxGfx.h"

#define WXI_YELLOW  0xFE60
#define WXI_ORANGE  0xFD20
#define WXI_MOON    0xFF56
#define WXI_CLOUD   0xCE79
#define WXI_CLOUD2  0x9CD3
#define WXI_DARK    0x6B4D
#define WXI_RAIN    0x05BF
#define WXI_SNOW    0xFFFF
#define WXI_FOG     0x8C71

static void wxi_thick_line(int x0, int y0, int x1, int y1, int w, uint16_t c) {
  const int a = -(w - 1) / 2, b = w / 2;
  for (int dx = a; dx <= b; dx++)
    for (int dy = a; dy <= b; dy++) tft.drawLine(x0 + dx, y0 + dy, x1 + dx, y1 + dy, c);
}

static void wxi_sun(int x, int y, int k, int cx = 16, int cy = 16, int r = 7) {
  const int X = x + cx * k, Y = y + cy * k;
  static const int8_t dir[8][2] = {{0,-1},{1,-1},{1,0},{1,1},{0,1},{-1,1},{-1,0},{-1,-1}};
  for (auto& d : dir) {
    const float n = (d[0] && d[1]) ? 0.7071f : 1.0f;
    const int r0 = (r + 3) * k, r1 = (r + 6) * k;
    wxi_thick_line(X + (int)(d[0] * n * r0), Y + (int)(d[1] * n * r0),
                   X + (int)(d[0] * n * r1), Y + (int)(d[1] * n * r1), k + 1, WXI_YELLOW);
  }
  tft.fillCircle(X, Y, r * k, WXI_YELLOW);
  tft.fillCircle(X - k, Y - k, (r - 3) * k, 0xFF20);
}

// Nachts: echter Mond mit aktueller Phase (wx_moon_disc aus WxSky.h)
static void wxi_moon(int x, int y, int k, int cx = 16, int cy = 16, int r = 10) {
  wx_moon_disc(x + cx * k, y + cy * k, r * k, g_moonPhase);
}

static void wxi_cloud(int x, int y, int k, uint16_t c = WXI_CLOUD) {
  tft.fillCircle(x + 10 * k, y + 19 * k, 6 * k, c);
  tft.fillCircle(x + 17 * k, y + 14 * k, 8 * k, c);
  tft.fillCircle(x + 24 * k, y + 19 * k, 6 * k, c);
  tft.fillRoundRect(x + 4 * k, y + 18 * k, 26 * k, 8 * k, 3 * k, c);
}

static void wxi_rain(int x, int y, int k, bool heavy) {
  wxi_cloud(x, y - 3 * k, k, heavy ? WXI_DARK : WXI_CLOUD2);
  const int n = heavy ? 4 : 3;
  for (int i = 0; i < n; i++) {
    const int dx = (heavy ? 8 : 10) + i * (heavy ? 5 : 6);
    wxi_thick_line(x + dx * k, y + 25 * k, x + (dx - 2) * k, y + 30 * k, k, WXI_RAIN);
  }
}

static void wxi_thunder(int x, int y, int k) {
  wxi_cloud(x, y - 3 * k, k, WXI_DARK);
  tft.fillTriangle(x + 18 * k, y + 20 * k, x + 12 * k, y + 27 * k, x + 17 * k, y + 27 * k, WXI_YELLOW);
  tft.fillTriangle(x + 16 * k, y + 25 * k, x + 20 * k, y + 25 * k, x + 13 * k, y + 32 * k - 1, WXI_YELLOW);
}

static void wxi_snow(int x, int y, int k) {
  wxi_cloud(x, y - 3 * k, k, WXI_CLOUD2);
  static const int8_t f[3][2] = {{10, 27}, {17, 29}, {24, 27}};
  for (auto& p : f) {
    const int X = x + p[0] * k, Y = y + p[1] * k, s = 2 * k;
    tft.drawFastHLine(X - s, Y, 2 * s + 1, WXI_SNOW);
    tft.drawFastVLine(X, Y - s, 2 * s + 1, WXI_SNOW);
    tft.drawLine(X - s + 1, Y - s + 1, X + s - 1, Y + s - 1, WXI_SNOW);
    tft.drawLine(X - s + 1, Y + s - 1, X + s - 1, Y - s + 1, WXI_SNOW);
  }
}

static void wxi_fog(int x, int y, int k) {
  for (int i = 0; i < 4; i++) {
    const int off = (i % 2) ? 3 : 0;
    tft.fillRoundRect(x + (4 + off) * k, y + (9 + i * 5) * k, (24 - off) * k, 3 * k, k + 1, WXI_FOG);
  }
}

// OpenWeather-Zustands-ID -> Icon. size = 32 oder 64.
static void wx_draw_icon(int id, bool night, int x, int y, int size) {
  const int k = size >= 64 ? 2 : 1;
  if (id >= 200 && id < 300)       wxi_thunder(x, y, k);
  else if (id >= 300 && id < 400)  wxi_rain(x, y, k, false);
  else if (id >= 500 && id < 600)  wxi_rain(x, y, k, id >= 502);
  else if (id >= 600 && id < 700)  wxi_snow(x, y, k);
  else if (id >= 700 && id < 800)  wxi_fog(x, y, k);
  else if (id == 800)              { if (night) wxi_moon(x, y, k); else wxi_sun(x, y, k); }
  else if (id == 801 || id == 802) {
    if (night) wxi_moon(x, y, k, 21, 10, 7); else wxi_sun(x, y, k, 21, 11, 5);
    wxi_cloud(x, y + 3 * k, k);
  }
  else {                           // 803/804: nachts steht der Mond hinter der Wolke
    if (night) wxi_moon(x, y, k, 11, 10, 8);
    wxi_cloud(x, y + (night ? 3 * k : 0), k, id == 804 ? WXI_CLOUD2 : WXI_CLOUD);
  }
}

// Kleine Symbole der Live-Ansicht
static void wx_draw_thermometer(int x, int y) {
  tft.fillCircle(x, y + 4, 4, ST77XX_RED);
  tft.fillRect(x - 2, y - 6, 4, 8, ST77XX_RED);
  tft.fillCircle(x, y - 6, 2, ST77XX_RED);
  tft.drawPixel(x - 1, y + 4, ST77XX_WHITE);
}

static void wx_draw_drop(int x, int y) {
  const uint16_t blue = 0x05BF;
  tft.fillCircle(x, y + 3, 4, blue);
  tft.fillTriangle(x - 4, y + 2, x + 4, y + 2, x, y - 6, blue);
  tft.drawPixel(x - 1, y + 2, ST77XX_WHITE);
}
