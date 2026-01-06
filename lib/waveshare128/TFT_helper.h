#pragma once

#include <Arduino.h>
#include <TFT_eSPI.h>
#include <cstdint>

#define DEFAULT_TFT_BRIGHTNESS 20

static inline uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// TODO: use pure green now. Check if need a more close NVIS green
const uint16_t NVIS_GREEN = 0xE007;

// value: 0-65536, map to DEFAULT_TFT_BRIGHTNESS - 200 (display content even if input is 0)
static void setBrightness(uint16_t value) {
  static uint16_t oldValue = 0;
  uint16_t newValue = map(value, 0, 65535, DEFAULT_TFT_BRIGHTNESS, 200);
  if (oldValue != newValue) {
    oldValue = newValue;
    analogWrite(TFT_BL, newValue);
  }
}

static inline uint16_t SWAP16(uint16_t x) { return (x << 8) | (x >> 8); }
void recolorSpriteGreen(TFT_eSprite& sprite) {
  uint16_t* p = (uint16_t*)sprite.getPointer();
  if (!p) {
    return;
  }

  uint16_t green = NVIS_GREEN;
  int n = sprite.width() * sprite.height();

  bool swapped = sprite.getSwapBytes();
  if (swapped) {
    green = SWAP16(green);
  }

  for (int i = 0; i < n; i++) {
    uint16_t c = p[i];
    if (swapped) {
      c = SWAP16(c);
    }

    uint8_t r = (c >> 11) & 0x1F;
    uint8_t g = (c >> 5) & 0x3F;
    uint8_t b = (c >> 0) & 0x1F;

    if (r >= 28 && g >= 56 && b >= 28) {
      p[i] = swapped ? SWAP16(green) : green;
    }
  }
}
