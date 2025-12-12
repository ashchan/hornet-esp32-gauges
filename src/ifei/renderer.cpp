#include <FS.h>
#include <LittleFS.h>
#include <lvgl.h>
#include <cctype>
#include <cstring>
#include "display_driver.h"
#include "renderer.h"
#include "message.h"

const unsigned int COLOR_DAY = 0xFFFFFFU;
const unsigned int COLOR_NIGHT = 0x1CDD2AU;
unsigned int ifeiColor = COLOR_DAY;
unsigned int ifeiBrightness = 0;
bool forceUpdate = false;

// Create tft screen 
LGFX tft;

// Create sprites
LGFX_Sprite twoDigitSprite(&tft);   // Data digits sprite (RPM/OIL)
LGFX_Sprite threeDigitSprite(&tft); // Data digits sprite (TEMP/FF)
LGFX_Sprite labelSprite(&tft); // Text label sprite 
LGFX_Sprite fuelSprite(&tft);   // Fuel sprite
LGFX_Sprite clockSprite(&tft);  // Clock sprite

LGFX_Sprite leftNozzleSprites[27] // Left nozzle gauge sprites
    {LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft),
     LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft),
     LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft),
     LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft),
     LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft),
     LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft),
     LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft)};

LGFX_Sprite rightNozzleSprites[27] // Right nozzle gauge sprites
    {LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft),
     LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft),
     LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft),
     LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft),
     LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft),
     LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft),
     LGFX_Sprite(&tft), LGFX_Sprite(&tft), LGFX_Sprite(&tft)};

LGFX_Sprite tagSprite(&tft); //Special Tags sprite (L/R/Z)

enum TextAlignment {
  TextAlignmentLeft = 0,
  TextAlignmentCenter = 1,
  TextAlignmentRight = 2
};

// Create data structure for display elements
struct DisplayElement {
  int spriteWidth;
  int spriteHeight;
  int posX;
  int posY;
  TextAlignment textalign;
  LGFX_Sprite* sprite;
  char value[8];
};

// Enumeration of display elements
enum DisplayName {
  RPML,
  RPMR,
  RPMT,
  TMPL,
  TMPR,
  TMPT,
  FFL,
  FFR,
  FFTU,
  FFTL,
  OILL,
  OILR,
  OILT,
  NOZL,
  NOZR,
  NOZT,
  FUELU,
  FUELL,
  BINGO,
  BINGOT,
  CLOCKU,
  CLOCKL,
  ZULU,
  L,
  R,
};

