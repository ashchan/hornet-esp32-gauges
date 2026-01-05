#include <FS.h>
#include <LittleFS.h>
#include "renderer.h"
#include "display_driver.h"

// Create tft screen
LGFX tft;

// Create sprites
LGFX_Sprite sMainSprite;
LGFX_Sprite sADI_BALL;               // ADI BALL
LGFX_Sprite sADI_BEZEL_STATIC;       // ADI BEZEL STATIC
LGFX_Sprite sADI_BEZEL_OUTER;        // ADI BEZEL OUTER
LGFX_Sprite sADI_BEZEL_INNER;        // ADI BEZEL INNER
LGFX_Sprite sADI_OFF_FLAG;           // ADI OFF FLAG
LGFX_Sprite sADI_SLIP_BALL;          // ADI SLIP BALL
LGFX_Sprite sADI_WINGS;              // ADI WINGS
LGFX_Sprite sILS_POINTER_H;          // ILS HORIZONTAL POINTER
LGFX_Sprite sILS_POINTER_V;          // ILS VERTICAL POINTER
LGFX_Sprite sTURN_RATE;              // TURN RATE INDICATOR
LGFX_Sprite sBANK_INDICATOR;
LGFX_Sprite sBEZEL_CLIPPED;          // Clipped BEZEL to cover animated area
LGFX_Sprite sBOTTOM_CLIPPED;         // Clipped BEZEL to cover animated area

// Parameters for the main clipped area - Bezel
constexpr int clipX = 82;
constexpr int clipY = 74;
constexpr int clipWidth = 316;
constexpr int clipHeight = 316;

// Parameters for the bottom clipped area - slip ball and rate of turn
constexpr int clipBottomX = 190;
constexpr int clipBottomY = 412;
constexpr int clipBottomWidth = 100;
constexpr int clipBottomHeight = 54;

const uint8_t colorDepth = 16;
const uint16_t transparentColor = 0x00FF00U; // Pure green

// Remove green transparency color from edges
void cleanSpriteEdges(LGFX_Sprite* sprite) {
  for (int y = 0; y < sprite->height(); y++) {
    for (int x = 0; x < sprite->width(); x++) {
      uint32_t pixel = sprite->readPixel(x, y);
      // Extract RGB components (RGB565 format)
      uint8_t r5 = (pixel >> 11) & 0x1F;
      uint8_t g6 = (pixel >> 5) & 0x3F;
      uint8_t b5 = pixel & 0x1F;

      if (g6 > 30 && r5 < 12 && b5 < 12) { // Very loose green detection
        sprite->drawPixel(x, y, transparentColor);
      }
    }
  }
}

struct Span { uint16_t x; uint16_t len; };

static uint16_t* spanOffsets = nullptr;
static uint16_t* spanCounts  = nullptr;
static Span* spans           = nullptr;
static uint32_t spanTotal    = 0;

void buildOpaqueSpans(LGFX_Sprite& patch, uint16_t keyColor) {
  const int W = patch.width();
  const int H = patch.height();
  uint16_t* buf = (uint16_t*)patch.getBuffer();

  // Allocate per-row metadata
  spanOffsets = (uint16_t*)ps_malloc(sizeof(uint16_t) * H);
  spanCounts  = (uint16_t*)ps_malloc(sizeof(uint16_t) * H);

  // First pass: count total spans so we can allocate once
  spanTotal = 0;
  for (int y = 0; y < H; ++y) {
    uint16_t count = 0;
    uint16_t* row = buf + y * W;

    bool inRun = false;
    for (int x = 0; x < W; ++x) {
      bool opaque = (row[x] != keyColor);
      if (opaque && !inRun) { inRun = true; count++; }
      else if (!opaque && inRun) { inRun = false; }
    }
    spanCounts[y] = count;
    spanTotal += count;
  }

  // Allocate span storage in PSRAM
  spans = (Span*)ps_malloc(sizeof(Span) * spanTotal);

  // Second pass: fill spans + offsets
  uint32_t idx = 0;
  for (int y = 0; y < H; ++y) {
    spanOffsets[y] = idx;
    uint16_t* row = buf + y * W;

    bool inRun = false;
    int startX = 0;

    for (int x = 0; x < W; ++x) {
      bool opaque = (row[x] != keyColor);

      if (opaque && !inRun) {
        inRun = true;
        startX = x;
      }
      else if (!opaque && inRun) {
        inRun = false;
        spans[idx++] = { (uint16_t)startX, (uint16_t)(x - startX) };
      }
    }

    // Close run at end of row
    if (inRun) {
      spans[idx++] = { (uint16_t)startX, (uint16_t)(W - startX) };
    }
  }
}

