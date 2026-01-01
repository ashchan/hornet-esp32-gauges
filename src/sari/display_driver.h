#pragma once
#include <LovyanGFX.hpp>

#include <lgfx/v1/platforms/esp32s3/Panel_RGB.hpp>
#include <lgfx/v1/platforms/esp32s3/Bus_RGB.hpp>
#include <driver/i2c.h>

#include <Wire.h>

#define TCA_ADDR    0x20
#define REG_OUTPUT  0x01
#define REG_CONFIG  0x03

// EXIO mapping (bit index)
#define EXIO_RST   0   // EXIO1
#define EXIO_CS    2   // EXIO3
#define EXIO_DISP  3   // EXIO4
#define EXIO_BUZZ  7   // EXIO8

static uint8_t tca_out = 0xFF;

static void tcaWrite(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(reg);
  Wire.write(val);
  Wire.endTransmission();
}

static void tcaInit() {
  tcaWrite(REG_CONFIG, 0x00);      // all outputs
  tca_out = 0xFF;
  tcaWrite(REG_OUTPUT, tca_out);
}

static void tcaSet(uint8_t bit, bool level) {
  if (level) tca_out |=  (1u << bit);
  else       tca_out &= ~(1u << bit);
  tcaWrite(REG_OUTPUT, tca_out);
}

#define ST_SDA 1
#define ST_SCL 2

static inline void stclk() {
  digitalWrite(ST_SCL, HIGH);
  digitalWrite(ST_SCL, LOW);
}

static void stWrite9(bool isData, uint8_t v) {
  // D/C bit
  digitalWrite(ST_SDA, isData ? HIGH : LOW);
  stclk();

  // 8-bit payload
  for (int i = 7; i >= 0; --i) {
    digitalWrite(ST_SDA, (v >> i) & 1);
    stclk();
  }
}

static inline void stCmd(uint8_t c) { stWrite9(false, c); }
static inline void stDat(uint8_t d) { stWrite9(true,  d); }

static void panel_power_and_reset() {
  Wire.begin(15, 7);
  Wire.setClock(400000);
  tcaInit();

  tcaSet(EXIO_BUZZ, 0);   // buzzer off
  tcaSet(EXIO_DISP, 1);   // DISP enable
  tcaSet(EXIO_CS,   0);   // CS low (selected)

  // reset pulse
  tcaSet(EXIO_RST, 0);
  delay(20);
  tcaSet(EXIO_RST, 1);
  delay(120);
}

static void st7701_init_rgb565() {
  pinMode(ST_SDA, OUTPUT);
  pinMode(ST_SCL, OUTPUT);
  digitalWrite(ST_SCL, LOW);

  // keep expander CS low
  tcaSet(EXIO_CS, 0);

  // --- 2.8inch init table ---
  stCmd(0xFF); stDat(0x77); stDat(0x01); stDat(0x00); stDat(0x00); stDat(0x13);
  stCmd(0xEF); stDat(0x08);

  stCmd(0xFF); stDat(0x77); stDat(0x01); stDat(0x00); stDat(0x00); stDat(0x10);

  stCmd(0xC0); stDat(0x3B); stDat(0x00);
  stCmd(0xC1); stDat(0x10); stDat(0x0C);
  stCmd(0xC2); stDat(0x07); stDat(0x0A);
  stCmd(0xC7); stDat(0x00);
  stCmd(0xCC); stDat(0x10);
  stCmd(0xCD); stDat(0x08);

  stCmd(0xB0);
  stDat(0x05); stDat(0x12); stDat(0x98); stDat(0x0E); stDat(0x0F); stDat(0x07); stDat(0x07); stDat(0x09);
  stDat(0x09); stDat(0x23); stDat(0x05); stDat(0x52); stDat(0x0F); stDat(0x67); stDat(0x2C); stDat(0x11);

  stCmd(0xB1);
  stDat(0x0B); stDat(0x11); stDat(0x97); stDat(0x0C); stDat(0x12); stDat(0x06); stDat(0x06); stDat(0x08);
  stDat(0x08); stDat(0x22); stDat(0x03); stDat(0x51); stDat(0x11); stDat(0x66); stDat(0x2B); stDat(0x0F);

  stCmd(0xFF); stDat(0x77); stDat(0x01); stDat(0x00); stDat(0x00); stDat(0x11);

  stCmd(0xB0); stDat(0x5D);
  stCmd(0xB1); stDat(0x3E);
  stCmd(0xB2); stDat(0x81);
  stCmd(0xB3); stDat(0x80);
  stCmd(0xB5); stDat(0x4E);
  stCmd(0xB7); stDat(0x85);
  stCmd(0xB8); stDat(0x20);

  stCmd(0xC1); stDat(0x78);
  stCmd(0xC2); stDat(0x78);
  stCmd(0xD0); stDat(0x88);

  stCmd(0xE0); stDat(0x00); stDat(0x00); stDat(0x02);

  stCmd(0xE1);
  stDat(0x06); stDat(0x30); stDat(0x08); stDat(0x30); stDat(0x05); stDat(0x30);
  stDat(0x07); stDat(0x30); stDat(0x00); stDat(0x33); stDat(0x33);

  stCmd(0xE2);
  stDat(0x11); stDat(0x11); stDat(0x33); stDat(0x33); stDat(0xF4); stDat(0x00); stDat(0x00); stDat(0x00);
  stDat(0xF4); stDat(0x00); stDat(0x00); stDat(0x00);

  stCmd(0xE3); stDat(0x00); stDat(0x00); stDat(0x11); stDat(0x11);
  stCmd(0xE4); stDat(0x44); stDat(0x44);

  stCmd(0xE5);
  stDat(0x0D); stDat(0xF5); stDat(0x30); stDat(0xF0);
  stDat(0x0F); stDat(0xF7); stDat(0x30); stDat(0xF0);
  stDat(0x09); stDat(0xF1); stDat(0x30); stDat(0xF0);
  stDat(0x0B); stDat(0xF3); stDat(0x30); stDat(0xF0);

  stCmd(0xE6); stDat(0x00); stDat(0x00); stDat(0x11); stDat(0x11);
  stCmd(0xE7); stDat(0x44); stDat(0x44);

  stCmd(0xE8);
  stDat(0x0C); stDat(0xF4); stDat(0x30); stDat(0xF0);
  stDat(0x0E); stDat(0xF6); stDat(0x30); stDat(0xF0);
 stDat(0x08); stDat(0xF0); stDat(0x30); stDat(0xF0);
  stDat(0x0A); stDat(0xF2); stDat(0x30); stDat(0xF0);

  stCmd(0xE9); stDat(0x36); stDat(0x01);

  stCmd(0xEB);
  stDat(0x00); stDat(0x01); stDat(0xE4); stDat(0xE4); stDat(0x44); stDat(0x88); stDat(0x40);

  stCmd(0xED);
  stDat(0xFF); stDat(0x10); stDat(0xAF); stDat(0x76); stDat(0x54); stDat(0x2B); stDat(0xCF);
  stDat(0xFF); stDat(0xFF); stDat(0xFC); stDat(0xB2); stDat(0x45); stDat(0x67); stDat(0xFA); stDat(0x01); stDat(0xFF);

  stCmd(0xEF);
  stDat(0x08); stDat(0x08); stDat(0x08); stDat(0x45); stDat(0x3F); stDat(0x54);

  stCmd(0xFF); stDat(0x77); stDat(0x01); stDat(0x00); stDat(0x00); stDat(0x00);

  // select BK0
  stCmd(0xFF); stDat(0x77); stDat(0x01); stDat(0x00); stDat(0x00); stDat(0x10);

  // RGBCTRL (DPI config)
  stCmd(0xC3);
  stDat(0x0C);   // de_idle=0, hsync_pol=0, vsync_pol=0  => 0x04|0x08 = 0x0C
  stDat(0x10);
  stDat(0x08);

  stCmd(0x11);
  delay(120);

  stCmd(0x3A);
  stDat(0x66);

  stCmd(0x36);
  stDat(0x00);

  stCmd(0x29);
  delay(20);

  // keep CS low for RGB streaming
  tcaSet(EXIO_CS, 0);
}

