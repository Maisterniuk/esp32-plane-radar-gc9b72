#pragma once

#include <cstdint>

#include <driver/gpio.h>

namespace config {

// --- Wi-Fi portal ---
constexpr char kPortalApName[] = "PlaneRadar-Setup";
constexpr char kPortalIp[] = "192.168.4.1";
/** mDNS host (no ".local" suffix); browser: http://plane-radar.local */
constexpr char kPortalHostname[] = "plane-radar";
constexpr char kPortalHostUrl[] = "plane-radar.local";

/** Per-attempt STA connect wait (ms); retried kWifiConnectAttempts times. */
constexpr unsigned long kWifiConnectAttemptMs = 15000;
constexpr uint8_t kWifiConnectAttempts = 3;
constexpr unsigned long kWifiPortalTimeoutSec = 0;  // 0 = no timeout while configuring
constexpr unsigned long kWifiConnectingFrameMs = 50;
/** Wait after disconnect before reconnecting (avoids portal on brief drops). */
constexpr unsigned long kWifiDownGraceMs = 4000;
/** Minimum interval between background reconnect tries. */
constexpr unsigned long kWifiReconnectIntervalMs = 15000;

#if defined(PLANE_RADAR_DISPLAY_GC9B72)

// --- BOOT button (ESP32-WROVER dev board, active LOW) ---
// GPIO0 is the physical "BOOT" pushbutton on virtually every ESP32 dev
// board (used to enter the bootloader at power-on). After boot it's an
// ordinary input — same reuse-the-BOOT-button idea as the C3 build below.
constexpr gpio_num_t kBootPin = GPIO_NUM_0;
constexpr unsigned long kBootResetHoldMs = 3000UL;
/** Ignore BOOT taps shorter than this (debounce). */
constexpr unsigned long kBootTapMinMs = 40UL;

// --- Display: GC9B72 2.1" round 360×360 (SPI) ---
// Pins chosen to avoid: GPIO6-11 (internal flash), GPIO16/17 (PSRAM on
// original WROVER modules — reserved even though newer WROVER-B/E/IE
// variants don't need them there), GPIO34-39 (input-only), and the other
// strapping pins (2/12/15). Rewire freely if your board breaks these out
// differently; nothing here is register-critical.
constexpr gpio_num_t kDisplayPinRst = GPIO_NUM_33;
constexpr gpio_num_t kDisplayPinCs = GPIO_NUM_5;   // VSPI native CS0
constexpr gpio_num_t kDisplayPinDc = GPIO_NUM_27;
constexpr gpio_num_t kDisplayPinMosi = GPIO_NUM_23;  // VSPI native MOSI / display SDA
constexpr gpio_num_t kDisplayPinSclk = GPIO_NUM_18;  // VSPI native SCLK / display SCL
/** GC9B72 modules need the backlight driven HIGH explicitly (unlike most
 *  GC9A01 breakouts, which tie BL to VCC) — see hardware/lgfx_config.hpp. */
constexpr gpio_num_t kDisplayPinBl = GPIO_NUM_32;

constexpr int kDisplayWidth = 360;
constexpr int kDisplayHeight = 360;

// GC9B72 panels are commonly reported to speckle above ~20 MHz on
// breadboard/long wiring; raise back towards 40 MHz once yours is soldered
// down and you've confirmed it's clean.
constexpr uint32_t kDisplaySpiWriteHz = 20000000;
// TUNE ON FIRST BOOT: LovyanGFX's Panel_GC9B72 init already sets MADCTL to
// RGB order with no inversion, so these start "off" (unlike the GC9A01
// block below, which needed both flipped). If colors come out inverted or
// red/blue-swapped on your specific module, flip these two.
constexpr bool kDisplayInvert = false;
constexpr bool kDisplayRgbOrder = false;

#else

// --- BOOT button (ESP32-C3 Super Mini, active LOW) ---
constexpr gpio_num_t kBootPin = GPIO_NUM_9;
constexpr unsigned long kBootResetHoldMs = 3000UL;
/** Ignore BOOT taps shorter than this (debounce). */
constexpr unsigned long kBootTapMinMs = 40UL;

// --- Display: GC9A01 1.28" round 240×240 (SPI) ---
constexpr gpio_num_t kDisplayPinRst = GPIO_NUM_0;
constexpr gpio_num_t kDisplayPinCs = GPIO_NUM_1;
constexpr gpio_num_t kDisplayPinDc = GPIO_NUM_10;
constexpr gpio_num_t kDisplayPinMosi = GPIO_NUM_3;  // display SDA
constexpr gpio_num_t kDisplayPinSclk = GPIO_NUM_4;  // display SCL
/** No separate backlight pin — this GC9A01 breakout ties BL to VCC. */
constexpr gpio_num_t kDisplayPinBl = GPIO_NUM_NC;

constexpr int kDisplayWidth = 240;
constexpr int kDisplayHeight = 240;

constexpr uint32_t kDisplaySpiWriteHz = 40000000;
// GC9A01 modules often need invert + BGR for correct black/green output
constexpr bool kDisplayInvert = true;
constexpr bool kDisplayRgbOrder = true;

#endif  // PLANE_RADAR_DISPLAY_GC9B72

// --- Radar center defaults (overridden via WiFi setup portal) ---
constexpr double kDefaultRadarLat = 52.3676;
constexpr double kDefaultRadarLon = 4.9041;

/** Poll adsb.fi (API public limit: 1 req/s). */
constexpr unsigned long kAdsbFetchIntervalMs = 3000;
/** Legacy scale unused — fetch uses radar::fetchRadiusKm() to screen edge. */
constexpr float kAdsbFetchRadiusScale = 1.0f;
/** false = hide aircraft with alt_baro "ground"; true = show them too. */
constexpr bool kAdsbShowGroundAircraft = false;

// --- UI colors (RGB565) — status screens ---
constexpr uint16_t kColorBlack = 0x0000;
constexpr uint16_t kColorYellow = 0xFFE0;
constexpr uint16_t kTextOnYellow = kColorBlack;
constexpr uint16_t kTextOnBlack = 0xFFFF;

}  // namespace config