//################ Configure Display elelments ###############################
//{width,hight, posx, posy, textalign, sprite, value} 
int globalOffsetX = 30;
int globalOffsetY = 10;
DisplayElement displayElements[] = {
  //{  w,  h, px, py,a, sprite,   v }
  {76, 38, 92 + globalOffsetX, 20 + globalOffsetY, TextAlignmentRight, &twoDigitSprite, "12"},  // RPML
  {76, 38, 246 + globalOffsetX, 20 + globalOffsetY, TextAlignmentRight, &twoDigitSprite, "34"}, // RPMR
  {58, 18, 180 + globalOffsetX, 31 + globalOffsetY, TextAlignmentCenter, &labelSprite, "RPM"},   // RPMT
  {108, 38, 60 + globalOffsetX, 85 + globalOffsetY, TextAlignmentRight, &threeDigitSprite, "567"}, // TMPL
  {108, 38, 246 + globalOffsetX, 85 + globalOffsetY, TextAlignmentRight, &threeDigitSprite, "890"}, // TMPR
  {58, 18, 180 + globalOffsetX, 96 + globalOffsetY, TextAlignmentCenter, &labelSprite, "TEMP"}, // TMPT
  {108, 38, 60 + globalOffsetX, 160 + globalOffsetY, TextAlignmentRight, &threeDigitSprite, "123"}, // FFL
  {108, 38, 246 + globalOffsetX, 160 + globalOffsetY, TextAlignmentRight, &threeDigitSprite, "456"}, // FFR
  {58, 18, 180 + globalOffsetX, 171 + globalOffsetY, TextAlignmentCenter, &labelSprite, "FF"},    // FFTU
  {65, 18, 180 + globalOffsetX, 188 + globalOffsetY, TextAlignmentCenter, &labelSprite, "X100"},  // FFTL
  {76, 38, 92 + globalOffsetX, 400 + globalOffsetY, TextAlignmentRight, &twoDigitSprite, "78"},  // OILL
  {76, 38, 246 + globalOffsetX, 400 + globalOffsetY, TextAlignmentRight, &twoDigitSprite, "90"}, // OILR
  {58, 18, 180 + globalOffsetX, 415 + globalOffsetY, TextAlignmentCenter, &labelSprite, "OIL"},   // OILT
  {150, 154, 58 + globalOffsetX, 230 + globalOffsetY, TextAlignmentLeft, &leftNozzleSprites[0], "0"}, // NOZL
  {150, 154, 211 + globalOffsetX, 230 + globalOffsetY, TextAlignmentLeft, &rightNozzleSprites[0], "0"}, // NOZR
  {58, 18, 180 + globalOffsetX, 300 + globalOffsetY, TextAlignmentCenter, &labelSprite, "NOZ"},  // NOZT
  {176, 38, 560 + globalOffsetX, 30 + globalOffsetY, TextAlignmentRight, &fuelSprite, "12345"}, // FUELU
  {176, 38, 560 + globalOffsetX, 85 + globalOffsetY, TextAlignmentRight, &fuelSprite, "67890"}, // FUELL
  {176, 38, 560 + globalOffsetX, 215 + globalOffsetY, TextAlignmentRight, &fuelSprite, "500"},  // BINGO
  {58, 18, 625 + globalOffsetX, 185 + globalOffsetY, TextAlignmentCenter, &labelSprite, "BINGO"}, // BINGOT
  {176, 35, 570 + globalOffsetX, 350 + globalOffsetY, TextAlignmentRight, &clockSprite, ""}, // CLOCKU
  {176, 35, 570 + globalOffsetX, 415 + globalOffsetY, TextAlignmentRight, &clockSprite, ""}, // CLOCKL
  {18, 18, 746 + globalOffsetX, 367 + globalOffsetY, TextAlignmentCenter, &tagSprite, "Z"}, // ZULU Tag
  {18, 18, 736 + globalOffsetX, 50 + globalOffsetY, TextAlignmentCenter, &tagSprite, "L"},  // L Tag
  {18, 18, 736 + globalOffsetX, 105 + globalOffsetY, TextAlignmentCenter, &tagSprite, "R"}, // R Tag
};

//################ Create sprites ###############################

//Create a sprite for each possible pointer nozzel pointer position from an image, located within LITTLEFS and store it in Psram
//additionl sprites for scale and scale numbers as well as a blank sprite
void createImageSprite() {
  int j = 0;
  //Left Nozzel White 
  for (int i = 0; i <= 120; i += 10){
    String filename = "/White/L" + String(i) + ".bmp";
    leftNozzleSprites[j].setPsram(true);
    leftNozzleSprites[j].setColorDepth(24);
    leftNozzleSprites[j].createFromBmp(LittleFS, filename.c_str());
 
    j++;
  }
  //Left Nozzel Green
  for (int i = 0; i <= 120; i += 10){
    String filename = "/Green/L" + String(i) + ".bmp";
    leftNozzleSprites[j].setPsram(true);
    leftNozzleSprites[j].setColorDepth(24);
    leftNozzleSprites[j].createFromBmp(LittleFS, filename.c_str());
    j++;
  }
  //Black sprite to hide nozzel gauge
  leftNozzleSprites[j].setPsram(true);
  leftNozzleSprites[j].setColorDepth(24);
  leftNozzleSprites[j].createSprite(displayElements[NOZL].spriteWidth, displayElements[NOZL].spriteHeight);
  leftNozzleSprites[j].fillScreen(0x000000U);
  
  j = 0;
  //Right Nozzel White 
  for (int k = 0; k <= 120; k += 10){
    String filename = "/White/R" + String(k) + ".bmp";
    rightNozzleSprites[j].setPsram(true);
    rightNozzleSprites[j].setColorDepth(24);
    rightNozzleSprites[j].createFromBmp(LittleFS,filename.c_str());
    j++;
  }
  
  //Right Nozzel Green 
  for (int k = 0; k <= 120; k += 10){
    String filename = "/Green/R" + String(k) + ".bmp";
    rightNozzleSprites[j].setPsram(true);
    rightNozzleSprites[j].setColorDepth(24);
    rightNozzleSprites[j].createFromBmp(LittleFS,filename.c_str());
    j++;
  }
  //Black sprite to hide nozzel gauge
  rightNozzleSprites[j].setPsram(true);
  rightNozzleSprites[j].setColorDepth(24);
  rightNozzleSprites[j].createSprite(displayElements[NOZR].spriteWidth, displayElements[NOZR].spriteHeight);
  rightNozzleSprites[j].fillScreen(0x000000U);
}