class LGFX : public lgfx::LGFX_Device
{
public:
  lgfx::Bus_RGB   _bus;
  lgfx::Panel_RGB _panel;
  lgfx::Light_PWM _light;

  LGFX()
  {
    // panel size
    {
      auto cfg = _panel.config();
      cfg.memory_width  = 480;
      cfg.memory_height = 480;
      cfg.panel_width   = 480;
      cfg.panel_height  = 480;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      _panel.config(cfg);
    }

    // RGB bus pins + timings (use your proven LVGL timings)
    {
      auto cfg = _bus.config();
      cfg.panel = &_panel;

      cfg.pin_d0  = GPIO_NUM_5;   // B1
      cfg.pin_d1  = GPIO_NUM_45;  // B2
      cfg.pin_d2  = GPIO_NUM_48;  // B3
      cfg.pin_d3  = GPIO_NUM_47;  // B4
      cfg.pin_d4  = GPIO_NUM_21;  // B5
      cfg.pin_d5  = GPIO_NUM_14;  // G0
      cfg.pin_d6  = GPIO_NUM_13;  // G1
      cfg.pin_d7  = GPIO_NUM_12;  // G2
      cfg.pin_d8  = GPIO_NUM_11;  // G3
      cfg.pin_d9  = GPIO_NUM_10;  // G4
      cfg.pin_d10 = GPIO_NUM_9;   // G5
      cfg.pin_d11 = GPIO_NUM_46;  // R1
      cfg.pin_d12 = GPIO_NUM_3;   // R2
      cfg.pin_d13 = GPIO_NUM_8;   // R3
      cfg.pin_d14 = GPIO_NUM_18;  // R4
      cfg.pin_d15 = GPIO_NUM_17;  // R5

      cfg.pin_henable = GPIO_NUM_40; // DE
      cfg.pin_vsync   = GPIO_NUM_39;
      cfg.pin_hsync   = GPIO_NUM_38;
      cfg.pin_pclk    = GPIO_NUM_41;

      cfg.freq_write  = 30 * 1000 * 1000;

      cfg.hsync_polarity    = 0;
      cfg.hsync_front_porch = 50;
      cfg.hsync_pulse_width = 8;
      cfg.hsync_back_porch  = 10;

      cfg.vsync_polarity    = 0;
      cfg.vsync_front_porch = 8;
      cfg.vsync_pulse_width = 2;
      cfg.vsync_back_porch  = 18;

      cfg.pclk_active_neg = 0;
      cfg.de_idle_high    = 0;

      _bus.config(cfg);
    }

    _panel.setBus(&_bus);

    // backlight (GPIO6)
    {
      auto cfg = _light.config();
      cfg.pin_bl = GPIO_NUM_6;
      _light.config(cfg);
      _panel.light(&_light);
    }

    setPanel(&_panel);
  }
};