#include <FS.h>
#include <LittleFS.h>
#include "renderer.h"
#include "display_driver.h"

// Create tft screen
LGFX tft;

// Create sprites
LGFX_Sprite sMainSprite[2];
LGFX_Sprite sADI_BALL;               // ADI BALL
LGFX_Sprite sADI_BEZEL_MOST_INNER;   // ADI INNER RING
LGFX_Sprite sADI_BEZEL_Inner;        // ADI BEZEL
LGFX_Sprite sADI_OFF_FLAG;           // ADI OFF FLAG
LGFX_Sprite sADI_BEZEL;              // ADI BEZEL
LGFX_Sprite sADI_SLIP_BEZEL;         // ADI SLIP BEZEL
LGFX_Sprite sADI_SLIP_BALL;          // ADI SLIP BALL
LGFX_Sprite sADI_WINGS;              // ADI WINGS
LGFX_Sprite sILS_POINTER_H;          // ILS HORIZONTAL POINTER
LGFX_Sprite sILS_POINTER_V;          // ILS VERTICAL POINTER
LGFX_Sprite sTURN_RATE;              // TURN RATE INDICATOR
LGFX_Sprite sWARNING_FLAG;           // WARNING FLAG
LGFX_Sprite sBANK_INDICATOR;

uint8_t colordepth = 16;

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

  sADI_BEZEL.setPsram(true);
  sADI_BEZEL.setColorDepth(colordepth);
  sADI_BEZEL.createFromBmp(LittleFS, "/Bazel_outer_big.bmp");

  sADI_BEZEL_Inner.setPsram(true);
  sADI_BEZEL_Inner.setColorDepth(colordepth);
  sADI_BEZEL_Inner.createFromBmp(LittleFS, "/Bazel_inner_big.bmp");

  sADI_BEZEL_MOST_INNER.setPsram(true);
  sADI_BEZEL_MOST_INNER.setColorDepth(colordepth);
  sADI_BEZEL_MOST_INNER.createFromBmp(LittleFS, "/Ring_big.bmp");

  sADI_OFF_FLAG.setPsram(true);
  sADI_OFF_FLAG.setColorDepth(colordepth);
  sADI_OFF_FLAG.createFromBmp(LittleFS, "/WarnFlag_big.bmp");

  sADI_WINGS.setPsram(true);
  sADI_WINGS.setColorDepth(colordepth);
  sADI_WINGS.createFromBmp(LittleFS, "/Wing_big.bmp");

  sBANK_INDICATOR.setPsram(true);
  sBANK_INDICATOR.setColorDepth(colordepth);
  sBANK_INDICATOR.createFromBmp(LittleFS, "/Bank.bmp");

  sADI_SLIP_BALL.setPsram(true);
  sADI_SLIP_BALL.setColorDepth(colordepth);
  sADI_SLIP_BALL.createFromBmp(LittleFS, "/Slip-Ball.bmp");

  sILS_POINTER_H.setPsram(true);
  sILS_POINTER_H.setColorDepth(colordepth);
  sILS_POINTER_H.createFromBmp(LittleFS, "/ILS-H.bmp");

  sILS_POINTER_V.setPsram(true);
  sILS_POINTER_V.setColorDepth(colordepth);
  sILS_POINTER_V.createFromBmp(LittleFS, "/ILS-V.bmp");

  sTURN_RATE.setPsram(true);
  sTURN_RATE.setColorDepth(colordepth);
  sTURN_RATE.createFromBmp(LittleFS, "/Slip.bmp");
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

  sADI_BALL.pushRotateZoom(mainSprite, 240, 240, ballAngle, 1, 1, 0x00FF00U);
  sADI_WINGS.pushSprite(mainSprite, 110, wingsY, 0x00FF00U);
  sILS_POINTER_H.pushSprite(mainSprite, 80, pointerHorY, 0x00FF00U);
  sILS_POINTER_V.pushSprite(mainSprite, pointerVerX, 100, 0x00FF00U);
  // TODO: optimize bezel inner drawing?
  sADI_BEZEL_Inner.pushSprite(mainSprite, -1, -1, 0x00FF00U);
  sADI_SLIP_BALL.pushSprite(mainSprite, slipBallX, 413, 0x00FF00U);
  sBANK_INDICATOR.pushRotateZoom(mainSprite, 240, 240, bankIndicatorAngle, 1, 1, 0x00FF00U);
  sADI_OFF_FLAG.pushRotateZoom(mainSprite, 430, 150, adiOffFlagAngle, 1, 1, 0x00FF00U);
  // TODO: optimize bezel drawing?
  sADI_BEZEL.pushSprite(mainSprite, -1, -1, 0x00FF00U);
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