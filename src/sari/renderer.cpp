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

// Create data structure for display elements
struct display_element {
  int sprite_width;
  int sprite_height;
  int pos_x;
  int pos_y;
  int angel;
  LGFX_Sprite* sprite;
};

// Enumeration of display elements
enum Display_Name {
  ADI_BEZEL,
  ADI_BALL,
  ADI_BEZEL_INNER,
  ADI_BEZEL_MOST_INNER,
  ADI_OFF_FLAG,
  ADI_WINGS,
  ADI_SLIP_BEZEL,
  ILS_POINTER_H,
  ILS_POINTER_V,
  SLIP_BALL,
  TURN_RATE,
  WARNING_FLAG,
  BANK_INDICATOR
};

uint8_t colordepth = 16;

//################ Configure Display elelments ###############################
int offset_x = 0;
int offset_y = 0;

display_element display_elements[]= {
//{  w,  h,          px,             py, angel, sprite }
  { 76, 38, offset_x - 1,    offset_y - 1,   0, &sADI_BEZEL }, //ADI_BEZEL
  { 76, 38, offset_x + 240,  offset_y + 864, 0, &sADI_BALL }, //ADI_BALL
  { 58, 18, offset_x - 1,    offset_y - 1,   0, &sADI_BEZEL_Inner }, //ADI_INNER BEZEL
  { 58, 18, offset_x + 180,  offset_y + 31,  0, &sADI_BEZEL_MOST_INNER }, //ADI_BEZEL_MOST_INNER
  {108, 38, offset_x + 430,  offset_y + 150, 0, &sADI_OFF_FLAG }, //ADI_OFF_FLAG
  {108, 38, offset_x + 110,  offset_y + 232, 0, &sADI_WINGS }, //ADI_WINGS
  { 58, 18, offset_x + 180,  offset_y + 96,  0, &sADI_SLIP_BEZEL }, //ADI_SLIP_BEZEL
  { 58, 18, offset_x + 80,   offset_y + 100, 0, &sILS_POINTER_H }, //ILS_POINTER Horizontal
  { 58, 18, offset_x + 100,  offset_y + 100, 0, &sILS_POINTER_V }, //ILS_POINTER Vertical
  { 58, 18, offset_x + 240,  offset_y + 413, 0, &sADI_SLIP_BALL }, //SLIP BALL
  { 58, 18, offset_x + 232,  offset_y + 447, 0, &sTURN_RATE }, //TURN_RATE
  { 58, 18, offset_x + 180,  offset_y + 96,  0, &sWARNING_FLAG }, //WARNING_FLAG
  { 58, 18, offset_x + 240,  offset_y + 100, 0, &sBANK_INDICATOR }, //sBANK_INDICATOR
};

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

  display_elements[ADI_BALL].pos_y = map(message.pitch, 0, 65535, 180, 1580);
  sADI_BALL.setPivot(sADI_BALL.width() / 2, display_elements[ADI_BALL].pos_y);
  display_elements[ADI_BALL].angel = map(message.bank, 0, 65535, 0, 360);
  display_elements[BANK_INDICATOR].angel = map(message.bank, 0, 65535, -60, 60);
  //sBANK_INDICATOR.setPivot(sBANK_INDICATOR.width() / 2, 180);
  display_elements[ADI_WINGS].pos_y = map(message.manPitchAdj, 0, 65535, 0, tft.height());
  display_elements[ILS_POINTER_H].pos_y = map(message.pointerHor, 0, 65535, 50, 280);
  display_elements[ILS_POINTER_V].pos_x = map(message.pointerVer, 0, 65535, 50, 280);
  display_elements[SLIP_BALL].pos_x = map(message.slipBall, 0, 65535, 190, 265);
  display_elements[TURN_RATE].pos_x = map(message.rateOfTurn, 0, 65535, 197, 263);
  display_elements[ADI_OFF_FLAG].angel = map(message.attWarningFlag, 0, 65535, 0, 20);

  tft.startWrite();

  sADI_BALL.pushRotateZoom(&sMainSprite[flip], 240, 240, display_elements[ADI_BALL].angel, 1, 1, 0x00FF00U);
  sADI_WINGS.pushSprite(&sMainSprite[flip], display_elements[ADI_WINGS].pos_x, display_elements[ADI_WINGS].pos_y, 0x00FF00U);
  sILS_POINTER_H.pushSprite(&sMainSprite[flip], display_elements[ILS_POINTER_H].pos_x, display_elements[ILS_POINTER_H].pos_y, 0x00FF00U);
  sILS_POINTER_V.pushSprite(&sMainSprite[flip], display_elements[ILS_POINTER_V].pos_x, display_elements[ILS_POINTER_V].pos_y, 0x00FF00U);
  sADI_BEZEL_Inner.pushSprite(&sMainSprite[flip], display_elements[ADI_BEZEL_INNER].pos_x, display_elements[ADI_BEZEL_INNER].pos_y, 0x00FF00U);
  sADI_SLIP_BALL.pushSprite(&sMainSprite[flip], display_elements[SLIP_BALL].pos_x, display_elements[SLIP_BALL].pos_y, 0x00FF00U);
  sBANK_INDICATOR.pushRotateZoom(&sMainSprite[flip], 240, 240, display_elements[BANK_INDICATOR].angel, 1, 1, 0x00FF00U);
  sADI_OFF_FLAG.pushRotateZoom(&sMainSprite[flip], display_elements[ADI_OFF_FLAG].pos_x, display_elements[ADI_OFF_FLAG].pos_y, display_elements[ADI_OFF_FLAG].angel, 1, 1, 0x00FF00U);
  sADI_BEZEL.pushSprite(&sMainSprite[flip], display_elements[ADI_BEZEL].pos_x, display_elements[ADI_BEZEL].pos_y, 0x00FF00U);
  sTURN_RATE.pushSprite(&sMainSprite[flip], display_elements[TURN_RATE].pos_x, display_elements[TURN_RATE].pos_y, 0x00FF00U);

  static std::uint32_t sec, psec;
  static std::uint32_t fps = 0, frame_count = 0;
  sMainSprite[flip].setCursor(100, 100);
  sMainSprite[flip].setTextColor(TFT_WHITE);
  sMainSprite[flip].printf("fps:%d", fps);
  sMainSprite[flip].pushSprite(&tft, 0, 0);
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