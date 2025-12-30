#pragma once
#include <LovyanGFX.hpp>

#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <driver/i2c.h>

class LGFX : public lgfx::LGFX_Device
{
public:

  lgfx::Bus_RGB     _bus_instance;
  lgfx::Panel_ST7701   _panel_instance;
  //lgfx::Panel_RGB   _panel_instance;
  lgfx::Light_PWM   _light_instance;
  lgfx::Touch_GT911 _touch_instance;

  LGFX(void)
  {
    {
      auto cfg = _panel_instance.config();

      cfg.memory_width  = 480;
      cfg.memory_height = 480;
      cfg.panel_width  = 480;
      cfg.panel_height = 480;

      cfg.offset_x = 0;
      cfg.offset_y = 0;

      _panel_instance.config(cfg);
    }

    {
      auto cfg = _panel_instance.config_detail();

      cfg.pin_cs = 42;
      cfg.pin_sclk = 2;
      cfg.pin_mosi = 1; // SDA

      cfg.use_psram = 1;

      _panel_instance.config_detail(cfg);
    }

    {
      auto cfg = _bus_instance.config();
      cfg.panel = &_panel_instance;
      cfg.pin_d0  = GPIO_NUM_5;  // B1
      cfg.pin_d1  = GPIO_NUM_45;  // B2
      cfg.pin_d2  = GPIO_NUM_48; // B3
      cfg.pin_d3  = GPIO_NUM_47;  // B4
      cfg.pin_d4  = GPIO_NUM_21;  // B5
      cfg.pin_d5  = GPIO_NUM_14;  // G0
      cfg.pin_d6  = GPIO_NUM_13;  // G1
      cfg.pin_d7  = GPIO_NUM_12;  // G2
      cfg.pin_d8  = GPIO_NUM_11; // G3
      cfg.pin_d9  = GPIO_NUM_10; // G4
      cfg.pin_d10 = GPIO_NUM_9;  // G5
      cfg.pin_d11 = GPIO_NUM_46; // R1
      cfg.pin_d12 = GPIO_NUM_3; // R2
      cfg.pin_d13 = GPIO_NUM_8; // R3
      cfg.pin_d14 = GPIO_NUM_18; // R4
      cfg.pin_d15 = GPIO_NUM_17; // R5

      cfg.pin_henable = GPIO_NUM_40;
      cfg.pin_vsync   = GPIO_NUM_39;
      cfg.pin_hsync   = GPIO_NUM_38;
      cfg.pin_pclk    = GPIO_NUM_41;
      cfg.freq_write  = (15 * 1000 * 1000);

      cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = 45;
      cfg.hsync_pulse_width = 8;
      cfg.hsync_back_porch  = 10;
      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = 8;
      cfg.vsync_pulse_width = 2;
      cfg.vsync_back_porch  = 18;
      cfg.pclk_active_neg = 0;
      cfg.de_idle_high = 1;
      //cfg.pclk_idle_high = 0;
    /* working kind of
     cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = 50;
      cfg.hsync_pulse_width = 8;
      cfg.hsync_back_porch  = 10;
      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = 8;
      cfg.vsync_pulse_width = 2;
      cfg.vsync_back_porch  = 18;
      cfg.pclk_active_neg = 0;
      cfg.de_idle_high = 1;
      */
      _bus_instance.config(cfg);
    }
    _panel_instance.setBus(&_bus_instance);

    {
      auto cfg = _light_instance.config();
      cfg.pin_bl = GPIO_NUM_6;
      _light_instance.config(cfg);
    }
    _panel_instance.light(&_light_instance);

    {
      auto cfg = _touch_instance.config();
      cfg.x_min      = 0;
      cfg.y_min      = 0;
      cfg.bus_shared = false;
      cfg.offset_rotation = 0;
      // I2C
      cfg.i2c_port   = I2C_NUM_1;
      cfg.pin_sda    = GPIO_NUM_15;
      cfg.pin_scl    = GPIO_NUM_7;

// for Board v1.3
      cfg.pin_int    = GPIO_NUM_16;
      cfg.pin_rst    = GPIO_NUM_NC;
      cfg.x_max      = 480;
      cfg.y_max      = 480;
/*
// for Board v1.1 / v1.2
      cfg.pin_int    = GPIO_NUM_38;
      cfg.pin_rst    = GPIO_NUM_NC;
      cfg.x_max      = 480;
      cfg.y_max      = 272;
//*/
      cfg.freq       = 400000;
      _touch_instance.config(cfg);
      _panel_instance.setTouch(&_touch_instance);
    }

    setPanel(&_panel_instance);
  }
};