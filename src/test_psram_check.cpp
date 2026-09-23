/**
 * One-shot hardware check: is PSRAM actually present and working on this
 * board? No display, no WiFi — just ESP.getPsramSize()/psramFound(),
 * printed once over serial. Use this to tell a genuine WROVER apart from
 * a WROOM/plain module before trusting a silkscreen printout.
 *
 * Build + flash:
 *   pio run -e psram-check -t upload -t monitor
 */
#include <Arduino.h>
#include <esp_heap_caps.h>

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println();
  Serial.println("=== PSRAM check ===");
  Serial.printf("Chip model: %s, rev %d, cores: %d\n", ESP.getChipModel(),
                ESP.getChipRevision(), ESP.getChipCores());
  Serial.printf("Flash size: %u bytes\n", ESP.getFlashChipSize());
  Serial.printf("psramFound(): %s\n", psramFound() ? "YES" : "NO");
  Serial.printf("ESP.getPsramSize(): %u bytes\n", ESP.getPsramSize());
  Serial.printf("ESP.getFreePsram(): %u bytes\n", ESP.getFreePsram());
  Serial.printf("heap_caps_get_free_size(MALLOC_CAP_SPIRAM): %u bytes\n",
                static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM)));
  Serial.println();
  if (psramFound() && ESP.getPsramSize() > 0) {
    Serial.println("RESULT: This is a real PSRAM-equipped module (WROVER-class).");
  } else {
    Serial.println("RESULT: No usable PSRAM detected — this is a WROOM-class module.");
  }
}

void loop() { delay(1000); }
