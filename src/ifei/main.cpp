#include <Arduino.h>
#include <FS.h>
#include <LittleFS.h>
#include <WiFi.h>
#include <esp_now.h>
#include <lvgl.h>
#include "message.h"
#include "helper.h"

#define LITTLEFS LittleFS

//################ Configure Display elelments ###############################
//{width,hight, posx, posy, textalign, sprite, value} 
int offset_x = 25;
int offset_y = 10;
display_element display_elements[]= {
//{  w,  h, px, py,a, sprite,   v }
  { 76, 38, 92 + offset_x, 20 + offset_y,2,&TWOD,"12"}, //RPML 
  { 76, 38,246 + offset_x, 20 + offset_y,2,&TWOD,"34"}, //RPMR
  { 58, 18,180 + offset_x, 31 + offset_y,1,&LABELS,"RPM"}, //RPMT
  {108, 38, 60 + offset_x, 85 + offset_y,2,&THREED,"567"}, //TMPL
  {108, 38,246 + offset_x, 85 + offset_y,2,&THREED,"890"}, //TMPR
  { 58, 18,180 + offset_x , 96 + offset_y,1,&LABELS,"TEMP"}, //TMPT
  {108, 38, 60 + offset_x,160 + offset_y,2,&THREED,"123"}, //FFL
  {108, 38,246 + offset_x,160 + offset_y,2,&THREED,"456"}, //FFR
  { 58, 18,180 + offset_x,171 + offset_y,1,&LABELS,"FF"}, //FFTU
  { 65, 18,180 + offset_x,188 + offset_y,1,&LABELS,"X100"}, //FFTL
  { 76, 38, 92 + offset_x,400 + offset_y,2,&TWOD,"78"}, //OILL
  { 76, 38,246 + offset_x,400 + offset_y,2,&TWOD,"90"}, //OILR
  { 58, 18,180 + offset_x,415 + offset_y,1,&LABELS,"OIL"}, //OILT
  {150,154, 58 + offset_x,230 + offset_y,0,&NOZL_IMAGE[0],"0"}, //NOZL
  {150,154,211 + offset_x,230 + offset_y,0,&NOZR_IMAGE[0],"0"}, //NOZR
  { 58, 18,180 + offset_x,300 + offset_y,1,&LABELS,"NOZ"}, //NOZT
  {176, 38,560 + offset_x, 30 + offset_y,2,&Fuel,"12345"}, //FUELU
  {176, 38,560 + offset_x, 85 + offset_y,2,&Fuel,"67890"}, //FUELL
  {176, 38,560 + offset_x,215 + offset_y,2,&Fuel,"500"}, //BINGO
  { 58, 18,625 + offset_x,185 + offset_y,1,&LABELS,"BINGO"}, //BINGOT
  {176, 35,570 + offset_x,350 + offset_y,2,&CLOCK,""}, //CLOCKU
  {176, 35,570 + offset_x,415 + offset_y,2,&CLOCK,""}, //CLOCKL
  { 18, 18,746 + offset_x,367 + offset_y,1,&TAG,"Z"}, //ZULU Tag
  { 18, 18,736 + offset_x, 50 + offset_y,1,&TAG,"L"}, //L Tag
  { 18, 18,736 + offset_x,105 + offset_y,1,&TAG,"R"}, //R Tag
};


//################ Create sprites ###############################


//Create a sprite for each possible pointer nozzel pointer position from an image, located within LITTLEFS and store it in Psram
//additionl sprites for scale and scale numbers as well as a blank sprite
void create_image_sprite(){
  int j = 0;
  //Left Nozzel White 
  for (int i = 0; i <= 120; i += 10){
    String filename = "/White/L" + String(i) + ".bmp";
    NOZL_IMAGE[j].setPsram(true);
    NOZL_IMAGE[j].setColorDepth(24);
    NOZL_IMAGE[j].createFromBmp(LITTLEFS, filename.c_str());
    
    j++;
  }
  //Left Nozzel Green
  for (int i = 0; i <= 120; i += 10){
    String filename = "/Green/L" + String(i) + ".bmp";
    NOZL_IMAGE[j].setPsram(true);
    NOZL_IMAGE[j].setColorDepth(24);
    NOZL_IMAGE[j].createFromBmp(LITTLEFS, filename.c_str());
    j++;
  }
  //Black sprite to hide nozzel gauge
  NOZL_IMAGE[j].setPsram(true);
  NOZL_IMAGE[j].setColorDepth(24);
  NOZL_IMAGE[j].createSprite(display_elements[NOZL].sprite_width, display_elements[NOZL].sprite_hight);
  NOZL_IMAGE[j].fillScreen(0x000000U);
  
  j = 0;
  //Right Nozzel White 
  for (int k = 0; k <= 120; k += 10){
    String filename = "/White/R" + String(k) + ".bmp";
    NOZR_IMAGE[j].setPsram(true);
    NOZR_IMAGE[j].setColorDepth(24);
    NOZR_IMAGE[j].createFromBmp(LITTLEFS,filename.c_str());
    j++;
  }
  
  //Right Nozzel Green 
  for (int k = 0; k <= 120; k += 10){
    String filename = "/Green/R" + String(k) + ".bmp";
    NOZR_IMAGE[j].setPsram(true);
    NOZR_IMAGE[j].setColorDepth(24);
    NOZR_IMAGE[j].createFromBmp(LITTLEFS,filename.c_str());
    j++;
  }
  //Black sprite to hide nozzel gauge
  NOZR_IMAGE[j].setPsram(true);
  NOZR_IMAGE[j].setColorDepth(24);
  NOZR_IMAGE[j].createSprite(display_elements[NOZR].sprite_width, display_elements[NOZR].sprite_hight);
  NOZR_IMAGE[j].fillScreen(0x000000U);
  
}

