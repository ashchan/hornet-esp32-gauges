#include "renderer.h"

#include "Display_ST7789.h"

#include <stdint.h>
#include <string.h>
#include <lvgl.h>

LV_IMG_DECLARE(standbyCompassTape);
extern "C" const uint8_t standbyCompassTape_map[];

namespace {
constexpr int32_t kDisplayWidth = 172;
constexpr int32_t kDisplayHeight = 320;
constexpr int32_t kDisplayCenterY = kDisplayHeight / 2;
constexpr int32_t kTapeBandX = 56;
constexpr int32_t kTapeBandWidth = 84;
constexpr int32_t kTapeWidth = kTapeBandWidth;
constexpr int32_t kCompassCycleHeight = 1120;
constexpr int32_t kTapeHeight = 1440;
constexpr lv_coord_t kNorthY = 561;
constexpr int32_t kFlushRows = 16;
constexpr uint16_t kBackgroundColor = 0x1082;
constexpr uint16_t kLubberLineColor = 0xdedb;
constexpr int32_t kLubberLineHalfThickness = 1;
constexpr int32_t kLubberLineRows = kLubberLineHalfThickness * 2 + 1;
constexpr int32_t kTrapezoidTopInsetPx = 14;
constexpr int32_t kPerspectiveTopPullPx = 12;

uint16_t frameBuffer[kTapeBandWidth * kDisplayHeight];
uint16_t clearBuffer[kDisplayWidth * kFlushRows];
uint16_t lubberLineBuffer[kDisplayWidth * kLubberLineRows];

int32_t centerYForHeadingCentiDegrees(uint32_t headingCentiDegrees) {
  const uint32_t normalized = headingCentiDegrees % 36000;
  const int32_t distanceFromNorth = ((36000 - normalized) % 36000) * kCompassCycleHeight / 36000;
  return kNorthY + distanceFromNorth;
}

uint16_t normalizeBackgroundPixel(uint16_t pixel) {
  const uint16_t red = (pixel >> 11) & 0x1f;
  const uint16_t green = (pixel >> 5) & 0x3f;
  const uint16_t blue = pixel & 0x1f;
  if (red < 8 && green < 16 && blue < 8) {
    return kBackgroundColor;
  }

  return pixel;
}

int32_t edgeFactorForDisplayY(int32_t y) {
  const int32_t centerDistance = abs(y - kDisplayCenterY);
  return centerDistance * centerDistance * 1000 / (kDisplayCenterY * kDisplayCenterY);
}

int32_t wrapTapeY(int32_t sourceY) {
  while (sourceY < 0) {
    sourceY += kCompassCycleHeight;
  }
  while (sourceY >= kTapeHeight) {
    sourceY -= kCompassCycleHeight;
  }

  return sourceY;
}

int32_t perspectiveHeadingOffset(int32_t displayY, int32_t sourceX) {
  const int32_t topWeight = (kTapeWidth - 1 - sourceX) * 1000 / (kTapeWidth - 1);
  return (kDisplayCenterY - displayY) * kPerspectiveTopPullPx * topWeight / (kDisplayCenterY * 1000);
}

void drawTapeCrop(int32_t topY) {
  const uint16_t *tapePixels = reinterpret_cast<const uint16_t *>(standbyCompassTape_map);
  for (int32_t y = 0; y < kDisplayHeight; y++) {
    uint16_t *target = &frameBuffer[y * kTapeBandWidth];
    const int32_t topInset = kTrapezoidTopInsetPx * edgeFactorForDisplayY(y) / 1000;
    const int32_t projectedHeight = kTapeBandWidth - topInset;

    for (int32_t x = 0; x < kTapeBandWidth; x++) {
      if (x < topInset) {
        target[x] = kBackgroundColor;
      } else {
        const int32_t sourceX = (x - topInset) * kTapeWidth / projectedHeight;
        const int32_t sourceY = wrapTapeY(topY + y + perspectiveHeadingOffset(y, sourceX));
        target[x] = normalizeBackgroundPixel(tapePixels[sourceY * kTapeWidth + sourceX]);
      }
    }
  }
}

void initLubberLineBuffer() {
  for (uint16_t &pixel : lubberLineBuffer) {
    pixel = kLubberLineColor;
  }
}

void drawLubberLineOnTape() {
  for (int32_t y = kDisplayCenterY - kLubberLineHalfThickness;
       y <= kDisplayCenterY + kLubberLineHalfThickness;
       y++) {
    uint16_t *target = &frameBuffer[y * kTapeBandWidth];
    for (int32_t x = 0; x < kTapeBandWidth; x++) {
      target[x] = kLubberLineColor;
    }
  }
}

void clearScreen() {
  for (uint16_t &pixel : clearBuffer) {
    pixel = kBackgroundColor;
  }

  for (int32_t y = 0; y < kDisplayHeight; y += kFlushRows) {
    const int32_t rows = ((y + kFlushRows) <= kDisplayHeight) ? kFlushRows : (kDisplayHeight - y);
    LCD_addWindow(0,
                  y,
                  kDisplayWidth - 1,
                  y + rows - 1,
                  clearBuffer);
  }
}

void flushFrameBuffer() {
  for (int32_t y = 0; y < kDisplayHeight; y += kFlushRows) {
    const int32_t rows = ((y + kFlushRows) <= kDisplayHeight) ? kFlushRows : (kDisplayHeight - y);
    LCD_addWindow(kTapeBandX,
                  y,
                  kTapeBandX + kTapeBandWidth - 1,
                  y + rows - 1,
                  &frameBuffer[y * kTapeBandWidth]);
  }
}

void flushLubberLine() {
  if (kTapeBandX > 0) {
    LCD_addWindow(0,
                  kDisplayCenterY - kLubberLineHalfThickness,
                  kTapeBandX - 1,
                  kDisplayCenterY + kLubberLineHalfThickness,
                  lubberLineBuffer);
  }

  const int32_t rightX = kTapeBandX + kTapeBandWidth;
  if (rightX < kDisplayWidth) {
    LCD_addWindow(rightX,
                  kDisplayCenterY - kLubberLineHalfThickness,
                  kDisplayWidth - 1,
                  kDisplayCenterY + kLubberLineHalfThickness,
                  lubberLineBuffer);
  }
}
}

void Renderer_Init(void) {
  initLubberLineBuffer();
  clearScreen();
  Renderer_UpdateHeading(0);
  flushLubberLine();
}

void Renderer_UpdateHeading(uint16_t heading) {
  Renderer_UpdateHeadingCentiDegrees((uint32_t)(heading % 360) * 100);
}

void Renderer_UpdateHeadingRaw(uint16_t headingRaw) {
  Renderer_UpdateHeadingCentiDegrees((uint32_t)headingRaw * 36000UL / 65536UL);
}

void Renderer_UpdateHeadingCentiDegrees(uint32_t headingCentiDegrees) {
  const int32_t topY = centerYForHeadingCentiDegrees(headingCentiDegrees) - kDisplayCenterY;
  drawTapeCrop(topY);
  drawLubberLineOnTape();
  flushFrameBuffer();
}
