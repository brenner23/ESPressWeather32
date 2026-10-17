#pragma once
#include "Board.h"

#if defined(BOARD_NMTV154)
  // NM-TV-154 - ST7789 240x240
  #define TFT_MOSI       13
  #define TFT_MISO       12
  #define TFT_SCK        14
  #define TFT_CS         15
  #define TFT_DC          2      // GPIO2 = Data/Command (darum keine LED auf GPIO2!)
  #define TFT_RST         4
  #define TFT_EN_PIN     21      // Display-Versorgung, LOW = an
  #define TFT_BACKLIGHT  19      // LOW-aktiv
  #define TOUCH_CAP_PIN  32      // kapazitive Taste
  #define TOUCH_THRESH   90      // gedrueckt ~82 / Ruhe ~106
  #define TOUCH_CONFIRM_MS 80

#elif defined(BOARD_C2)
  // MiniTV ESP32-C2 - ST7789V 240x240, CS fest auf GND, kein MISO
  #define TFT_SCK         4
  #define TFT_MOSI        6
  #define TFT_MISO       -1
  #define TFT_CS         -1
  #define TFT_DC          5
  #define TFT_RST         1
  #define TFT_BACKLIGHT  18      // LOW-aktiv

#else
  // CYD - ST7789 240x320
  #define TFT_MOSI       13
  #define TFT_MISO       12
  #define TFT_SCK        14
  #define TFT_CS         15
  #define TFT_DC          2      // GPIO2 = Data/Command (darum keine LED auf GPIO2!)
  #define TFT_RST        -1
  #define TFT_BACKLIGHT  21      // HIGH-aktiv

  // XPT2046 Touch - eigener SPI-Bus
  #define TOUCH_SCK      25
  #define TOUCH_MOSI     32
  #define TOUCH_MISO     39
  #define TOUCH_CS       33
  #define TOUCH_IRQ      36

  // Onboard CdS/LDR des CYD
  #define LDR_PIN        34
#endif
