#pragma once

#include <Arduino.h>
#include <cstdint>

#define DEFAULT_TFT_BRIGHTNESS 20

// value: 0-65536, map to DEFAULT_TFT_BRIGHTNESS - 200 (display content even if input is 0)
static void setBrightness(uint16_t value) {
  static uint16_t oldValue = 0;
  uint16_t newValue = map(value, 0, 65535, DEFAULT_TFT_BRIGHTNESS, 200);
  if (oldValue != newValue) {
    oldValue = newValue;
    analogWrite(TFT_BL, newValue);
  }
}