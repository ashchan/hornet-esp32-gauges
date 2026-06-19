#include "renderer.h"

#include "Display_ST7789.h"
#include "tape_masks.h"

#include <stdint.h>
#include <string.h>

namespace {
constexpr int32_t kDisplayWidth = 172;
constexpr int32_t kDisplayHeight = 320;
constexpr int32_t kDisplayCenterY = kDisplayHeight / 2;
constexpr int32_t kTapeBandX = 56;
constexpr int32_t kTapeBandWidth = 84;
constexpr int32_t kTapeWidth = kTapeBandWidth;
constexpr int32_t kCompassCycleHeight = 1120;
constexpr int32_t kTapeHeight = TAPE_LABEL_MASK_HEIGHT;
constexpr int32_t kNorthY = 561;
constexpr int32_t kFlushRows = 16;
constexpr uint16_t kBackgroundColor = 0x1082;
constexpr uint16_t kMarkColor = 0xffff;
constexpr uint16_t kLubberLineColor = 0xdedb;
constexpr int32_t kLubberLineHalfThickness = 1;
constexpr int32_t kLubberLineRows = kLubberLineHalfThickness * 2 + 1;
constexpr int32_t kTrapezoidTopInsetPx = 8;
constexpr int32_t kPerspectiveTopPullPx = 12;
constexpr int32_t kTickStartX = 13;
constexpr int32_t kTickFiveDegreeEndX = 22;
constexpr int32_t kTickEndX = 28;
constexpr int32_t kTickThinHalfWidth = 1;
constexpr int32_t kTickThickHalfWidth = 2;

uint16_t frameBuffer[kTapeBandWidth * kDisplayHeight];
uint16_t clearBuffer[kDisplayWidth * kFlushRows];
uint16_t lubberLineBuffer[kDisplayWidth * kLubberLineRows];

int32_t centerYForHeadingCentiDegrees(uint32_t headingCentiDegrees) {
  const uint32_t normalized = headingCentiDegrees % 36000;
  const int32_t distanceFromNorth = ((36000 - normalized) % 36000) * kCompassCycleHeight / 36000;
  return kNorthY + distanceFromNorth;
}

int32_t edgeFactorForDisplayY(int32_t y) {
  const int32_t centerDistance = abs(y - kDisplayCenterY);
  return centerDistance * centerDistance * 1000 / (kDisplayCenterY * kDisplayCenterY);
}

int32_t wrapHeight(int32_t value, int32_t height) {
  while (value < 0) {
    value += height;
  }
  while (value >= height) {
    value -= height;
  }

  return value;
}

int32_t perspectiveHeadingOffset(int32_t displayY, int32_t sourceX) {
  const int32_t topWeight = (kTapeWidth - 1 - sourceX) * 1000 / (kTapeWidth - 1);
  return (kDisplayCenterY - displayY) * kPerspectiveTopPullPx * topWeight / (kDisplayCenterY * 1000);
}

uint8_t maskAlpha4(const uint8_t *mask, int32_t width, int32_t x, int32_t y) {
  const int32_t index = y * width + x;
  const uint8_t packed = mask[index / 2];
  return (index & 1) ? (packed & 0x0f) : (packed >> 4);
}

uint8_t labelAlpha4(int32_t x, int32_t y) {
  if (x < 0 || x >= kTapeWidth) {
    return 0;
  }

  return maskAlpha4(standbyCompassLabelMask4, TAPE_MASK_WIDTH, x, wrapHeight(y, kCompassCycleHeight));
}

uint8_t lightenedLabelAlpha4(int32_t x, int32_t y) {
  const uint8_t alpha = labelAlpha4(x, y);
  if (alpha == 0) {
    return 0;
  }

  const bool edgePixel =
      labelAlpha4(x - 1, y) == 0 ||
      labelAlpha4(x + 1, y) == 0 ||
      labelAlpha4(x, y - 1) == 0 ||
      labelAlpha4(x, y + 1) == 0;

  return edgePixel ? 9 : alpha;
}

uint16_t blend565(uint16_t background, uint16_t foreground, uint8_t alpha4) {
  if (alpha4 == 0) {
    return background;
  }
  if (alpha4 >= 15) {
    return foreground;
  }

  const uint16_t bgR = (background >> 11) & 0x1f;
  const uint16_t bgG = (background >> 5) & 0x3f;
  const uint16_t bgB = background & 0x1f;
  const uint16_t fgR = (foreground >> 11) & 0x1f;
  const uint16_t fgG = (foreground >> 5) & 0x3f;
  const uint16_t fgB = foreground & 0x1f;
  const uint16_t r = (bgR * (15 - alpha4) + fgR * alpha4) / 15;
  const uint16_t g = (bgG * (15 - alpha4) + fgG * alpha4) / 15;
  const uint16_t b = (bgB * (15 - alpha4) + fgB * alpha4) / 15;
  return (r << 11) | (g << 5) | b;
}

uint8_t tickAlpha4(int32_t sourceX, int32_t sourceY) {
  const int32_t position = wrapHeight(sourceY - kNorthY, kCompassCycleHeight);
  const int32_t tickIndex = (position * 72 + (kCompassCycleHeight / 2)) / kCompassCycleHeight;
  const int32_t wrappedTickIndex = tickIndex % 72;
  const int32_t tickY = wrappedTickIndex * kCompassCycleHeight / 72;
  int32_t distance = abs(position - tickY);
  if (distance > (kCompassCycleHeight / 2)) {
    distance = kCompassCycleHeight - distance;
  }

  const bool labeledTick = (wrappedTickIndex % 6) == 0;
  const bool tenDegreeTick = (wrappedTickIndex % 2) == 0;
  const int32_t tickEndX = tenDegreeTick ? kTickEndX : kTickFiveDegreeEndX;
  const int32_t halfWidth = labeledTick ? kTickThickHalfWidth : kTickThinHalfWidth;

  if (sourceX < kTickStartX || sourceX > tickEndX || distance > halfWidth) {
    return 0;
  }

  if (distance == halfWidth && halfWidth > 1) {
    return 10;
  }
  return 15;
}

uint16_t sampleTapePixel(int32_t sourceX, int32_t sourceY) {
  if (sourceX < 0 || sourceX >= kTapeWidth) {
    return kBackgroundColor;
  }

  const uint8_t labelAlpha = lightenedLabelAlpha4(sourceX, sourceY);
  const uint8_t markAlpha = tickAlpha4(sourceX, sourceY);
  const uint8_t alpha = labelAlpha > markAlpha ? labelAlpha : markAlpha;
  return blend565(kBackgroundColor, kMarkColor, alpha);
}

void drawTapeCrop(int32_t topY) {
  for (int32_t y = 0; y < kDisplayHeight; y++) {
    uint16_t *target = &frameBuffer[y * kTapeBandWidth];
    const int32_t topInset = kTrapezoidTopInsetPx * edgeFactorForDisplayY(y) / 1000;
    const int32_t projectedHeight = kTapeBandWidth - topInset;

    for (int32_t x = 0; x < kTapeBandWidth; x++) {
      if (x < topInset) {
        target[x] = kBackgroundColor;
      } else {
        const int32_t sourceX = (x - topInset) * kTapeWidth / projectedHeight;
        const int32_t sourceY = topY + y + perspectiveHeadingOffset(y, sourceX);
        target[x] = sampleTapePixel(sourceX, sourceY);
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
