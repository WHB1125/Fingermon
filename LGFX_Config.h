#define LGFX_USE_V1
#include <LovyanGFX.hpp>

class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_ST7789  _panel_instance;
  lgfx::Bus_SPI       _bus_instance;
public:
  LGFX(void) {
    {
      auto cfg = _bus_instance.config();
      cfg.pin_mosi = 15;
      cfg.pin_miso = 14; // 不使用
      cfg.pin_sclk = 13;
      cfg.pin_dc   = 23;
      _bus_instance.config(cfg);
      _panel_instance.setBus(&_bus_instance);
    }
    {
      auto cfg = _panel_instance.config();
      cfg.pin_cs           =  5;
      cfg.pin_rst          = 18;
      cfg.pin_busy         = -1;
      cfg.panel_width      = 135;
      cfg.panel_height     = 240;
      cfg.offset_x         = 0;
      cfg.offset_y         = 0;
      _panel_instance.config(cfg);
    }
    setPanel(&_panel_instance);
  }
};

LGFX lcd;
LGFX_Sprite canvas(&lcd);