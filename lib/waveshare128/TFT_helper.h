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

// Blend pixel toward green based on brightness + knob.
// This makes even pure-white markings shift greener (unlike additive).
void tintGreen(TFT_eSprite& sprite,
                             uint8_t illum,         // 0..255
                             uint16_t greenNative = 0x07E0,  // e.g. 0x07E0 (native RGB565)
                             uint8_t strength = 255,// 0..255
                             uint8_t threshold = 2) // 0..31
{
  uint16_t* p = (uint16_t*)sprite.getPointer();
  if (!p || illum == 0) return;

  const int n = sprite.width() * sprite.height();

  // Convert green to swapped storage once
  uint16_t greenSwapped = SWAP16(greenNative);

  // Unpack native green once (0..31,0..63,0..31)
  uint16_t gn = greenNative;
  uint8_t gr5 = (gn >> 11) & 0x1F;
  uint8_t gg6 = (gn >>  5) & 0x3F;
  uint8_t gb5 =  gn        & 0x1F;

  for (int i = 0; i < n; i++)
  {
    uint16_t cn = SWAP16(p[i]); // sprite is swapped, convert to native

    uint8_t r5 = (cn >> 11) & 0x1F;
    uint8_t g6 = (cn >>  5) & 0x3F;
    uint8_t b5 =  cn        & 0x1F;

    // brightness estimate (0..31)
    uint8_t lum5 = r5;
    if (b5 > lum5) lum5 = b5;
    uint8_t g5 = g6 >> 1;
    if (g5 > lum5) lum5 = g5;

    if (lum5 < threshold) continue;

    // Alpha = brightness * knob * strength (0..255)
    // lum5/31 gives 0..1, then scaled by illum and strength.
    uint16_t a = (uint32_t)lum5 * illum * strength / (31u * 255u);
    if (a > 255) a = 255;

    // Blend toward green target:
    // out = src*(1-a) + green*a
    r5 = (r5 * (255 - a) + gr5 * a) / 255;
    g6 = (g6 * (255 - a) + gg6 * a) / 255;
    b5 = (b5 * (255 - a) + gb5 * a) / 255;

    uint16_t outNative = (r5 << 11) | (g6 << 5) | b5;
    p[i] = SWAP16(outNative); // store swapped
  }
}
