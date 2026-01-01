#include <FS.h>
#include <LittleFS.h>
#include "renderer.h"
#include "display_driver.h"

// Create tft screen
LGFX tft;

// Create sprites

#ifdef DOUBLE_FRAME_BUFFER
	LGFX_Sprite sMainSprite[2];
#else
	LGFX_Sprite sMainSprite[2];
#endif
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
struct display_element{
  int sprite_width;
  int sprite_height;
  int pos_x;
  int pos_y;
  int angel;
  LGFX_Sprite* sprite;
};

// Enumeration of display elements
enum Display_Name{
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
bool fail = false;

//################ Configure Display elelments ###############################
//{width,hight, posx, posy, textalign, sprite, value}
int offset_x = 0;
int offset_y = 0;

display_element display_elements[]= {
//{  w,  h, px, py,a, sprite,   v }
  { 76, 38, -1 + offset_x, -1 + offset_y,0,&sADI_BEZEL}, //ADI_BEZEL
  { 76, 38,240 + offset_x, 864 + offset_y,0,&sADI_BALL}, //ADI_BALL
  { 58, 18,-1 + offset_x, -1 + offset_y,0,&sADI_BEZEL_Inner}, //ADI_INNER BEZEL
  { 58, 18,180 + offset_x, 31 + offset_y,0,&sADI_BEZEL_MOST_INNER}, //ADI_BEZEL_MOST_INNER
  {108, 38, 430 + offset_x, 150 + offset_y,0,&sADI_OFF_FLAG}, //ADI_OFF_FLAG
  {108, 38,110 + offset_x, 232 + offset_y,0,&sADI_WINGS}, //ADI_WINGS
  { 58, 18,180 + offset_x, 96 + offset_y,0,&sADI_SLIP_BEZEL}, //ADI_SLIP_BEZEL
  { 58, 18,80 + offset_x, 100 + offset_y,0,&sILS_POINTER_H}, //ILS_POINTER Horizontal
  { 58, 18,100 + offset_x, 100 + offset_y,0,&sILS_POINTER_V}, //ILS_POINTER Vertical
  { 58, 18,240 + offset_x, 413 + offset_y,0,&sADI_SLIP_BALL}, //SLIP BALL
  { 58, 18,232 + offset_x, 447 + offset_y,0,&sTURN_RATE}, //TURN_RATE
  { 58, 18,180 + offset_x, 96 + offset_y,0,&sWARNING_FLAG}, //WARNING_FLAG
  { 58, 18,240 + offset_x, 100 + offset_y,0,&sBANK_INDICATOR}, //sBANK_INDICATOR
};


//################ Create sprites ###############################


//create sprites for digital display areas and text lables; Fonts loaded from LittleFS
void create_display_elements(){
  //tft.setRotation(90);
//#ifdef DOUBLE_FRAME_BUFFER
  for (int i=0; i < 2; i++){
    sMainSprite[i].setPsram(true);
    sMainSprite[i].setColorDepth(colordepth);
    sMainSprite[i].createSprite(480,480);
  }
  /*
#else
  sMainSprite.setPsram(true);
  sMainSprite.setColorDepth(colordepth);
  sMainSprite.createSprite(480,480);
#endif
*/
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

  //sWARNING_FLAG.setColorDepth(colordepth);
  //sWARNING_FLAG.createFromBmp(LittleFS, "ADI_WARNING_FLAG.bmp");

}

/*
// Update digital and label sprites and print them on the screen
void update_element(int element){
  display_elements[element].sprite->clear();
  display_elements[element].sprite->setPivot(display_elements[element].sprite_width/2,display_elements[element].pos_y);
  display_elements[element].sprite->pushRotateZoomWithAA(display_elements[element].pos_x,display_elements[element].pos_y,display_elements[element].angel,1);
  //display_elements[element].sprite->pushSprite(display_elements[element].pos_x,display_elements[element].pos_y);

}

//################## Start DCS-BIOS routines ####################


//################## SAI PITCH / BANK  ##################
void onSaiPitchChange(unsigned int newValue) {
  //-(BALL Image Height - Display Hight / 2) = zero y position @32.768
  //TODO calculate and set pivot point
  display_elements[ADI_BALL].pos_y = map(newValue,0,65535,-1340, 35);
  update_element(ADI_BALL);

}
DcsBios::IntegerBuffer saiPitchBuffer(FA_18C_hornet_SAI_PITCH, onSaiPitchChange);

void onSaiBankChange(unsigned int newValue) {
  display_elements[ADI_BALL].angel = map(newValue,0,65535,0, 360);
  update_element(ADI_BALL);

}
DcsBios::IntegerBuffer saiBankBuffer(FA_18C_hornet_SAI_BANK, onSaiBankChange);

//################## SAI Manual Pitch Adjustment  ##################

void onSaiWingPitchChange(unsigned int newValue) {

    display_elements[ADI_WINGS].pos_y = map(newValue,0,65535,0, tft.height());
    update_element(ADI_WINGS);
}
DcsBios::IntegerBuffer saiWingPitchBuffer(FA_18C_hornet_SAI_MAN_PITCH_ADJ, onSaiWingPitchChange);

// ################## SARI ILS POINTER   ##################
//Horizontal Pointer
void onSaiPointerHorChange(unsigned int newValue) {


    display_elements[ILS_POINTER_H].pos_y = map(newValue,0,65535,0, tft.height());
    sILS_POINTER.drawFastHLine(display_elements[ILS_POINTER_H].pos_x,display_elements[ILS_POINTER_H].pos_y,display_elements[ILS_POINTER_H].sprite_width);
    update_element(ILS_POINTER_H);

}
DcsBios::IntegerBuffer saiPointerHorBuffer(FA_18C_hornet_SAI_POINTER_HOR, onSaiPointerHorChange);

//Vertical Pointer
void onSaiPointerVerChange(unsigned int newValue) {

    display_elements[ILS_POINTER_V].pos_x = map(newValue,0,65535,0, tft.width());
    sILS_POINTER.drawFastVLine(display_elements[ILS_POINTER_V].pos_x,display_elements[ILS_POINTER_V].pos_y,display_elements[ILS_POINTER_V].sprite_height);
    update_element(ILS_POINTER_V);

}
DcsBios::IntegerBuffer saiPointerVerBuffer(FA_18C_hornet_SAI_POINTER_VER, onSaiPointerVerChange);

// ################## SAI Slip Ball   ##################

void onSaiSlipBallChange(unsigned int newValue) {

    display_elements[SLIP_BALL].pos_x = map(newValue,0,65535,0, tft.width() / 3);
    update_element(SLIP_BALL);


}
DcsBios::IntegerBuffer saiSlipBallBuffer(FA_18C_hornet_SAI_SLIP_BALL, onSaiSlipBallChange);

// ################## SAI Rate Of Turn   ##################

void onSaiRateOfTurnChange(unsigned int newValue) {

    display_elements[TURN_RATE].pos_x = map(newValue,0,65535,0, tft.width() / 3);
    update_element(TURN_RATE);

}
DcsBios::IntegerBuffer saiRateOfTurnBuffer(FA_18C_hornet_SAI_RATE_OF_TURN, onSaiRateOfTurnChange);

// ################## SAI Attitude Warning Flag   ##################

void onSaiAttWarningFlagChange(unsigned int newValue) {
    //TODO Disable Warning Flag
    if (newValue){
      update_element(WARNING_FLAG);
    }

}
DcsBios::IntegerBuffer saiAttWarningFlagBuffer(FA_18C_hornet_SAI_ATT_WARNING_FLAG, onSaiAttWarningFlagChange);
*/

bool spiffenabled = false;

void initRenderer() {
  delay(200);

  panel_power_and_reset();
  st7701_init_rgb565();

  tft.begin();
  tft.setColorDepth(colordepth);
  tft.setSwapBytes(false);
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);

