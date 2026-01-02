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
LGFX_Sprite sADI_BEZEL_Inner;        // ADI BEZEL
LGFX_Sprite sADI_OFF_FLAG;           // ADI OFF FLAG
LGFX_Sprite sADI_BEZEL;              // ADI BEZEL
LGFX_Sprite sADI_SLIP_BALL;          // ADI SLIP BALL
LGFX_Sprite sADI_WINGS;              // ADI WINGS
LGFX_Sprite sILS_POINTER_H;          // ILS HORIZONTAL POINTER
LGFX_Sprite sILS_POINTER_V;          // ILS VERTICAL POINTER
LGFX_Sprite sTURN_RATE;              // TURN RATE INDICATOR
LGFX_Sprite sBANK_INDICATOR;

uint8_t colordepth = 16;

void cleanSpriteEdges(LGFX_Sprite* sprite) {
  // Replace near-green pixels with pure green for proper transparency
  for (int y = 0; y < sprite->height(); y++) {
    for (int x = 0; x < sprite->width(); x++) {
      uint32_t pixel = sprite->readPixel(x, y);
      // Extract RGB components (RGB565 format)
      uint8_t r5 = (pixel >> 11) & 0x1F;
      uint8_t g6 = (pixel >> 5) & 0x3F;
      uint8_t b5 = pixel & 0x1F;

      // Check if pixel is close to green (0x00FF00 = R:0 G:63 B:0 in RGB565)
      // Allow some tolerance for anti-aliased edges
      if (g6 > 50 && r5 < 8 && b5 < 8) { // Close to green
        sprite->drawPixel(x, y, 0x00FF00U); // Pure green
      }
    }
  }
}

void cleanSpriteEdgesAggressive(LGFX_Sprite* sprite) {
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

//create sprites for digital display areas and text lables; Fonts loaded from LittleFS
void create_display_elements() {
  for (int i = 0; i < 2; i++) {
    sMainSprite[i].setPsram(true);
    sMainSprite[i].setColorDepth(colordepth);
    sMainSprite[i].createSprite(480, 480);
  }
  sADI_BALL.setPsram(true);
  sADI_BALL.setColorDepth(colordepth);
  sADI_BALL.createFromBmp(LittleFS, "/Ball_big.bmp");
  cleanSpriteEdges(&sADI_BALL);

  sADI_BEZEL.setPsram(true);
  sADI_BEZEL.setColorDepth(colordepth);
  sADI_BEZEL.createFromBmp(LittleFS, "/Bazel_outer_big.bmp");
  cleanSpriteEdges(&sADI_BEZEL);

  sADI_BEZEL_Inner.setPsram(true);
  sADI_BEZEL_Inner.setColorDepth(colordepth);
  sADI_BEZEL_Inner.createFromBmp(LittleFS, "/Bazel_inner_big.bmp");
  cleanSpriteEdges(&sADI_BEZEL_Inner);

  sADI_BEZEL_Static.setPsram(true);
  sADI_BEZEL_Static.setColorDepth(colordepth);
  sADI_BEZEL_Static.createFromBmp(LittleFS, "/Bazel_static_big.bmp");
  cleanSpriteEdges(&sADI_BEZEL_Static);

  sADI_OFF_FLAG.setPsram(true);
  sADI_OFF_FLAG.setColorDepth(colordepth);
  sADI_OFF_FLAG.createFromBmp(LittleFS, "/WarnFlag_big.bmp");
  cleanSpriteEdgesAggressive(&sADI_OFF_FLAG);

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
  create_display_elements();
}

void render(SaiMessage message) {
  static bool flip = 0;
  flip = !flip;
  LGFX_Sprite* mainSprite = &sMainSprite[flip];

  int ballY = map(message.pitch, 0, 65535, 180, 1580);
  sADI_BALL.setPivot(sADI_BALL.width() / 2, ballY);
  int ballAngle = map(message.bank, 0, 65535, 0, 360);
  int bankIndicatorAngle = map(message.bank, 0, 65535, -60, 60);
  //sBANK_INDICATOR.setPivot(sBANK_INDICATOR.width() / 2, 180);
  int wingsY = map(message.manPitchAdj, 0, 65535, 0, tft.height());
  int pointerHorY = map(message.pointerHor, 0, 65535, 50, 280);
  int pointerVerX = map(message.pointerVer, 0, 65535, 50, 280);
  int slipBallX = map(message.slipBall, 0, 65535, 190, 265);
  int turnRateX = map(message.rateOfTurn, 0, 65535, 197, 263);
  int adiOffFlagAngle = map(message.attWarningFlag, 0, 65535, 0, 20);

  tft.startWrite();

  sADI_BALL.pushRotateZoom(mainSprite, 240, 240, ballAngle, 1, 1);//, 0x00FF00U);
  sADI_WINGS.pushSprite(mainSprite, 110, wingsY, 0x00FF00U);
  sILS_POINTER_H.pushSprite(mainSprite, 80, pointerHorY, 0x00FF00U);
  sILS_POINTER_V.pushSprite(mainSprite, pointerVerX, 100, 0x00FF00U);
  // TODO: optimize bezel inner drawing?
  //sADI_BEZEL_Inner.pushSprite(mainSprite, -1, -1, 0x00FF00U);
  sBANK_INDICATOR.pushRotateZoom(mainSprite, 240, 240, bankIndicatorAngle, 1, 1, 0x00FF00U);
  // TODO: optimize bezel drawing?
  //sADI_BEZEL.pushSprite(mainSprite, -1, -1, 0x00FF00U);
  sADI_BEZEL_Static.pushSprite(mainSprite, -1, -1, 0x00FF00U);
  sADI_OFF_FLAG.pushRotateZoom(mainSprite, 430, 150, adiOffFlagAngle, 1, 1, 0x00FF00U);
  sADI_SLIP_BALL.pushSprite(mainSprite, slipBallX, 413, 0x00FF00U);
  sTURN_RATE.pushSprite(mainSprite, turnRateX, 447, 0x00FF00U);

  static std::uint32_t sec, psec;
  static std::uint32_t fps = 0, frame_count = 0;
  mainSprite->setCursor(100, 100);
  mainSprite->setTextColor(TFT_WHITE);
  mainSprite->printf("fps:%d", fps);
  mainSprite->pushSprite(&tft, 0, 0);
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