//create sprites for digital display areas and text lables; Fonts loaded from littlefs
void create_display_elements(){
 create_image_sprite();
  
  TWOD.createSprite(display_elements[RPML].sprite_width, display_elements[RPML].sprite_hight);
  TWOD.loadFont(LITTLEFS,"/Fonts/IFEI-Data-36.vlw");
  TWOD.setFont(TWOD.getFont());
  TWOD.setColorDepth(24);
  TWOD.setTextWrap(false);
  TWOD.setTextColor(ifei_color);
    
  THREED.createSprite(display_elements[TMPL].sprite_width, display_elements[TMPL].sprite_hight);
  THREED.loadFont(LITTLEFS,"/Fonts/IFEI-Data-36.vlw");
  THREED.setFont(THREED.getFont());
  THREED.setColorDepth(24);
  THREED.setTextWrap(false);
  THREED.setTextColor(ifei_color);
    
  LABELS.createSprite(display_elements[RPMT].sprite_width, display_elements[RPMT].sprite_hight);
  LABELS.loadFont(LITTLEFS,"/Fonts/IFEI-Labels-16.vlw");
  LABELS.setFont(LABELS.getFont());
  LABELS.setColorDepth(24);
  LABELS.setTextColor(ifei_color);
  
  CLOCK.createSprite(display_elements[CLOCKU].sprite_width, display_elements[CLOCKU].sprite_hight);
  CLOCK.loadFont(LITTLEFS,"/Fonts/IFEI-Data-32.vlw");
  CLOCK.setFont(CLOCK.getFont());
  CLOCK.setColorDepth(24);
  CLOCK.setTextWrap(false);
  CLOCK.setTextColor(ifei_color);
  
  TAG.createSprite(display_elements[ZULU].sprite_width, display_elements[ZULU].sprite_hight);
  TAG.loadFont(LITTLEFS,"/Fonts/IFEI-Labels-16.vlw");
  TAG.setFont(LABELS.getFont());
  TAG.print(display_elements[NOZT].value);


  Fuel.createSprite(display_elements[FUELU].sprite_width, display_elements[FUELU].sprite_hight);
  Fuel.loadFont(LITTLEFS,"/Fonts/IFEI-Data-36.vlw");
  Fuel.setFont(Fuel.getFont());
  Fuel.setColorDepth(24);
  Fuel.setTextWrap(false);
  Fuel.setTextColor(ifei_color);
}

//Align text within it's sprite. 
//alignment 0=left; 1=middle; 2=right  
int set_textalignment(int element,int alignment){
  if (alignment == 2){ 
      return (display_elements[element].sprite_width - display_elements[element].sprite->textWidth(display_elements[element].value)); 
  }else if (alignment == 1){
    return (display_elements[element].sprite_width - display_elements[element].sprite->textWidth(display_elements[element].value))/2;
  }else{  
      return 0;
  
  }
}

// Update digital and label sprites and print them on the screen
void update_element(int element){
  int x1 = set_textalignment(element, display_elements[element].textalign);
  display_elements[element].sprite->clear();
  display_elements[element].sprite->setCursor(x1,0);
  display_elements[element].sprite->setTextColor(ifei_color);
  display_elements[element].sprite->print(display_elements[element].value);
  display_elements[element].sprite->pushSprite(display_elements[element].pos_x,display_elements[element].pos_y);
  
}