  if(!LittleFS.begin(true)) {
    Serial.println("An Error has occurred while mounting LittleFS");
  } else {
    spiffenabled = true;
  }

  create_display_elements();
  //tft.fillScreen(0x000000U);
  //tft.fillScreen(tft.color888(141,76,71));
  //sADI_BALL[0].setPivot(sADI_BALL[0].width() / 2, sADI_BALL[0].height() / 2);
  sADI_BALL.pushRotateZoom(&tft,100,0,90,2,2);
  sADI_BALL.pushSprite(&tft,150,0);
  //sMainSprite.fillScreen(0xFF0000U);
  //sMainSprite.drawRect(240, 240, 50, 50, 0x00FF00U);
  //sMainSprite.pushSprite(&tft,0,0);
  sBANK_INDICATOR.setPivot(sBANK_INDICATOR.width() /2 ,(sBANK_INDICATOR.height() / 2) + 155);
  sADI_OFF_FLAG.setPivot(8,10);
}

bool first = true;
int slip_x = 240;
int turn_x = 240;
int ball_y = 240;
int ils_h_y = 50;
int ils_v_x = 50;
int ball_angel = 0;
int offFlag_angel = 0;
bool ball_inc = true;
bool offFlag_inc = true;
bool slip_inc = true;
bool turn_inc = true;
bool ils_h_inc = true;
bool ils_v_inc = true;
bool bank_inc = true;
unsigned long update = 0;
unsigned long tim = 0;

