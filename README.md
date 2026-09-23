# Plane Radar

<img width="800" height="450" alt="plane-radar" src="https://github.com/user-attachments/assets/716d0992-dab8-47ba-8f1a-2aec7f607419" />

**3D printed case (STL + assembly):** [MakerWorld](https://makerworld.com/en/models/2872376-esp32-plane-radar-live-ads-b-on-a-round-display#profileId-3207083) · **Firmware:** [Releases](https://github.com/MatixYo/ESP32-Plane-Radar/releases)

Firmware for an **ESP32-C3 Super Mini** and a **1.28″ round GC9A01** display (240×240). Shows a circular **ADS-B radar** around your configured location, with **WiFiManager** for first-time setup.

> **This fork adds a second hardware target: ESP32-WROVER + a 2.1″ round
> GC9B72 display (360×360).** Everything below documents the original
> **`supermini`** (C3 + GC9A01) build; see
> [**ESP32-WROVER + GC9B72 (360×360) variant**](#esp32-wrover--gc9b72-360×360-variant)
> below for what's different on the bigger screen. All credit for the core
> project (this README included) goes to
> [MatixYo/ESP32-Plane-Radar](https://github.com/MatixYo/ESP32-Plane-Radar) —
> this fork only adds the alternate display/board support, a live Wi‑Fi
> settings page for the range preset, and origin→destination tags.

## What it does

1. **Wi‑Fi setup** (if needed) — captive portal on AP **`PlaneRadar-Setup`**
2. **Radar** — live aircraft from [adsb.fi](https://opendata.adsb.fi/) on a sonar-style grid

After Wi‑Fi is saved, the device reconnects automatically; the radar runs in the main loop with periodic ADS-B updates (~5 s).

## Controls (BOOT, GPIO 9, active LOW)

| Action | Effect |
|--------|--------|
| **Short tap** | Cycle range preset (5 → 10 → 15 → 25 km); saved to flash |
| **Hold 3 s** | Clear Wi‑Fi, location, and units; reboot into setup portal |

During setup you can also hold BOOT at power-on to force a credential reset (same as the long press).

## Wi‑Fi setup portal

**First-time setup** (no saved Wi‑Fi):

1. Connect to **`PlaneRadar-Setup`**
2. Open **`http://plane-radar.local`** (preferred) or **`http://192.168.4.1`** — both are shown on the yellow setup screen; captive portal may open automatically
3. Set home Wi‑Fi, then save

**Reconfigure anytime** (after the device is on your network):

1. Open **`http://plane-radar.local`** or **`http://<device-ip>`** (e.g. from your router or serial log at boot)
2. Change Wi‑Fi, location, units, or runway overlay; save

The same portal runs on the setup AP and on the device’s LAN IP while connected to Wi‑Fi. mDNS hostname is `plane-radar` → **plane-radar.local** (`kPortalHostname` in `config.h`). Some clients resolve `.local` slowly; use the IP if needed.

**Custom fields** (stored in NVS):

| Field | Purpose |
|-------|---------|
| **Latitude / Longitude** | Radar center and ADS-B query position (defaults in `config.h` until set) |
| **Display distances in miles** | Ring scale label in **mi** instead of **km** (e.g. `6mi` vs `10km`) |
| **Show airport runways** | Major-airport runway overlay on the radar (off to hide) |

After a reset, the device reboots and shows the setup screen immediately (no “Connecting” loop on stale credentials).

## Radar display

### Grid

- Dark blue background, subdued green rings and crosshairs
- White **N / S / E / W** at the bezel; range label on the **east** spoke (ring 3 = ¾ of outer radius)
- White center dot

Layout and colors: `include/ui/radar_theme.h`.

### Range presets

| Ring 3 label | Outer radius (aircraft scale) |
|------------|-------------------------------|
| 5 km / 3 mi | ~6.7 km |
| 10 km / 6 mi | ~13.3 km (default) |
| 15 km / 9 mi | ~20 km |
| 25 km / 16 mi | ~33.3 km |

Preset and miles/km choice persist across reboot (`planeradar` NVS namespace).

### Runways

- Major airports from OurAirports (`large_airport`); all open runway strips in range (helipads excluded)
- Teal runway lines with one ICAO label per airport (e.g. `KJFK`); toggle in the Wi‑Fi setup portal
- Update the embedded list: `python3 scripts/build_large_airports.py`

### Aircraft

- **Inside the outer ring** — red heading triangle, magenta speed vector (clipped at the ring), callsign / type / altitude tags
- **Outside the ring** (still within ADS-B fetch) — small **red dot on the screen rim** at the correct bearing (direction cue; not distance-accurate past the ring)
- **Tags** — placed toward the **center**: west (left) → tag on the **right** of the symbol; east (right) → tag on the **left**

As range decreases (or aircraft approach), targets move inward; beyond-ring dots become full symbols when they cross the outer ring.

### ADS-B

- Source: `https://opendata.adsb.fi/api/v3/`
- Fetch radius: `ui::radar::fetchRadiusKm()` — scales with the active preset to roughly the screen edge (so rim dots have data)
- Poll interval: `kAdsbFetchIntervalMs` (5 s) in `config.h`
- Ground aircraft hidden by default (`kAdsbShowGroundAircraft`)

## Configuration

Edit **`include/config.h`** for hardware and behavior:

| Area | Keys / notes |
|------|----------------|
| Portal | `kPortalApName`, `kPortalIp`, `kPortalHostname` / `kPortalHostUrl` (mDNS; needs `-DWM_MDNS` in `platformio.ini`) |
| Wi‑Fi timing | connect attempts, reconnect grace, portal timeout (`0` = no timeout) |
| BOOT | `kBootPin`, `kBootResetHoldMs`, `kBootTapMinMs` |
| Display SPI | pins, `kDisplayInvert`, `kDisplayRgbOrder`, `kDisplaySpiWriteHz` |
| Default location | `kDefaultRadarLat`, `kDefaultRadarLon` (until portal overrides) |
| ADS-B | `kAdsbFetchIntervalMs`, `kAdsbShowGroundAircraft` |

Range presets: `include/ui/radar_range.h` (`kRangePresets`).

## Project layout

```
include/
  config.h
  hardware/
    lgfx_config.hpp
    display.h
    display_font.h
  data/
    large_airports.h
  ui/
    radar_theme.h
    radar_range.h
    radar_display.h
    runway_overlay.h
    status_screens.h
  services/
    wifi_setup.h
    radar_location.h
    adsb_client.h
data/
  ui_font.vlw              — embedded smooth UI font (Noto Sans Bold)
scripts/
  build_large_airports.py
src/
  main.cpp
  data/
    large_airports_data.cpp
  hardware/
  ui/
  services/
```

## Wiring (GC9A01 ↔ ESP32-C3 Super Mini)

| Display | ESP32-C3 |
|---------|----------|
| VCC | 3V3 |
| GND | GND |
| RST | GPIO **0** |
| CS | GPIO **1** |
| DC | GPIO **10** |
| SDA (MOSI) | GPIO **3** |
| SCL (SCLK) | GPIO **4** |
| BOOT (user) | GPIO **9** |

## Build

```bash
pio run -t upload
pio device monitor
```

- PlatformIO env: **`supermini`**
- Serial: **115200** baud
- USB CDC on boot enabled in `platformio.ini` for the Super Mini

### Web-flashable release image

Single `.bin` for [esptool-js](https://espressif.github.io/esptool-js/) and similar tools (ESP32-C3, 4 MB, flash at **0x0**):

```bash
chmod +x scripts/merge-firmware.sh   # once
./scripts/merge-firmware.sh
```

Writes `release/plane-radar-merged.bin`. Skip rebuild if firmware is already built:

```bash
./scripts/merge-firmware.sh --no-build
```

Or via PlatformIO only (output: `.pio/build/supermini/firmware-merged.bin`):

```bash
pio run -e supermini
pio run -t merge -e supermini
```

Put the board in download mode (hold **BOOT**, tap **RESET**), then flash with Chrome/Edge over USB.

### CI and releases (GitHub Actions)

| Workflow | When | Output |
|----------|------|--------|
| [Build](.github/workflows/build.yml) | Push / PR to `main` | Artifact `plane-radar-supermini` (merged + split `.bin` files, ~90 days) |
| [Release](.github/workflows/release.yml) | Git tag `v*` (e.g. `v1.0.0`) | GitHub Release asset `plane-radar-v1.0.0.bin` + `.sha256` |

To ship a version users can download:

```bash
git tag v1.0.0
git push origin v1.0.0
```

The release workflow builds firmware in CI and attaches the merged image to the release. Download from **Releases** on GitHub, then flash at **0x0** (ESP32-C3, 4 MB).

## ESP32-WROVER + GC9B72 (360×360) variant

A second PlatformIO target, **`wrover-gc9b72`**, runs the same radar on an
**ESP32-WROVER** (needs real PSRAM — see below) driving a **2.1″ round
GC9B72** panel (360×360). Selected via the `PLANE_RADAR_DISPLAY_GC9B72`
build flag, which swaps in a different pin map, panel driver, and
360px-scaled UI geometry — the C3/GC9A01 `supermini` target is untouched
and still builds/works exactly as documented above.

```bash
pio run -e wrover-gc9b72 -t upload -t monitor
```

### Why PSRAM is required

The double-buffered frame sprite at 360×360×16bpp is **~253 KB** — too big
for a plain ESP32's internal DRAM alongside the Wi‑Fi stack. Panel_GC9B72
itself (LovyanGFX ≥ **1.2.28**, [PR #898](https://github.com/lovyan03/LovyanGFX/pull/898))
doesn't need PSRAM, but *this project's* double-buffering does. **Use a
genuine WROVER** (PSRAM populated) — not every board sold as "WROVER" on
marketplaces actually has it; see `src/test_psram_check.cpp` below. If the
sprite can't be allocated, the app still runs — it falls back to drawing
straight to the panel every frame (visible flicker, no crash).

### Wiring (GC9B72 ↔ ESP32-WROVER)

| GC9B72 | WROVER | Notes |
|--------|--------|-------|
| VCC | 3V3 | **3.3V only** |
| GND | GND | |
| SCL | GPIO **18** | VSPI SCLK |
| SDA | GPIO **23** | VSPI MOSI |
| CS | GPIO **5** | |
| DC | GPIO **27** | |
| RST | GPIO **33** | |
| BL | GPIO **32** | GC9B72 needs backlight driven explicitly (unlike most GC9A01 breakouts, which tie BL to VCC) |
| SDO, TE | — | not connected |

BOOT button: physical **GPIO0** pushbutton already on every ESP32 dev
board — no extra wiring, same reuse-the-BOOT-button idea as the C3 build.

### Bring-up / diagnostic firmware

Three extra, minimal PlatformIO envs — useful when wiring up a new board,
independent of the full radar app (no Wi‑Fi credentials needed):

| Env | What it does | Needs PSRAM? |
|-----|---------------|:---:|
| `wroom-gc9b72-test` | Fills the screen with test colors/shapes, then cycles red/green/blue/black — checks wiring, backlight, orientation, `kDisplayInvert`/`kDisplayRgbOrder` | No |
| `psram-check` | Prints `ESP.getPsramSize()` / `psramFound()` once over serial — the ground-truth way to tell a real WROVER from a mislabeled WROOM | No |
| `radar-sim-gc9b72` / `radar-sim-gc9b72-psram` | Full radar UI (real grid/colors/aircraft rendering) fed fabricated moving traffic instead of a live adsb.fi fetch — no Wi‑Fi. The `-psram` variant enables PSRAM so you can visually confirm smooth, flicker-free double-buffering before wiring up ADS-B | No / Yes |

```bash
pio run -e wroom-gc9b72-test -t upload -t monitor
pio run -e psram-check -t upload -t monitor
pio run -e radar-sim-gc9b72-psram -t upload -t monitor
```

### Range preset from the Wi‑Fi portal

The same live portal described above (`plane-radar.local`, no Wi‑Fi reset
needed) now also has a **Default range** dropdown (5/10/15/25 km) —
previously the range preset could only be changed with BOOT taps on the
device itself. The portal's fields refresh once a second while the page is
open, so they stay in sync even if you also cycle the range with BOOT.

### Origin → Destination tags

Aircraft tags can show a 4th line, e.g. `AMS>LHR`, below callsign/type/
altitude. ADS-B itself carries no flight-plan data, so this comes from a
free, keyless lookup — [api.adsbdb.com](https://github.com/mrjackwills/adsbdb)
— by callsign, throttled to roughly one new request every 2 seconds and
cached for the session (`services/route_lookup.*`). Most scheduled airline
traffic gets a route; GA/military/charter usually won't (adsbdb has no
published route for them) — the 4th line just doesn't appear, no error.

## Dependencies

- [LovyanGFX](https://github.com/lovyan03/LovyanGFX)
- [WiFiManager](https://github.com/tzapu/WiFiManager)
- [ArduinoJson](https://github.com/bblanchon/ArduinoJson)
- [adsbdb](https://github.com/mrjackwills/adsbdb) — free callsign→route lookup (WROVER/GC9B72 variant only)