// Update clock sprites and print them on the screem
void update_Clock(int element){
  String H;
  String DP1;
  String M;
  String DP2;
  String S;
  int offset = 0;
  if (element == CLOCKU){
    H = TC_H;
    DP1 = TC_Dd1;
    M = TC_M;
    DP2 = TC_Dd2;
    S = TC_S;
  }else{
    if (LC_H == " 0") {
      H = "0";
      offset = 28;
    } else if (LC_H[1] == 32){
      H = LC_H[0];
      offset = 28;
    }else {
      H = LC_H;
    }
    DP1 = LC_Dd1;
    M = LC_M;
    DP2 = LC_Dd2;
    S = LC_S;
  }

  display_elements[element].sprite->clear();
  display_elements[element].sprite->setTextColor(ifei_color);
  if ( H[0] == 32 ){
     display_elements[element].sprite->setCursor(57,1);
  }else {
    display_elements[element].sprite->setCursor(1 + offset,1);
    display_elements[element].sprite->print(H);  
  }
  if ( DP1[0] == 32 ){
    display_elements[element].sprite->setCursor(63,1);
  }else {
    display_elements[element].sprite->print(DP1);
  }
  if ( M[0] == 32 ){
    display_elements[element].sprite->setCursor(119,1);
  }else {
    display_elements[element].sprite->print(M);
  }
  if ( DP2[0] == 32 ){
    display_elements[element].sprite->setCursor(125,1);
  }else {
    display_elements[element].sprite->print(DP2);
  }
  if ( S[0] == 32 ){
    display_elements[element].sprite->setCursor(181,1);
  }else {
    display_elements[element].sprite->print(S);
  }

  display_elements[element].sprite->pushSprite(display_elements[element].pos_x,display_elements[element].pos_y);
}

void show_demo(){
    if (!reset){
      reset = true;
      tft.fillScreen(0x000000U);
      ifei_color = color_day;
      NOZL_IMAGE[11].pushSprite(&tft,display_elements[NOZL].pos_x,display_elements[NOZL].pos_y,0x000000U);
      NOZL_IMAGE[12].pushSprite(&tft,display_elements[NOZL].pos_x,display_elements[NOZL].pos_y,0x000000U);
      NOZR_IMAGE[11].pushSprite(&tft,display_elements[NOZR].pos_x,display_elements[NOZR].pos_y,0x000000U);
      NOZR_IMAGE[12].pushSprite(&tft,display_elements[NOZR].pos_x,display_elements[NOZR].pos_y,0x000000U);
      
      for (int i=0; i < 25; i++){
        if ( i != NOZL && i != NOZR && i != CLOCKU && i != CLOCKL){
          update_element(i);
        }
      }
      update_Clock(CLOCKL);
      update_Clock(CLOCKU);
    } 
 
     if (millis() - nozzle_update > 1000){
      NOZL_IMAGE[demo_counter].pushSprite(&tft,display_elements[NOZL].pos_x,display_elements[NOZL].pos_y,0x000000U);  
      NOZR_IMAGE[demo_counter].pushSprite(&tft,display_elements[NOZR].pos_x,display_elements[NOZR].pos_y,0x000000U);    
      nozzle_update = millis();
      if ( demo_forward ){
        if ( demo_counter == 10){
          demo_forward = false;
          demo_counter--;
        }else {
          demo_counter++;
        }
      }else{
        if (demo_counter == 0){
          demo_forward = true;
          demo_counter++;
        }else{
          demo_counter--;
        }
      }
      if ( TC_S.toInt() < 9 ){
        TC_S = "0" + String(TC_S.toInt() + 1);
      }else{
        TC_S = String(TC_S.toInt() + 1);
      }
      
      if ( TC_S == "60" ){
         if ( TC_M.toInt() < 9 ){
          TC_M = "0" + String(TC_M.toInt() + 1);
         }else{
          TC_M = String(TC_M.toInt() + 1);
         }
        TC_S = "00";
      }
      if ( TC_M == "60" ){
        if (TC_H == "24" ){
          TC_H == "00";
        }else{
         if ( TC_H.toInt() < 9 ){
          TC_H = "0" + String(TC_H.toInt() + 1);
         }else{
          TC_H = String(TC_H.toInt() + 1);
         }
        }
         TC_M = "00";
      }
        
      update_Clock(CLOCKU);
  }
}

// ---- Heartbeat state (updated by ESP-NOW callback, consumed in loop) ----
static volatile bool gotHb = false;
static volatile uint32_t lastHbMs = 0;
static volatile uint16_t lastSeq = 0;

static void onEspNowRecv(const uint8_t* mac, const uint8_t* data, int len) {
  if (len != (int)sizeof(Message)) return;

  Message m;
  memcpy(&m, data, sizeof(m));
  if (m.type != 1) return;

  lastSeq = m.seq;
  lastHbMs = millis();
  gotHb = true;
}

static void initEspNowClient() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);

  if (esp_now_init() != ESP_OK) {
    while (true) delay(1000);
  }

  esp_now_register_recv_cb(onEspNowRecv);
}

void setup() {
  tft.begin();

  if (!LITTLEFS.begin(true)) {
    return;
  }

  create_display_elements();
  tft.setColorDepth(24);
  tft.fillScreen(0x000000U);

  initEspNowClient();
}

void loop() {
  const uint32_t now = millis();

  // If no heartbeat for 2 seconds -> NO DATA
  if (now - (uint32_t)lastHbMs > 2000) {
    static uint32_t lastDraw = 0;
    if (now - lastDraw > 250) {
      lastDraw = now;
      TC_S = String(lastSeq % 60);
      update_Clock(CLOCKU);
    }
    return;
  }
  if (!ishornet) {
    show_demo();
  } else if (reset) {
    tft.fillScreen(0x000000U);
    reset = false;
  }
}