//create sprites for digital display areas and text lables; Fonts loaded from littlefs
void createDisplayElements() {
  createImageSprite();

  twoDigitSprite.createSprite(displayElements[RPML].spriteWidth, displayElements[RPML].spriteHeight);
  twoDigitSprite.loadFont(LittleFS,"/Fonts/IFEI-Data-36.vlw");
  twoDigitSprite.setFont(twoDigitSprite.getFont());
  twoDigitSprite.setColorDepth(24);
  twoDigitSprite.setTextWrap(false);
  twoDigitSprite.setTextColor(ifeiColor);
    
  threeDigitSprite.createSprite(displayElements[TMPL].spriteWidth, displayElements[TMPL].spriteHeight);
  threeDigitSprite.loadFont(LittleFS,"/Fonts/IFEI-Data-36.vlw");
  threeDigitSprite.setFont(threeDigitSprite.getFont());
  threeDigitSprite.setColorDepth(24);
  threeDigitSprite.setTextWrap(false);
  threeDigitSprite.setTextColor(ifeiColor);
    
  labelSprite.createSprite(displayElements[RPMT].spriteWidth, displayElements[RPMT].spriteHeight);
  labelSprite.loadFont(LittleFS,"/Fonts/IFEI-Labels-16.vlw");
  labelSprite.setFont(labelSprite.getFont());
  labelSprite.setColorDepth(24);
  labelSprite.setTextColor(ifeiColor);
  
  clockSprite.createSprite(displayElements[CLOCKU].spriteWidth, displayElements[CLOCKU].spriteHeight);
  clockSprite.loadFont(LittleFS,"/Fonts/IFEI-Data-32.vlw");
  clockSprite.setFont(clockSprite.getFont());
  clockSprite.setColorDepth(24);
  clockSprite.setTextWrap(false);
  clockSprite.setTextColor(ifeiColor);
  
  tagSprite.createSprite(displayElements[ZULU].spriteWidth, displayElements[ZULU].spriteHeight);
  tagSprite.loadFont(LittleFS,"/Fonts/IFEI-Labels-16.vlw");
  tagSprite.setFont(labelSprite.getFont());
  tagSprite.print(displayElements[NOZT].value);


  fuelSprite.createSprite(displayElements[FUELU].spriteWidth, displayElements[FUELU].spriteHeight);
  fuelSprite.loadFont(LittleFS,"/Fonts/IFEI-Data-36.vlw");
  fuelSprite.setFont(fuelSprite.getFont());
  fuelSprite.setColorDepth(24);
  fuelSprite.setTextWrap(false);
  fuelSprite.setTextColor(ifeiColor);
}

//Align text within its sprite. 
int setTextAlignment(DisplayElement element, TextAlignment alignment) {
  if (alignment == TextAlignmentRight) {
    return element.spriteWidth - element.sprite->textWidth(element.value);
  } else if (alignment == TextAlignmentCenter) {
    return (element.spriteWidth - element.sprite->textWidth(element.value)) / 2;
  } else {
    return 0;
  }
}