void pushWithOpaqueSpans(LGFX_Sprite& target, LGFX_Sprite& patch, int dstX, int dstY) {
  const int W = patch.width();
  const int H = patch.height();
  uint16_t* buf = (uint16_t*)patch.getBuffer();

  for (int y = 0; y < H; ++y) {
    uint16_t count = spanCounts[y];
    if (!count) continue;

    uint16_t* row = buf + y * W;
    uint32_t base = spanOffsets[y];

    for (int i = 0; i < count; ++i) {
      const Span& s = spans[base + i];
      target.pushImage(dstX + s.x, dstY + y, s.len, 1, row + s.x);
    }
  }
}

void createSprites() {
  sMainSprite.setPsram(true);
  sMainSprite.setColorDepth(colorDepth);
  sMainSprite.createSprite(480, 480);

  sADI_BALL.setPsram(true);
  sADI_BALL.setColorDepth(colorDepth);
  sADI_BALL.createFromBmp(LittleFS, "/Ball_big.bmp");
  cleanSpriteEdges(&sADI_BALL);

  sADI_BEZEL_STATIC.setPsram(true);
  sADI_BEZEL_STATIC.setColorDepth(colorDepth);
  sADI_BEZEL_STATIC.createFromBmp(LittleFS, "/Bezel_static_big.bmp");
  cleanSpriteEdges(&sADI_BEZEL_STATIC);

  sADI_BEZEL_OUTER.setPsram(true);
  sADI_BEZEL_OUTER.setColorDepth(colorDepth);
  sADI_BEZEL_OUTER.createFromBmp(LittleFS, "/Bezel_outer_big.bmp");
  cleanSpriteEdges(&sADI_BEZEL_OUTER);

  sADI_BEZEL_INNER.setPsram(true);
  sADI_BEZEL_INNER.setColorDepth(colorDepth);
  sADI_BEZEL_INNER.createFromBmp(LittleFS, "/Bezel_inner_big.bmp");
  cleanSpriteEdges(&sADI_BEZEL_INNER);

  sADI_OFF_FLAG.setPsram(true);
  sADI_OFF_FLAG.setColorDepth(colorDepth);
  sADI_OFF_FLAG.createFromBmp(LittleFS, "/WarnFlag_big.bmp");
  cleanSpriteEdges(&sADI_OFF_FLAG);

  sADI_WINGS.setPsram(true);
  sADI_WINGS.setColorDepth(colorDepth);
  sADI_WINGS.createFromBmp(LittleFS, "/Wing_big.bmp");
  cleanSpriteEdges(&sADI_WINGS);

  sBANK_INDICATOR.setPsram(true);
  sBANK_INDICATOR.setColorDepth(colorDepth);
  sBANK_INDICATOR.createFromBmp(LittleFS, "/Bank.bmp");
  cleanSpriteEdges(&sBANK_INDICATOR);

  sADI_SLIP_BALL.setPsram(true);
  sADI_SLIP_BALL.setColorDepth(colorDepth);
  sADI_SLIP_BALL.createFromBmp(LittleFS, "/Slip-Ball.bmp");
  cleanSpriteEdges(&sADI_SLIP_BALL);

  sILS_POINTER_H.setPsram(true);
  sILS_POINTER_H.setColorDepth(colorDepth);
  sILS_POINTER_H.createFromBmp(LittleFS, "/ILS-H.bmp");
  cleanSpriteEdges(&sILS_POINTER_H);

  sILS_POINTER_V.setPsram(true);
  sILS_POINTER_V.setColorDepth(colorDepth);
  sILS_POINTER_V.createFromBmp(LittleFS, "/ILS-V.bmp");
  cleanSpriteEdges(&sILS_POINTER_V);

  sTURN_RATE.setPsram(true);
  sTURN_RATE.setColorDepth(colorDepth);
  sTURN_RATE.createFromBmp(LittleFS, "/Slip.bmp");
  cleanSpriteEdges(&sTURN_RATE);

  sBEZEL_CLIPPED.setPsram(true);
  sBEZEL_CLIPPED.setColorDepth(colorDepth);
  sBEZEL_CLIPPED.createSprite(clipWidth + 1, clipHeight + 1);
  sADI_BEZEL_STATIC.pushSprite(&sBEZEL_CLIPPED, -clipX, -clipY);
  uint16_t* buf = (uint16_t*)sBEZEL_CLIPPED.getBuffer();
  // sample center pixel, which should be the transparent color
  uint16_t keyColor = buf[((clipHeight + 1) / 2) * (clipWidth + 1) + ((clipWidth + 1) / 2)];
  buildOpaqueSpans(sBEZEL_CLIPPED, keyColor);

  sBOTTOM_CLIPPED.setPsram(true);
  sBOTTOM_CLIPPED.setColorDepth(colorDepth);
  sBOTTOM_CLIPPED.createSprite(clipBottomWidth, clipBottomHeight);
  sADI_BEZEL_STATIC.pushSprite(&sBOTTOM_CLIPPED, -clipBottomX, -clipBottomY);
}

