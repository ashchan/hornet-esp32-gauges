#include <FS.h>
#include <LittleFS.h>
#include "renderer.h"
#include "display_driver.h"

// Create tft screen
LGFX tft;

// Create sprites
LGFX_Sprite sMainSprite[2];
LGFX_Sprite sADI_BALL;               // ADI BALL
LGFX_Sprite sADI_BEZEL_Static;       // ADI BEZEL
LGFX_Sprite sADI_OFF_FLAG;           // ADI OFF FLAG
LGFX_Sprite sADI_SLIP_BALL;          // ADI SLIP BALL
LGFX_Sprite sADI_WINGS;              // ADI WINGS
LGFX_Sprite sILS_POINTER_H;          // ILS HORIZONTAL POINTER
LGFX_Sprite sILS_POINTER_V;          // ILS VERTICAL POINTER
LGFX_Sprite sTURN_RATE;              // TURN RATE INDICATOR
LGFX_Sprite sBANK_INDICATOR;
LGFX_Sprite sBEZEL_CLIPPED;          // Clipped BEZEL to cover animated area

constexpr int clipX = 82;
constexpr int clipY = 75;
constexpr int clipWidth = 316;
constexpr int clipHeight = 390;

uint8_t colordepth = 16;

void cleanSpriteEdges(LGFX_Sprite* sprite) {
  // More aggressive cleaning for problematic sprites like off flag
  for (int y = 0; y < sprite->height(); y++) {
    for (int x = 0; x < sprite->width(); x++) {
      uint32_t pixel = sprite->readPixel(x, y);
      // Extract RGB components (RGB565 format)
      uint8_t r5 = (pixel >> 11) & 0x1F;
      uint8_t g6 = (pixel >> 5) & 0x3F;
      uint8_t b5 = pixel & 0x1F;

      // Much looser tolerance - catch any pixel with significant green
      if (g6 > 30 && r5 < 12 && b5 < 12) { // Very loose green detection
        sprite->drawPixel(x, y, 0x00FF00U); // Pure green
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
  for (int i = 0; i < 2; i++) {
    sMainSprite[i].setPsram(true);
    sMainSprite[i].setColorDepth(colordepth);
    sMainSprite[i].createSprite(480, 480);
  }
  sADI_BALL.setPsram(true);
  sADI_BALL.setColorDepth(colordepth);
  sADI_BALL.createFromBmp(LittleFS, "/Ball_big.bmp");
  cleanSpriteEdges(&sADI_BALL);

  sADI_BEZEL_Static.setPsram(true);
  sADI_BEZEL_Static.setColorDepth(colordepth);
  sADI_BEZEL_Static.createFromBmp(LittleFS, "/Bezel_static_big.bmp");
  cleanSpriteEdges(&sADI_BEZEL_Static);

  sADI_OFF_FLAG.setPsram(true);
  sADI_OFF_FLAG.setColorDepth(colordepth);
  sADI_OFF_FLAG.createFromBmp(LittleFS, "/WarnFlag_big.bmp");
  cleanSpriteEdges(&sADI_OFF_FLAG);

  sADI_WINGS.setPsram(true);
  sADI_WINGS.setColorDepth(colordepth);
  sADI_WINGS.createFromBmp(LittleFS, "/Wing_big.bmp");
  cleanSpriteEdges(&sADI_WINGS);

  sBANK_INDICATOR.setPsram(true);
  sBANK_INDICATOR.setColorDepth(colordepth);
  sBANK_INDICATOR.createFromBmp(LittleFS, "/Bank.bmp");
  cleanSpriteEdges(&sBANK_INDICATOR);

  sADI_SLIP_BALL.setPsram(true);
  sADI_SLIP_BALL.setColorDepth(colordepth);
  sADI_SLIP_BALL.createFromBmp(LittleFS, "/Slip-Ball.bmp");
  cleanSpriteEdges(&sADI_SLIP_BALL);

  sILS_POINTER_H.setPsram(true);
  sILS_POINTER_H.setColorDepth(colordepth);
  sILS_POINTER_H.createFromBmp(LittleFS, "/ILS-H.bmp");
  cleanSpriteEdges(&sILS_POINTER_H);

  sILS_POINTER_V.setPsram(true);
  sILS_POINTER_V.setColorDepth(colordepth);
  sILS_POINTER_V.createFromBmp(LittleFS, "/ILS-V.bmp");
  cleanSpriteEdges(&sILS_POINTER_V);

  sTURN_RATE.setPsram(true);
  sTURN_RATE.setColorDepth(colordepth);
  sTURN_RATE.createFromBmp(LittleFS, "/Slip.bmp");
  cleanSpriteEdges(&sTURN_RATE);

  sBEZEL_CLIPPED.setPsram(true);
  sBEZEL_CLIPPED.setColorDepth(colordepth);
  sBEZEL_CLIPPED.createSprite(clipWidth, clipHeight);
  sADI_BEZEL_Static.pushSprite(&sBEZEL_CLIPPED, -clipX, -clipY);
  uint16_t* buf = (uint16_t*)sBEZEL_CLIPPED.getBuffer();
  int W = sBEZEL_CLIPPED.width();
  int H = sBEZEL_CLIPPED.height();
  // sample center pixel
  uint16_t keyColor = buf[(H/2) * W + (W/2)];
  buildOpaqueSpans(sBEZEL_CLIPPED, keyColor);
}

void initRenderer() {
  panel_power_and_reset();
  st7701_init_rgb565();

  tft.begin();
  tft.setColorDepth(colordepth);
  tft.setSwapBytes(false);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  LittleFS.begin(true);
  createSprites();
  sADI_BEZEL_Static.pushSprite(&tft, 0, 0);
}

void render(SaiMessage message) {
  static bool flip = 0;
  flip = !flip;
  LGFX_Sprite* mainSprite = &sMainSprite[flip];

  int ballY = map(message.pitch, 0, 65535, 180, 1580);
  sADI_BALL.setPivot(sADI_BALL.width() / 2, ballY);
  int ballAngle = map(message.bank, 0, 65534, -180, 180); // 65534: take of the initial round (we use 65535 / 2 as default value)
  int bankIndicatorAngle = map(message.bank, 0, 65535, -60, 60);
  //sBANK_INDICATOR.setPivot(sBANK_INDICATOR.width() / 2, 180);
  int wingsY = map(message.manPitchAdj, 0, 65535, 0, tft.height());
  int pointerHorY = map(message.pointerHor, 0, 65535, 50, 280);
  int pointerVerX = map(message.pointerVer, 0, 65535, 50, 280);
  int slipBallX = map(message.slipBall, 0, 65535, 190, 265);
  int turnRateX = map(message.rateOfTurn, 0, 65535, 197, 263);
  int adiOffFlagAngle = map(message.attWarningFlag, 0, 65535, 0, 20);

  tft.startWrite();

  static bool needsFullRedraw = true;
  if (needsFullRedraw) {
    // Draw the full bezel. The outer edges are not animated.
    sADI_BEZEL_Static.pushSprite(&tft, 0, 0);
    needsFullRedraw = false;
  }

  tft.setClipRect(clipX, clipY, clipWidth, clipHeight);

  sADI_BALL.pushRotateZoom(mainSprite, 240, 240, ballAngle, 1, 1);
  sADI_WINGS.pushSprite(mainSprite, 110, wingsY, 0x00FF00U);
  sILS_POINTER_H.pushSprite(mainSprite, 80, pointerHorY, 0x00FF00U);
  sILS_POINTER_V.pushSprite(mainSprite, pointerVerX, 100, 0x00FF00U);
  sBANK_INDICATOR.pushRotateZoom(mainSprite, 240, 240, bankIndicatorAngle, 1, 1, 0x00FF00U);
  pushWithOpaqueSpans(*mainSprite, sBEZEL_CLIPPED, clipX, clipY);
  // TODO: off flag is outside of clip rect. Need to handle that.
  sADI_OFF_FLAG.pushRotateZoom(mainSprite, 430, 150, adiOffFlagAngle, 1, 1, 0x00FF00U);
  sADI_SLIP_BALL.pushSprite(mainSprite, slipBallX, 413, 0x00FF00U);
  sTURN_RATE.pushSprite(mainSprite, turnRateX, 447, 0x00FF00U);

  static std::uint32_t sec, psec;
  static std::uint32_t fps = 0, frame_count = 0;
  mainSprite->setCursor(100, 100);
  mainSprite->setTextColor(TFT_WHITE);
  mainSprite->printf("fps:%d", fps);
  mainSprite->pushSprite(&tft, 0, 0);
   tft.clearClipRect();
  tft.endWrite();

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