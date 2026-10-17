# ☁ ESPressWeather32

🇬🇧 **English** · 🇩🇪 [Deutsch](README.de.md)

A **weather display for ESP32 display boards** – one firmware for the **CYD** (ESP32-2432S028),
the **NM-TV-154** and the **MiniTV ESP32-C2**. Clock, current weather, 3-day forecast, moon phase and
an energy view for a Tasmota smart plug. Setup portal, web dashboard and settings in the style of
[ESPressMiner32](https://github.com/brenner23/ESPressMiner32) and [ESPressDuino](https://github.com/brenner23/ESPressDuino).
No credentials in the source code.

🔗 **Project page:** https://github.com/brenner23/ESPressWeather32

<img src="docs/screen_weather2_dusk.png" width="320" alt="Weather 2 at dusk (CYD, landscape)"> <img src="docs/screen_weather3_moon_night.png" width="320" alt="Weather 3 with moon at night">

<img src="docs/screen_weather1_day.png" width="210" alt="Weather 1, day"> <img src="docs/screen_weather1_night.png" width="210" alt="Weather 1, night"> <img src="docs/screen_forecast_night.png" width="210" alt="3-day forecast"> <img src="docs/screen_energy_day.png" width="210" alt="Energy view">

*Pictures: display images rendered by the device itself (debug preview, see below), CYD in landscape.*

## Supported boards

| Board | Display | Status |
|---|---|---|
| **CYD** ESP32-2432S028 | ST7789 240×320, XPT2046 touch, light sensor | ✅ version 1.0, tested |
| **NM-TV-154** (NerdMiner TV) | ST7789 240×240, touch pad | 🔨 version 1.0 builds, device test pending |
| **MiniTV ESP32-C2** | ST7789 240×240, single core | 🔨 version 1.0 builds, device test pending |

## Views

| View | Contents |
|---|---|
| **Weather 1** | Date, weekday, big clock, weather icon, temperature and humidity bars, pressure in hPa and cmHg |
| **Weather 2** | Like Weather 1, plus a sun arc under the clock (sun / moon moves from sunrise to sunset). On the CYD the free area (right in landscape, bottom in portrait) shows moon phase, wind and sunrise / sunset |
| **Weather 3** | Moon phase with % and days to full / new moon, icon, temperature (coloured), clock, date, wind, description, pressure. CYD extra area: sunrise / sunset, moonrise / moonset, feels-like |
| **3 days** | Forecast for the next three days (min / max, rain probability) |
| **Energy** | Power, today, yesterday, total of a Tasmota plug (via MQTT) and the clock |
| **Auto** | Changes the view every N seconds |

**Sky background on all views:** blue by day, a violet-orange glow at dusk and dawn, night blue with stars at night.
The stars fade in for 30 minutes after sunset and fade out 30 minutes before sunrise – sunrise and sunset come with
the OpenWeather data for your city. At night the weather icon shows the real moon phase, behind a cloud if it is cloudy.
Only changed areas are redrawn (background section first, then the content) – no flicker, the stars stay.

## Features

- **Setup portal:** Wi-Fi `ESPressWeather-XXXX` (password `espressweather`), Wi-Fi scan, everything stored on the device (NVS)
- **OpenWeather:** API key (never sent back to the browser), city, country, language (de / en, also the display language),
  units (°C / °F), time zone; **query interval 2–60 minutes** (default 5), forecast every 2 h
- **API protection:** growing pause after errors (1, 2, 4 … 15 min; 1 h for an invalid key or "too many calls"),
  "refresh now" at most once a minute, hard limit of 1000 calls per device and day; calls of today are shown in the dashboard
- **Energy view:** the display *is* the MQTT broker – in Tasmota set *MQTT → Host* to the IP of the display
  (optional user / password). Only changed numbers are redrawn
- **Web dashboard** at `http://espressweather.local/`: weather, forecast, energy, Wi-Fi, firmware. The page reloads itself after a firmware update
- **Display settings live** (no restart): view, change interval, brightness, auto light (CYD light sensor), rotation 0/90/180/270° (CYD),
  sky preview day / dusk / night for 60 s
- **Touch:** tap = light on/off, double tap = next view. **CYD:** hold BOOT 5 s → setup portal. **NM-TV:** hold the touch pad 10 s → setup portal
  (countdown on the display). **MiniTV C2:** no button – portal on first start or if Wi-Fi is not reachable
- **Wi-Fi watchdog:** reconnects automatically, restarts after 5 min without Wi-Fi; portal after 30 s without Wi-Fi at start
- Start with full brightness, start screen with boot log, IP address and Tux for 5 s
- **MiniTV C2 (single core):** the weather download runs in its own task, so web page, OTA and clock keep running
- Optional: everything from `secrets.ini` instead of the portal (see below)
- Only **free content**: own vector weather icons, free fonts and Tux with licence files (see *Credits*)

## First-time setup

1. Connect to the Wi-Fi **`ESPressWeather-XXXX`** (password `espressweather`). The page opens automatically,
   otherwise go to `http://192.168.4.1/`.
2. Enter your Wi-Fi (📶 Scan), your **OpenWeather API key** (free at [openweathermap.org](https://openweathermap.org)) and your city.
3. **Save & restart.** Dashboard: `http://<hostname>.local/` – the IP address is also shown on the start screen.

### Alternative: `secrets.ini` instead of the portal

Copy `secrets.example.ini` to `secrets.ini`, fill it in and set **`use_secrets = 1`**. The firmware then takes Wi-Fi, OpenWeather,
MQTT, device name and hostname from there and starts without the portal; in the browser these settings are read-only
(display settings still work). `secrets.ini` is in `.gitignore` and never goes into the repository. Enter values without spaces.

`secrets.ini` also holds the **OTA password** and the IP addresses for the `*_ota` environments (optional).

## Build & flash (VS Code + PlatformIO)

1. Open the folder in VS Code with PlatformIO.
2. Choose the environment (`cyd_usb`, `nmtv_usb` or `c2_usb`) and click **→ Upload**. Serial monitor: 115200 baud.

On Windows build from a normal terminal (PowerShell); Git Bash is not supported by the ESP32 toolchain.

### Updates over Wi-Fi (OTA)

Every board has an `*_ota` environment (`cyd_ota`, `nmtv_ota`, `c2_ota`). Put the **IP address of your device** and an optional
OTA password into `secrets.ini` (`ota_ip_cyd`, `ota_ip_nmtv`, `ota_ip_c2`, `ota_pass`). The default address `192.168.0.100`
is only a placeholder – **change it**.

There are intentionally no prebuilt firmware files: you flash what you can read in the source.

## Debug: display image in the browser (off)

Only used during development, **not part of the normal firmware**. With `-DWX_WEB_PREVIEW=1` in `platformio.ini` the device
renders the current screen a second time into RAM (strips of 40 lines, ~26 KB while an image is requested) and serves it as a
BMP at `/api/screen`; the settings page then shows it above *Display*. The pictures in this README were made that way.
Default is `0`: the code is not built in at all – no memory, no CPU. Do not enable it on the MiniTV C2 (little RAM).

## Project layout

```
ESPressWeather32/
├── platformio.ini          Environments, optional secrets.ini, WX_WEB_PREVIEW
├── secrets.example.ini     Template (OTA, optional credentials)
├── include/
│   ├── Board.h, pins.h     Board profiles
│   ├── WxSettings.h        Settings (NVS)
│   ├── WxWebPage.h         Dashboard + setup page
│   ├── WxViews.h           Weather 1 / 2 / 3, CYD extra area
│   ├── WxDisplay.h         Start, portal, forecast, energy, OTA screens
│   ├── WxSky.h             Sky background, stars, moon phase, moonrise, sun arc
│   ├── WxIcons.h           Weather icons (vector drawing)
│   ├── WxMqtt.h            MQTT broker for Tasmota
│   ├── WxGfx.h             Drawing target (display or web preview)
│   ├── tux_icon.h          Tux + LICENSE-Tux.txt
│   └── Fonts/              Free fonts + licences
└── src/                    main.cpp, WxSettings.cpp
```

## Credits & licence

- Weather logic and layout based on the author's earlier projects *NM-TV-154-Weather-MQTT* and *ESP32-C2-Weather-MQTT-Display*;
  Weather 3 follows the author's own weather image from a home server.
- Weather data: [OpenWeather](https://openweathermap.org) (own API key required).
- Fonts: Archivo Narrow Bold (SIL Open Font License 1.1), OCR-A (public domain) – licence texts in `include/Fonts/`.
- **Tux:** Larry Ewing (lewing@isc.tamu.edu) and The GIMP; vector version Simon Budig; updated by Garrett LeSage (CC0).
  Source: [Wikimedia Commons](https://commons.wikimedia.org/wiki/File:Tux.svg), notice in `include/LICENSE-Tux.txt`.
- Weather icons: own vector drawings in the code, no image files.
- MQTT broker: [PicoMQTT](https://github.com/mlesniew/PicoMQTT). Display: Adafruit GFX / ST7789. JSON: ArduinoJson.
- Developed with the assistance of Claude.

MIT – see [LICENSE](LICENSE).