void update_positions(){
  //Update positions
    // Ball angel
    display_elements[ADI_BALL].angel+=5;
    // Ball y position
    if ( display_elements[ADI_BALL].pos_y < 1580 && ball_inc){
      display_elements[ADI_BALL].pos_y+=10;
    }else{
      ball_inc = false;
      display_elements[ADI_BALL].pos_y-=10;
      if (display_elements[ADI_BALL].pos_y < 180 ){
        ball_inc = true;
      }
    }
    // Flag Angel
    if ( display_elements[ADI_OFF_FLAG].angel < 20 && offFlag_inc){
      display_elements[ADI_OFF_FLAG].angel+=1;
    }else{
      offFlag_inc = false;
      display_elements[ADI_OFF_FLAG].angel-=1;
      if ( display_elements[ADI_OFF_FLAG].angel == 0 ){
        offFlag_inc = true;
      }
    }
    // Slip Ball X
    if ( display_elements[SLIP_BALL].pos_x < 265 && slip_inc){
      display_elements[SLIP_BALL].pos_x+=3;
    }else{
      slip_inc = false;
      display_elements[SLIP_BALL].pos_x-=3;
      if ( display_elements[SLIP_BALL].pos_x < 190 ){
        slip_inc = true;
      }
    }
     // Turn Rate X
    if ( display_elements[TURN_RATE].pos_x < 263 && turn_inc){
      display_elements[TURN_RATE].pos_x+=3;
    }else{
      turn_inc = false;
      display_elements[TURN_RATE].pos_x-=3;
      if ( display_elements[TURN_RATE].pos_x < 197 ){
        turn_inc = true;
      }
    }
    // ILS Horizontal Y
    if ( display_elements[ILS_POINTER_H].pos_y < 280 && ils_h_inc){
      display_elements[ILS_POINTER_H].pos_y+=1;
    }else{
      ils_h_inc = false;
      display_elements[ILS_POINTER_H].pos_y-=1;
      if ( display_elements[ILS_POINTER_H].pos_y == 50 ){
        ils_h_inc = true;
      }
    }
    // ILS Vertical Y
      if ( display_elements[ILS_POINTER_V].pos_x < 280 && ils_v_inc){
        display_elements[ILS_POINTER_V].pos_x+=1;
      }else{
        ils_v_inc = false;
        display_elements[ILS_POINTER_V].pos_x-=1;
        if ( display_elements[ILS_POINTER_V].pos_x == 50 ){
          ils_v_inc = true;
        }
      }
    // Bank Indicator angel
      if ( display_elements[BANK_INDICATOR].angel < 60 && bank_inc){
        display_elements[BANK_INDICATOR].angel+=1;
      }else{
        bank_inc = false;
        display_elements[BANK_INDICATOR].angel-=1;
        if (display_elements[BANK_INDICATOR].angel == -60 ){
          bank_inc = true;
        }
      }

}