// Update digital and label sprites and print them on the screen
void updateElement(DisplayElement& element) {
  element.sprite->clear();
  element.sprite->setCursor(setTextAlignment(element, element.textalign), 0);
  element.sprite->setTextColor(ifeiColor);
  element.sprite->print(element.value);
  element.sprite->pushSprite(element.posX, element.posY);
}

void copyTrimLeft(char* dst, size_t dstSize, const char* src) {
  if (!dst || dstSize == 0) {
    return;
  }
  if (!src) {
    dst[0] = '\0';
    return;
  }

  while (*src == ' ' || *src == '\t') {
    ++src;
  }

  snprintf(dst, dstSize, "%s", src);
}

void renderNozzleLeft(IfeiMessage message) {
  DisplayElement nozzle = displayElements[NOZL];
  int value = map(message.extNozzlePosL, 0, 65535, 0, 100);
  int spriteOffset = ifeiColor == COLOR_DAY ? 0 : 13;
  if (value < 5) {
    strcpy(nozzle.value, "0");
  } else {
    strcpy(nozzle.value, String((value - 5) / 10 + 1).c_str());
  }
  nozzle.sprite = &leftNozzleSprites[atoi(nozzle.value) + spriteOffset];
  if (message.lPointerTex == 1) {
    nozzle.sprite->pushSprite(nozzle.posX, nozzle.posY, 0x000000U);
  }
  if (message.lScaleTex == 1) {
    leftNozzleSprites[12 + spriteOffset].pushSprite(nozzle.posX, nozzle.posY, 0x000000U);
  }
  if (message.l100Tex == 1) {
    leftNozzleSprites[11 + spriteOffset].pushSprite(nozzle.posX, nozzle.posY, 0x000000U);
  }
}

void renderNozzleRight(IfeiMessage message) {
  DisplayElement nozzle = displayElements[NOZR];
  int value = map(message.extNozzlePosR, 0, 65535, 0, 100);
  int spriteOffset = ifeiColor == COLOR_DAY ? 0 : 13;
  if (value < 5) {
    strcpy(nozzle.value, "0");
  } else {
    strcpy(nozzle.value, String((value - 5) / 10 + 1).c_str());
  }
  nozzle.sprite = &rightNozzleSprites[atoi(nozzle.value) + spriteOffset];
  if (message.rPointerTex == 1) {
    nozzle.sprite->pushSprite(nozzle.posX, nozzle.posY, 0x000000U);
  }
  if (message.rScaleTex == 1) {
    rightNozzleSprites[12 + spriteOffset].pushSprite(nozzle.posX, nozzle.posY, 0x000000U);
  }
  if (message.r100Tex == 1) {
    rightNozzleSprites[11 + spriteOffset].pushSprite(nozzle.posX, nozzle.posY, 0x000000U);
  }
}

void renderClock(DisplayElement element, String& hours, String& minutes, String& seconds, char dp1, char dp2) {
  element.sprite->clear();
  element.sprite->setTextColor(ifeiColor);
  if (hours[0] == 32) {
    element.sprite->setCursor(57, 1);
  } else {
    element.sprite->setCursor(1 + (hours == "0" ? 28 : 0), 1);
    element.sprite->print(hours);
  }
  if (dp1 == 32) {
    element.sprite->setCursor(63, 1);
  } else {
    element.sprite->print(dp1);
  }
  if (minutes[0] == 32) {
    element.sprite->setCursor(119, 1);
  } else {
    element.sprite->print(minutes);
  }
  if (dp2 == 32) {
    element.sprite->setCursor(125, 1);
  } else {
    element.sprite->print(dp2);
  }
  if (seconds[0] == 32) {
    element.sprite->setCursor(181, 1);
  } else {
    element.sprite->print(seconds);
  }

  element.sprite->pushSprite(element.posX, element.posY);
}

