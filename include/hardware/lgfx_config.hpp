#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

#include "config.h"

#if defined(PLANE_RADAR_BOARD_WROVER_GC9B72)

/**
 * LovyanGFX device: GC9B72 (2.1" round, 360x360) on SPI, ESP32-WROVER.
 * Panel_GC9B72 needs LovyanGFX >= 1.2.28 (see platformio.ini). Pin values
 * come from config.h.
 */
class LGFX : public lgfx::LGFX_Device {
  lgfx::Bus_SPI _bus;
  lgfx::Panel_GC9B72 _panel;
  lgfx::Light_PWM _light;

public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.freq_write = config::kDisplaySpiWriteHz;
      cfg.pin_sclk = static_cast<int>(config::kDisplayPinSclk);
      cfg.pin_mosi = static_cast<int>(config::kDisplayPinMosi);
      cfg.pin_miso = -1;
      cfg.pin_dc = static_cast<int>(config::kDisplayPinDc);
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = static_cast<int>(config::kDisplayPinCs);
      cfg.pin_rst = static_cast<int>(config::kDisplayPinRst);
      cfg.invert = config::kDisplayInvert;
      cfg.rgb_order = config::kDisplayRgbOrder;
      _panel.config(cfg);
    }
    {
      // GC9B72 modules need BL driven explicitly (screen stays black
      // otherwise). Routing it through Light_PWM makes the existing
      // tft.setBrightness(255) call in displayInit() actually do something,
      // instead of the no-op it was on the GC9A01 build (BL tied to VCC).
      auto cfg = _light.config();
      cfg.pin_bl = static_cast<int>(config::kDisplayPinBl);
      cfg.invert = false;
      cfg.freq = 44100;
      cfg.pwm_channel = 7;
      _light.config(cfg);
      _panel.setLight(&_light);
    }
    setPanel(&_panel);
  }
};

#else

/** LovyanGFX device: GC9A01 on SPI. Pin values come from config.h. */
class LGFX : public lgfx::LGFX_Device {
  lgfx::Bus_SPI _bus;
  lgfx::Panel_GC9A01 _panel;

public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.freq_write = config::kDisplaySpiWriteHz;
      cfg.pin_sclk = static_cast<int>(config::kDisplayPinSclk);
      cfg.pin_mosi = static_cast<int>(config::kDisplayPinMosi);
      cfg.pin_miso = -1;
      cfg.pin_dc = static_cast<int>(config::kDisplayPinDc);
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = static_cast<int>(config::kDisplayPinCs);
      cfg.pin_rst = static_cast<int>(config::kDisplayPinRst);
      cfg.invert = config::kDisplayInvert;
      cfg.rgb_order = config::kDisplayRgbOrder;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

#endif  // PLANE_RADAR_BOARD_WROVER_GC9B72