void render(SaiMessage message) {
  static std::uint32_t sec, psec;
  static std::uint32_t fps = 0, frame_count = 0;
  static bool flip = 0;

  flip = flip ? 0 : 1;

  sADI_BALL.setPivot(sADI_BALL.width() / 2, display_elements[ADI_BALL].pos_y);


#ifdef DOUBLE_FRAME_BUFFER
  sADI_BALL.pushRotateZoom(&sMainSprite[flip],240,240,display_elements[ADI_BALL].angel,1,1,0x00FF00U);
  sADI_BEZEL_Inner.pushSprite(&sMainSprite[flip],display_elements[ADI_BEZEL_INNER].pos_x,display_elements[ADI_BEZEL_INNER].pos_y,0x00FF00U);
  //sADI_SLIP_BALL.pushSprite(&sMainSprite[flip],display_elements[SLIP_BALL].pos_x,display_elements[SLIP_BALL].pos_y,0x00FF00U);
  //sBANK_INDICATOR.pushRotateZoom(&sMainSprite[flip],240,240,display_elements[BANK_INDICATOR].angel,1,1,0x00FF00U);
  sADI_OFF_FLAG.pushRotateZoom(&sMainSprite[flip],display_elements[ADI_OFF_FLAG].pos_x,display_elements[ADI_OFF_FLAG].pos_y,display_elements[ADI_OFF_FLAG].angel,1,1,0x00FF00U);
  //sILS_POINTER_H.pushSprite(&sMainSprite[flip],display_elements[ILS_POINTER_H].pos_x,display_elements[ILS_POINTER_H].pos_y,0x00FF00U);
  //sILS_POINTER_V.pushSprite(&sMainSprite[flip],display_elements[ILS_POINTER_V].pos_x,display_elements[ILS_POINTER_V].pos_y,0x00FF00U);
  sADI_BEZEL.pushSprite(&sMainSprite[flip],display_elements[ADI_BEZEL].pos_x,display_elements[ADI_BEZEL].pos_y,0x00FF00U);
  //sTURN_RATE.pushSprite(&sMainSprite[flip],display_elements[TURN_RATE].pos_x,display_elements[TURN_RATE].pos_y,0x00FF00U);
  sADI_WINGS.pushSprite(&sMainSprite[flip],display_elements[ADI_WINGS].pos_x,display_elements[ADI_WINGS].pos_y,0x00FF00U);
  sMainSprite[flip].setCursor(201,201);
  sMainSprite[flip].setTextColor(TFT_BLACK);
  sMainSprite[flip].printf("fps:%d",  fps);
  sMainSprite[flip].setCursor(200,200);
  sMainSprite[flip].setTextColor(TFT_WHITE);
  sMainSprite[flip].printf("fps:%d", fps);
    //Draw final result
  sMainSprite[flip].pushSprite(&tft,0,0);
#else
  tft.startWrite();
  //sprite->clear();

  sADI_BALL.pushRotateZoom(&sMainSprite[flip],240,240,display_elements[ADI_BALL].angel,1,1,0x00FF00U);

  sADI_WINGS.pushSprite(&sMainSprite[flip],display_elements[ADI_WINGS].pos_x,display_elements[ADI_WINGS].pos_y,0x00FF00U);

  sILS_POINTER_H.pushSprite(&sMainSprite[flip],display_elements[ILS_POINTER_H].pos_x,display_elements[ILS_POINTER_H].pos_y,0x00FF00U);
  sILS_POINTER_V.pushSprite(&sMainSprite[flip],display_elements[ILS_POINTER_V].pos_x,display_elements[ILS_POINTER_V].pos_y,0x00FF00U);
  sADI_BEZEL_Inner.pushSprite(&sMainSprite[flip],display_elements[ADI_BEZEL_INNER].pos_x,display_elements[ADI_BEZEL_INNER].pos_y,0x00FF00U);
  sADI_SLIP_BALL.pushSprite(&sMainSprite[flip],display_elements[SLIP_BALL].pos_x,display_elements[SLIP_BALL].pos_y,0x00FF00U);
  sBANK_INDICATOR.pushRotateZoom(&sMainSprite[flip],240,240,display_elements[BANK_INDICATOR].angel,1,1,0x00FF00U);
  sADI_OFF_FLAG.pushRotateZoom(&sMainSprite[flip],display_elements[ADI_OFF_FLAG].pos_x,display_elements[ADI_OFF_FLAG].pos_y,display_elements[ADI_OFF_FLAG].angel,1,1,0x00FF00U);

  sADI_BEZEL.pushSprite(&sMainSprite[flip],display_elements[ADI_BEZEL].pos_x,display_elements[ADI_BEZEL].pos_y,0x00FF00U);

  sTURN_RATE.pushSprite(&sMainSprite[flip],display_elements[TURN_RATE].pos_x,display_elements[TURN_RATE].pos_y,0x00FF00U);

  sMainSprite[flip].setCursor(110,110);
  sMainSprite[flip].setTextColor(TFT_BLACK);
  sMainSprite[flip].printf("fps:%d",  fps);
  sMainSprite[flip].setCursor(100,100);
  sMainSprite[flip].setTextColor(TFT_WHITE);
  sMainSprite[flip].printf("fps:%d", fps);
    //Draw final result
  //tft.startWrite();
  sMainSprite[flip].pushSprite(&tft,0,0);
  tft.endWrite();
  //  diffDraw(&sMainSprite[flip], &sMainSprite[!flip]);

#endif

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