void renderClocks(IfeiMessage message) {
  String hours = String(message.clockH);
  if (hours == "") {
  } else if (hours.toInt() < 10) {
    hours = "0" + hours;
  }
  String minutes = String(message.clockM);
  if (minutes == "") {
  } else if (minutes.toInt() < 10) {
    minutes = "0" + minutes;
  }
  String seconds = String(message.clockS);
  if (seconds == "") {
  } else if (seconds.toInt() < 10) {
    seconds = "0" + seconds;
  }
  renderClock(displayElements[CLOCKU], hours, minutes, seconds, (char)message.dd1, (char)message.dd2);

  // TODO: timer could show as " : : "
  hours = String(message.timerH);
  minutes = String(message.timerM);
  if (minutes == "") {
  } else if (minutes.toInt() < 10) {
    minutes = "0" + minutes;
  }
  seconds = String(message.timerS);
  if (seconds == "") {
  } else if (seconds.toInt() < 10) {
    seconds = "0" + seconds;
  }
  renderClock(displayElements[CLOCKL], hours, minutes, seconds, (char)message.dd3, (char)message.dd4);
}

void initIfeiRenderer() {
  tft.begin();

  if (!LittleFS.begin(true)) {
    return;
  }

  createDisplayElements();
  tft.setColorDepth(24);
  tft.fillScreen(0x000000U);
}

