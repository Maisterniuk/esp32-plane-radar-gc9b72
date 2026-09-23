/**
 * Standalone GC9B72 360x360 display bring-up test.
 *
 * No WiFi, no ADS-B, no PSRAM required — just wiring, init, backlight and
 * colors. Use this to check the display on whatever ESP32 you have lying
 * around (WROOM is fine) before the WROVER arrives, using the exact same
 * pins/panel config the real firmware will use.
 *
 * Build + flash:
 *   pio run -e wroom-gc9b72-test -t upload -t monitor
 */
#include <Arduino.h>

#include "config.h"
#include "hardware/display.h"
#include "hardware/display_font.h"

namespace {

constexpr uint16_t kColors[] = {
    0xF800,  // red
    0x07E0,  // green
    0x001F,  // blue
    0x0000,  // black
};
constexpr size_t kColorCount = sizeof(kColors) / sizeof(kColors[0]);
size_t s_color_index = 0;

}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("GC9B72 360x360 color test — no WiFi, no PSRAM needed");
  Serial.printf("Panel: %dx%d, SPI %lu Hz, invert=%d, rgb_order(BGR)=%d\n",
                config::kDisplayWidth, config::kDisplayHeight,
                static_cast<unsigned long>(config::kDisplaySpiWriteHz),
                config::kDisplayInvert, config::kDisplayRgbOrder);

  displayInit();  // tft.init() + setRotation(0) + setBrightness(255) + font

  const int w = config::kDisplayWidth;
  const int h = config::kDisplayHeight;

  tft.fillScreen(0x001F);                              // blue background
  tft.fillRect(w / 3, h / 3, w / 3, h / 3, 0xFFFF);     // white center square
  tft.fillRect(10, 10, 50, 50, 0xF800);                 // red, top-left
  tft.fillRect(w - 60, h - 60, 50, 50, 0x07E0);         // green, bottom-right

  tft.setTextDatum(textdatum_t::middle_center);
  tft.setTextColor(0xFFFF, 0x001F);
  if (displayFontIsSmooth()) {
    displayFontSetSmoothSize(tft, 1.2f);
  }
  tft.drawString("GC9B72 360x360", w / 2, h / 2 + h / 6);

  Serial.println("If you see: blue screen, white center square, red square");
  Serial.println("top-left, green square bottom-right, text below center —");
  Serial.println("wiring, backlight and panel init are all correct.");
  Serial.println();
  Serial.println("Squares in the wrong corners = rotation/MADCTL issue.");
  Serial.println("Colors swapped (e.g. red<->blue) = flip kDisplayRgbOrder.");
  Serial.println("Colors look like a photo negative = flip kDisplayInvert.");
  Serial.println();
  Serial.println("Now cycling full-screen red/green/blue/black every 2s —");
  Serial.println("watch for speckle/noise (SPI too fast for your wiring).");
}

void loop() {
  tft.fillScreen(kColors[s_color_index]);
  s_color_index = (s_color_index + 1) % kColorCount;
  delay(2000);
}