void initRenderer() {
  panel_power_and_reset();
  st7701_init_rgb565();

  tft.begin();
  tft.setColorDepth(colorDepth);
  tft.setSwapBytes(false);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  LittleFS.begin(true);
  createSprites();

  // Draw the full bezel. The outer edges are not animated.
  sADI_BEZEL_STATIC.pushSprite(&tft, -1, -1, transparentColor);

  sBANK_INDICATOR.setPivot(sBANK_INDICATOR.width() / 2, 160);
  sADI_OFF_FLAG.setPivot(8, 10);
}

void render(SaiMessage message) {
  static bool needsFullRedraw = true;

  int ballY = map(message.pitch, 0, 65535, 180, 1580);
  sADI_BALL.setPivot(sADI_BALL.width() / 2, ballY);
  int ballAngle = map(message.bank, 0, 65534, -180, 180); // 65534: take of the initial round (we use 65535 / 2 as default value)
  int bankIndicatorAngle = map(message.bank, 0, 65535, -180, 180);
  int wingsY = map(message.manPitchAdj, 0, 65535, 0, tft.height());
  int pointerHorY = map(message.pointerHor, 0, 65535, 25, 450);
  int pointerVerX = map(message.pointerVer, 0, 65535, 15, 420); // Need to consider the handle position
  int slipBallX = map(message.slipBall, 0, 65535, 190, 265);
  int turnRateX = map(message.rateOfTurn, 0, 65535, 197, 262);

  static int prevAdiOffFlagAngle = -1;
  int adiOffFlagAngle = message.attWarningFlag == 0 ? 0 : 30;

  bool needsRedrawOffFlag = prevAdiOffFlagAngle != adiOffFlagAngle;
  prevAdiOffFlagAngle = adiOffFlagAngle;
  if (needsRedrawOffFlag) {
    needsFullRedraw = true;
  }

  tft.startWrite();

  tft.setClipRect(clipBottomX, clipBottomY, clipBottomWidth, clipBottomHeight);
  sBOTTOM_CLIPPED.pushSprite(&sMainSprite, clipBottomX, clipBottomY, transparentColor);
  sADI_SLIP_BALL.pushSprite(&sMainSprite, slipBallX, 413, transparentColor);
  sTURN_RATE.pushSprite(&sMainSprite, turnRateX, 447, transparentColor);
  sMainSprite.pushSprite(&tft, 0, 0);

  tft.setClipRect(clipX, clipY, clipWidth, clipHeight);
  sADI_BALL.pushRotateZoom(&sMainSprite, 240, 230, ballAngle, 1, 1);

  if (needsFullRedraw) {
    sADI_BEZEL_INNER.pushSprite(&tft, -1, -1, transparentColor);
  }
  pushWithOpaqueSpans(sMainSprite, sBEZEL_CLIPPED, clipX - 1, clipY - 1);

  sADI_WINGS.pushSprite(&sMainSprite, 110, wingsY, transparentColor);
  sILS_POINTER_H.pushSprite(&sMainSprite, 68, pointerHorY, transparentColor);
  sILS_POINTER_V.pushSprite(&sMainSprite, pointerVerX, 70, transparentColor);
  sBANK_INDICATOR.pushRotateZoom(&sMainSprite, 240, 233, bankIndicatorAngle, 1, 1, transparentColor);

  static std::uint32_t sec, psec;
  static std::uint32_t fps = 0, frame_count = 0;
  sMainSprite.setCursor(100, 100);
  sMainSprite.setTextColor(TFT_WHITE);
  sMainSprite.printf("fps:%d", fps);
  sMainSprite.pushSprite(&tft, 0, 0);

  tft.clearClipRect();

  //sADI_OFF_FLAG.pushRotateZoom(&tft, 420, 120, adiOffFlagAngle, 1, 1, transparentColor);
  //if (needsFullRedraw) {
  //  sADI_BEZEL_OUTER.pushSprite(&tft, -1, -1, transparentColor);
  //}

  tft.endWrite();
  needsFullRedraw = false;

  // Calc FPS
  ++frame_count;
  sec = lgfx::millis() / 1000;
  if (psec != sec) {
    psec = sec;
    fps = frame_count;
    frame_count = 0;
  }
}

void setBrightness(uint16_t value) {
  static uint16_t oldValue = 0;
  uint16_t newValue = map(value, 0, 65535, DEFAULT_BRIGHTNESS, 255);
  if (oldValue != newValue) {
    oldValue = newValue;
    tft.setBrightness(newValue);
  }
}