/**
 * Plane Radar UI simulation — the exact same grid/colors/aircraft-drawing
 * code as the real app (ui/radar_display.cpp), fed fabricated moving
 * "aircraft" instead of a live adsb.fi fetch. No WiFi, no WiFiManager
 * portal, and no PSRAM required — just visual proof that layout, colors,
 * and motion look right on real GC9B72 hardware before wiring up ADS-B.
 *
 * BOOT button cycles range presets (5/10/15/25 km), same as production.
 *
 * Build + flash:
 *   pio run -e radar-sim-gc9b72 -t upload -t monitor
 */
#include <Arduino.h>

#include <cmath>
#include <cstring>

#include "config.h"
#include "hardware/display.h"
#include "services/adsb_client.h"
#include "services/radar_location.h"
#include "ui/radar_display.h"
#include "ui/radar_range.h"

// --- Fake services::adsb backend (no network, no HTTP/JSON) -------------
// This build excludes the real services/adsb_client.cpp (see platformio.ini
// src_filter) and provides its own definitions of the same interface.
namespace services::adsb {

namespace {

constexpr size_t kFakeCount = 6;
Aircraft s_planes[kFakeCount];
PollFn s_poll_fn = nullptr;

struct Seed {
  float radius_km;
  float bearing_deg;
  float track_deg;
  float gs_knots;
  const char* callsign;
  const char* type;
  const char* alt;
};

// Spawn points/headings chosen to sweep across the visible ring — a couple
// start beyond it, to exercise the rim bearing-cue dots too.
constexpr Seed kSeeds[kFakeCount] = {
    {8.0f, 45.0f, 225.0f, 250.0f, "TEST01", "A320", "FL120"},
    {6.0f, 200.0f, 20.0f, 180.0f, "TEST02", "B738", "FL080"},
    {3.0f, 300.0f, 120.0f, 90.0f, "TEST03", "C172", "3500"},
    {15.0f, 90.0f, 270.0f, 420.0f, "TEST04", "A388", "FL350"},
    {1.5f, 0.0f, 180.0f, 60.0f, "TEST05", "R44", "1200"},
    {20.0f, 160.0f, 340.0f, 300.0f, "TEST06", "E190", "FL240"},
};

constexpr float kDegToRad = 0.01745329252f;
constexpr float kKmPerDeg = 111.32f;

void seedPlane(Aircraft& a, const Seed& s) {
  const double lat0 = services::location::lat();
  const double lon0 = services::location::lon();
  const float br = s.bearing_deg * kDegToRad;
  const float dlat_km = s.radius_km * std::cos(br);
  const float dlon_km = s.radius_km * std::sin(br);
  const float lat0f = static_cast<float>(lat0);
  a.lat = lat0f + dlat_km / kKmPerDeg;
  a.lon = static_cast<float>(lon0) + dlon_km / (kKmPerDeg * std::cos(lat0f * kDegToRad));
  a.track_deg = s.track_deg;
  a.nose_deg = s.track_deg;
  a.gs_knots = s.gs_knots;
  strncpy(a.callsign, s.callsign, sizeof(a.callsign) - 1);
  a.callsign[sizeof(a.callsign) - 1] = '\0';
  strncpy(a.type, s.type, sizeof(a.type) - 1);
  a.type[sizeof(a.type) - 1] = '\0';
  strncpy(a.alt, s.alt, sizeof(a.alt) - 1);
  a.alt[sizeof(a.alt) - 1] = '\0';
}

}  // namespace

size_t aircraftCount() { return kFakeCount; }
const Aircraft* aircraftList() { return s_planes; }
void setPollFn(PollFn fn) { s_poll_fn = fn; }

bool fetchUpdate(double, double, float) {
  return true;  // never called by the simulation loop below
}

void simInit() {
  for (size_t i = 0; i < kFakeCount; ++i) {
    seedPlane(s_planes[i], kSeeds[i]);
  }
}

/** Advance every plane along its track by elapsed seconds; respawn at its
 *  seed point once it drifts well past the widest range preset. */
void simStep(float dt_sec) {
  const float lat0f = static_cast<float>(services::location::lat());
  const float lon0f = static_cast<float>(services::location::lon());
  for (size_t i = 0; i < kFakeCount; ++i) {
    Aircraft& a = s_planes[i];
    const float speed_km_s = a.gs_knots * 1.852f / 3600.0f;
    const float dist_km = speed_km_s * dt_sec;
    const float tr = a.track_deg * kDegToRad;
    a.lat += (dist_km * std::cos(tr)) / kKmPerDeg;
    a.lon += (dist_km * std::sin(tr)) / (kKmPerDeg * std::cos(lat0f * kDegToRad));

    const float dlat = (a.lat - lat0f) * kKmPerDeg;
    const float dlon = (a.lon - lon0f) * kKmPerDeg * std::cos(lat0f * kDegToRad);
    if (std::sqrt(dlat * dlat + dlon * dlon) > 40.0f) {
      seedPlane(a, kSeeds[i]);
    }
  }
}

}  // namespace services::adsb

// --- Simulation main ------------------------------------------------------
namespace {
unsigned long s_last_step_ms = 0;
unsigned long s_last_boot_ms = 0;
}  // namespace

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("Plane Radar UI simulation — fake traffic, no WiFi/PSRAM");

  pinMode(config::kBootPin, INPUT_PULLUP);

  displayInit();
  services::location::init();
  ui::radar::rangeInit();
  services::adsb::simInit();

  ui::radarDisplayDraw();
  ui::radarDisplayRefreshAircraft();
  s_last_step_ms = millis();

  Serial.println("Hold BOOT to cycle range presets (5/10/15/25 km).");
}

void loop() {
  const unsigned long now = millis();

  if (digitalRead(config::kBootPin) == LOW) {
    if (now - s_last_boot_ms > 400) {
      s_last_boot_ms = now;
      ui::radar::rangeNext();
      ui::radarDisplayDraw();
    }
  }

  // Slower than production's 3s ADS-B poll on purpose: without PSRAM this
  // falls back to a full-screen redraw every tick (no sprite to blit), so a
  // longer interval means less flicker. The real WROVER target double-
  // buffers and won't flicker regardless of interval.
  if (now - s_last_step_ms >= 800) {
    const float dt = (now - s_last_step_ms) / 1000.0f;
    s_last_step_ms = now;
    services::adsb::simStep(dt);
    ui::radarDisplayRefreshAircraft();
  }
}