void renderIfeiMessage(IfeiMessage message) {
  unsigned int color = message.colorMode == 0 ? COLOR_DAY : COLOR_NIGHT;
  if (color != ifeiColor) {
    ifeiColor = color;
    forceUpdate = true;
  }

  unsigned int brightness = map(message.dispIntLt, 0, 65535, 0, 255);
  if (brightness != ifeiBrightness) {
    ifeiBrightness = brightness;
    tft.setBrightness(ifeiBrightness);
  }

  char value[8];
  unsigned int len = sizeof(value);

  // RPML
  snprintf(value, len, "%u", message.rpmL);
  if (forceUpdate || strcmp(displayElements[RPML].value, value) != 0) {
    strcpy(displayElements[RPML].value, value);
    updateElement(displayElements[RPML]);
  }
  // RPMR
  snprintf(value, len, "%u", message.rpmR);
  if (forceUpdate || strcmp(displayElements[RPMR].value, value) != 0) {
    strcpy(displayElements[RPMR].value, value);
    updateElement(displayElements[RPMR]);
  }
  // RPMT
  const char* rpmTexture = message.rpmTex == 1 ? "RPM" : "   ";
  if (forceUpdate || strcmp(displayElements[RPMT].value, rpmTexture) != 0) {
    strcpy(displayElements[RPMT].value, rpmTexture);
    updateElement(displayElements[RPMT]);
  }
  // TMPL
  snprintf(value, len, "%u", message.tempL);
  if (forceUpdate || strcmp(displayElements[TMPL].value, value) != 0) {
    strcpy(displayElements[TMPL].value, value);
    updateElement(displayElements[TMPL]);
  }
  // TMPR
  snprintf(value, len, "%u", message.tempR);
  if (forceUpdate || strcmp(displayElements[TMPR].value, value) != 0) {
    strcpy(displayElements[TMPR].value, value);
    updateElement(displayElements[TMPR]);
  }
  // TMPT
  const char* tmpTexture = message.tempTex == 1 ? "TEMP" : "    ";
  if (forceUpdate || strcmp(displayElements[TMPT].value, tmpTexture) != 0) {
    strcpy(displayElements[TMPT].value, tmpTexture);
    updateElement(displayElements[TMPT]);
  }
  // FFL
  snprintf(value, len, "%u", message.ffL);
  if (forceUpdate || strcmp(displayElements[FFL].value, value) != 0) {
    strcpy(displayElements[FFL].value, value);
    updateElement(displayElements[FFL]);
  }
  // FFR
  snprintf(value, len, "%u", message.ffR);
  if (forceUpdate || strcmp(displayElements[FFR].value, value) != 0) {
    strcpy(displayElements[FFR].value, value);
    updateElement(displayElements[FFR]);
  }
  // FFTU & FFTL
  const char* ffTexture = message.ffTex == 1 ? "FF" : "";
  if (forceUpdate || strcmp(displayElements[FFTU].value, ffTexture) != 0) {
    strcpy(displayElements[FFTU].value, ffTexture);
    updateElement(displayElements[FFTU]);
  }
  const char* ffScaleTexture = message.ffTex == 1 ? "X100" : "";
  if (forceUpdate || strcmp(displayElements[FFTL].value, ffScaleTexture) != 0) {
    strcpy(displayElements[FFTL].value, ffScaleTexture);
    updateElement(displayElements[FFTL]);
  }
  // OILL
  snprintf(value, len, "%u", message.oilPressL);
  if (forceUpdate || strcmp(displayElements[OILL].value, value) != 0) {
    strcpy(displayElements[OILL].value, value);
    updateElement(displayElements[OILL]);
  }
  // OILR
  snprintf(value, len, "%u", message.oilPressR);
  if (forceUpdate || strcmp(displayElements[OILR].value, value) != 0) {
    strcpy(displayElements[OILR].value, value);
    updateElement(displayElements[OILR]);
  }
  // NOZL
  renderNozzleLeft(message);
  // NOZR
  renderNozzleRight(message);
  // OILT & NOZT
  const char* oilTexture = message.oilTex == 1 ? "OIL" : "   ";
  if (forceUpdate || strcmp(displayElements[OILT].value, oilTexture) != 0) {
    strcpy(displayElements[OILT].value, oilTexture);
    updateElement(displayElements[OILT]);
  }
  const char* nozTexture = message.oilTex == 1 ? "NOZ" : "   ";
  if (forceUpdate || strcmp(displayElements[NOZT].value, nozTexture) != 0) {
    strcpy(displayElements[NOZT].value, nozTexture);
    updateElement(displayElements[NOZT]);
  }
  // FUELU
  copyTrimLeft(value, len, message.fuelUp);
  if (forceUpdate || strcmp(displayElements[FUELU].value, value) != 0) {
    strcpy(displayElements[FUELU].value, value);
    updateElement(displayElements[FUELU]);
  }
  // FUELL
  copyTrimLeft(value, len, message.fuelDown);
  if (forceUpdate || strcmp(displayElements[FUELL].value, value) != 0) {
    strcpy(displayElements[FUELL].value, value);
    updateElement(displayElements[FUELL]);
  }
  // BINGO
  snprintf(value, len, "%u", message.bingo);
  if (forceUpdate || strcmp(displayElements[BINGO].value, value) != 0) {
    strcpy(displayElements[BINGO].value, value);
    updateElement(displayElements[BINGO]);
  }
  // BINGOT
  const char* bingoTexture = message.bingoTex == 1 ? "BINGO" : "     ";
  if (forceUpdate || strcmp(displayElements[BINGOT].value, bingoTexture) != 0) {
    strcpy(displayElements[BINGOT].value, bingoTexture);
    updateElement(displayElements[BINGOT]);
  }
  // CLOCKU & CLOCKL
  renderClocks(message);
  // ZULU
  const char* tagZ = message.zTex == 1 ? "Z" : " ";
  if (forceUpdate || strcmp(displayElements[ZULU].value, tagZ) != 0) {
    strcpy(displayElements[ZULU].value, tagZ);
    updateElement(displayElements[ZULU]);
  }
  // L
  const char* tagL = message.lTex == 1 ? "L" : " ";
  if (forceUpdate || strcmp(displayElements[L].value, tagL) != 0) {
    strcpy(displayElements[L].value, tagL);
    updateElement(displayElements[L]);
  }
  // R
  const char* tagR = message.rTex == 1 ? "R" : " ";
  if (forceUpdate || strcmp(displayElements[R].value, tagR) != 0) {
    strcpy(displayElements[R].value, tagR);
    updateElement(displayElements[R]);
  }
}