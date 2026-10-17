#pragma once
// ESPressWeather32: Geraeteprofile. Auswahl per Build-Flag in platformio.ini:
//   -DBOARD_CYD      ESP32-2432S028 "Cheap Yellow Display" (ST7789 240x320, XPT2046-Touch, LDR)
//   -DBOARD_NMTV154  NM-TV-154 / NerdMiner TV (ST7789 240x240, kapazitiver Touch)
//   -DBOARD_C2       MiniTV ESP32-C2 (ST7789 240x240, ein Kern, kein Touch, kein BOOT-Knopf)
#include <SPI.h>

#if defined(BOARD_NMTV154)
  #define WX_BOARD           "NM-TV-154"
  #define WX_TFT_W           240
  #define WX_TFT_H           240
  #define WX_BL_ACTIVE_LOW   1     // Backlight GPIO19, LOW = an
  #define WX_HAS_XPT         0
  #define WX_HAS_CAP_TOUCH   1     // GPIO32: kurz = Licht, doppelt = Ansicht, 10 s = Portal
  #define WX_HAS_LDR         0
  #define WX_HAS_ROTATION    0
  #define WX_FIXED_ROTATION  2
  #define WX_TFT_INVERT_OFF  0     // Panel braucht die Standard-Invertierung
  #define WX_TFT_SPI_MODE    SPI_MODE0
  #define WX_BOOT_PIN        0
#elif defined(BOARD_C2)
  #define WX_BOARD           "MiniTV C2"
  #define WX_TFT_W           240
  #define WX_TFT_H           240
  #define WX_BL_ACTIVE_LOW   1     // Backlight GPIO18, LOW = an
  #define WX_HAS_XPT         0
  #define WX_HAS_CAP_TOUCH   0
  #define WX_HAS_LDR         0
  #define WX_HAS_ROTATION    0
  #define WX_FIXED_ROTATION  2
  #define WX_TFT_INVERT_OFF  0
  #define WX_TFT_SPI_MODE    SPI_MODE3  // CS liegt fest auf GND
  #define WX_BOOT_PIN        -1    // kein BOOT-Knopf erreichbar: Portal nur bei WLAN-Ausfall/Erststart
#else
  #ifndef BOARD_CYD
    #define BOARD_CYD
  #endif
  #define WX_BOARD           "CYD 2.8"
  #define WX_TFT_W           240
  #define WX_TFT_H           320
  #define WX_BL_ACTIVE_LOW   0     // Backlight GPIO21, HIGH = an
  #define WX_HAS_XPT         1
  #define WX_HAS_CAP_TOUCH   0
  #define WX_HAS_LDR         1
  #define WX_HAS_ROTATION    1
  #define WX_FIXED_ROTATION  1
  #define WX_TFT_INVERT_OFF  1     // CYD-Panel: Inversion aus, sonst Negativfarben
  #define WX_TFT_SPI_MODE    SPI_MODE0
  #define WX_BOOT_PIN        0
#endif

#define WX_HAS_TOUCH (WX_HAS_XPT || WX_HAS_CAP_TOUCH)

// Backlight-PWM unabhaengig von der Polaritaet (duty 0..255 = dunkel..hell)
#define WX_BL_PWM(duty)  (WX_BL_ACTIVE_LOW ? (255 - (int)(duty)) : (int)(duty))
