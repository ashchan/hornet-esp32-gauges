// ===== Altimeter Gauge for Waveshare 1.85" LCD (ESP32-S3) =====
// LVGL v8.3.11
// Display_ST77916, esp_lcd_st77916, I2C_Driver from Waveshare demo
// DCS-BIOS integration for F/A-18C Altimeter
#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <lvgl.h>
#include "Display_ST77916.h"
#include "message.h"

// ===== Bitmaps =====
#include "altimeterBackground.c"
#include "altimeterMarquee.c"    // 1000s drum
#include "altimeterMarquee2.c"   // 10000s drum
#include "altimeterNeedle.c"
#include "barometerDrum.c"

#define DISP_WIDTH  360
#define DISP_HEIGHT 360

#define DIGIT_HEIGHT   41   // 1000s digit height
#define DIGIT_HEIGHT2  37   // 10000s digit height

// ===== Barometer 4-Digit Drum Window =====
const int BARO_DIGIT_W = 18;    // each digit width (px)
const int BARO_DIGIT_H = 28;    // each digit height (px)
const int BARO_TOTAL_H = 280;   // full drum image height (10 digits stacked)

// ===== LVGL buffers =====
static lv_color_t buf1[DISP_WIDTH * 40];
static lv_color_t buf2[DISP_WIDTH * 40];

// ===== GUI Objects =====
lv_obj_t *img_altimeterBackground;
lv_obj_t *img_altimeterMarquee;
lv_obj_t *img_altimeterMarquee2;
lv_obj_t *img_altimeterNeedle;
lv_obj_t *baro_window;
lv_obj_t *img_baroThousands;
lv_obj_t *img_baroHundreds;
lv_obj_t *img_baroTens;
lv_obj_t *img_baroOnes;

// Gauge center
const int16_t center_x = DISP_WIDTH / 2;
const int16_t center_y = DISP_HEIGHT / 2;

// ===== Flush function for LVGL =====
void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  LCD_addWindow(area->x1, area->y1, area->x2, area->y2, (uint16_t*)color_p);
  lv_disp_flush_ready(disp);
}

// Needle (0–65535 => 0–360°)
void onStbyAlt100FtPtrChange(unsigned int newValue) {
  float angle = (newValue / 65535.0f) * 360.0f;
  lv_img_set_angle(img_altimeterNeedle, angle * 10); // LVGL uses 0.1° units
}

void onStbyAlt1000FtCntChange(unsigned int newValue) {
  float value = 9;
  if (newValue < 6553) value = 0;
  else if (newValue < 13106) value = 1;
  else if (newValue < 19660) value = 2;
  else if (newValue < 26214) value = 3;
  else if (newValue < 32767) value = 4;
  else if (newValue < 39321) value = 5;
  else if (newValue < 45874) value = 6;
  else if (newValue < 52428) value = 7;
  else if (newValue < 58981) value = 8;

  float offset = (value - 9) / 10 * 410;
  lv_img_set_offset_y(img_altimeterMarquee, (int)offset);
}

// 10000s drum (0–5)
void onStbyAlt10000FtCntChange(unsigned int newValue) {
  int offset10000s = map(newValue, 0, 36407, DIGIT_HEIGHT2 , (DIGIT_HEIGHT2 * 6));
  lv_img_set_offset_y(img_altimeterMarquee2, offset10000s);
}

// value: 0 - 9
void updateBaroDrum(lv_obj_t *img, float value) {
  float offset = (value - 9) / 10 * BARO_TOTAL_H;
  lv_img_set_offset_y(img, (int)offset);
}

void onStbyPressSet0Change(unsigned int newValue) {
  float number = 9;
  if (newValue < 6553) number = 0;
  else if (newValue < 13106) number = 1;
  else if (newValue < 16301) number = 2;
  else if (newValue < 19660) number = 3;
  else if (newValue < 29918) number = 4;
  else if (newValue < 36727) number = 5;
  else if (newValue < 43536) number = 6;
  else if (newValue < 50345) number = 7;
  else if (newValue < 53284) number = 8;
  updateBaroDrum(img_baroOnes, number);
}

void onStbyPressSet1Change(unsigned int newValue) {
  float number = 9;
  if (newValue < 6553) number = 0;
  else if (newValue < 13106) number = 1;
  else if (newValue < 16301) number = 2;
  else if (newValue < 19660) number = 3;
  else if (newValue < 29918) number = 4;
  else if (newValue < 36727) number = 5;
  else if (newValue < 43536) number = 6;
  else if (newValue < 50345) number = 7;
  else if (newValue < 53284) number = 8;
  updateBaroDrum(img_baroTens, number);
}

void onStbyPressSet2Change(unsigned int newValue) {
  float thousands = 2;
  float hundreds = 9;
  if (newValue < 39321) {
    thousands = 2;
    hundreds = 8;
  } else if (newValue < 52428) {
    thousands = 2;
    hundreds = 9;
  } else if (newValue < 65535) {
    thousands = 3;
    hundreds = 0;
  } else {
    thousands = 3;
    hundreds = 1;
  }
  updateBaroDrum(img_baroThousands, thousands);
  updateBaroDrum(img_baroHundreds, hundreds);
}

static AltimeterMessage lastMessage = {};
volatile bool hasNewMessage = false;

void updateRendering() {
  onStbyAlt10000FtCntChange(lastMessage.alt10000FtCnt);
  onStbyAlt1000FtCntChange(lastMessage.alt1000FtCnt);
  onStbyAlt100FtPtrChange(lastMessage.alt100FtPtr);
  onStbyPressSet0Change(lastMessage.pressSet0);
  onStbyPressSet1Change(lastMessage.pressSet1);
  onStbyPressSet2Change(lastMessage.pressSet2);
}

static void initEspNowClient() {
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb([](const esp_now_recv_info_t* info, const uint8_t* data, int len) {
    if (len < (int)sizeof(MessageHeader)) {
      return;
    }

    const MessageHeader* hdr = reinterpret_cast<const MessageHeader*>(data);
    switch (hdr->category) {
    case MessageCategory::Altimeter:
      if (len != (int)sizeof(AltimeterMessage)) {
        return;
      }
      lastMessage = *reinterpret_cast<const AltimeterMessage *>(data);
      hasNewMessage = true;
      break;
    default:
      break;
    }
  });
}

void setup() {
  Serial.begin(115200);

  Wire1.begin(11, 10);   // SDA=11, SCL=10 (EXT I2C on this Waveshare 1.85 family)
  Wire1.setClock(400000);

  ST77916_Init();
  Backlight_Init();
  Set_Backlight(25);

  lv_init();

  static lv_disp_draw_buf_t draw_buf;
  lv_disp_draw_buf_init(&draw_buf, buf1, buf2, DISP_WIDTH * 40);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = DISP_WIDTH;
  disp_drv.ver_res = DISP_HEIGHT;
  disp_drv.flush_cb = my_disp_flush;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  // ===== Black background =====
  lv_obj_t *bg_rect = lv_obj_create(lv_scr_act());
  lv_obj_set_size(bg_rect, DISP_WIDTH, DISP_HEIGHT);
  lv_obj_set_style_bg_color(bg_rect, lv_color_hex(0x000000), 0);
  lv_obj_set_style_border_width(bg_rect, 0, 0);
  lv_obj_clear_flag(bg_rect, LV_OBJ_FLAG_SCROLLABLE);

  // ===== Altimeter background =====
  img_altimeterBackground = lv_img_create(lv_scr_act());
  lv_img_set_src(img_altimeterBackground, &altimeterBackground);
  lv_obj_align(img_altimeterBackground, LV_ALIGN_CENTER, 0, 0);

  // ===== 1000s Drum Window =====
  lv_obj_t *marquee_window = lv_obj_create(lv_scr_act());
  lv_obj_set_style_pad_all(marquee_window, 0, 0);
  lv_obj_set_size(marquee_window, 38, 41);
  lv_obj_set_style_bg_opa(marquee_window, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(marquee_window, 0, 0);
  lv_obj_clear_flag(marquee_window, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(marquee_window, LV_SCROLLBAR_MODE_OFF);
  lv_obj_align(marquee_window, LV_ALIGN_TOP_LEFT, 118, 111);

  img_altimeterMarquee = lv_img_create(marquee_window);
  lv_img_set_src(img_altimeterMarquee, &altimeterMarquee);
  lv_obj_set_pos(img_altimeterMarquee, 0, 0);

  // ===== 10000s Drum Window =====
  lv_obj_t *marquee_window2 = lv_obj_create(lv_scr_act());
  lv_obj_set_style_pad_all(marquee_window2, 0, 0);
  lv_obj_set_size(marquee_window2, 34, 37);
  lv_obj_set_style_bg_opa(marquee_window2, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(marquee_window2, 0, 0);
  lv_obj_clear_flag(marquee_window2, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_set_scrollbar_mode(marquee_window2, LV_SCROLLBAR_MODE_OFF);
  lv_obj_align(marquee_window2, LV_ALIGN_TOP_LEFT, 79, 113);

  img_altimeterMarquee2 = lv_img_create(marquee_window2);
  lv_img_set_src(img_altimeterMarquee2, &altimeterMarquee2);
  lv_obj_set_pos(img_altimeterMarquee2, 0, 0);

  // ===== Needle =====
  img_altimeterNeedle = lv_img_create(lv_scr_act());
  lv_img_set_src(img_altimeterNeedle, &altimeterNeedle);

  // Pivot at center of needle image
  lv_point_t pivot = { (lv_coord_t)(altimeterNeedle.header.w / 2), (lv_coord_t)(altimeterNeedle.header.h / 2) };
  lv_img_set_pivot(img_altimeterNeedle, pivot.x, pivot.y);

  // Place pivot exactly at gauge center
  lv_obj_set_pos(img_altimeterNeedle, center_x - pivot.x, center_y - pivot.y);

  // overall window width = 4 digits + small gaps
  int windowW = (BARO_DIGIT_W * 4) + 6; // ~78 px wide
  int windowH = BARO_DIGIT_H;           // 28 px tall

  baro_window = lv_obj_create(lv_scr_act());
  lv_obj_set_size(baro_window, windowW, windowH);
  lv_obj_set_style_pad_all(baro_window, 0, 0);

  // alignment border (temporary)
  // lv_obj_set_style_border_width(baro_window, 1, 0);
  // lv_obj_set_style_border_color(baro_window, lv_palette_main(LV_PALETTE_RED), 0);
  // lv_obj_set_style_border_side(baro_window, LV_BORDER_SIDE_FULL, 0);

  // transparent background and non-scrollable
  lv_obj_set_style_bg_opa(baro_window, LV_OPA_TRANSP, 0);
  lv_obj_set_style_border_width(baro_window, 0, 0);
  lv_obj_clear_flag(baro_window, LV_OBJ_FLAG_SCROLLABLE);

  // 🧭 position on your gauge (tweak these to line up)
  lv_obj_align(baro_window, LV_ALIGN_TOP_LEFT, 141, 257);

  // Leftmost thousands
  img_baroThousands = lv_img_create(baro_window);
  lv_img_set_src(img_baroThousands, &barometerDrum);
  lv_obj_set_pos(img_baroThousands, 0, 0);

  // Hundreds
  img_baroHundreds = lv_img_create(baro_window);
  lv_img_set_src(img_baroHundreds, &barometerDrum);
  lv_obj_set_pos(img_baroHundreds, BARO_DIGIT_W + 2, 0);

  // Tens
  img_baroTens = lv_img_create(baro_window);
  lv_img_set_src(img_baroTens, &barometerDrum);
  lv_obj_set_pos(img_baroTens, (BARO_DIGIT_W + 2) * 2, 0);

  // Ones (rightmost)
  img_baroOnes = lv_img_create(baro_window);
  lv_img_set_src(img_baroOnes, &barometerDrum);
  lv_obj_set_pos(img_baroOnes, (BARO_DIGIT_W + 2) * 3, 0);

  lv_img_set_offset_y(img_altimeterMarquee, DIGIT_HEIGHT);
  lv_img_set_offset_y(img_altimeterMarquee2, DIGIT_HEIGHT2);

  lv_img_set_offset_y(img_baroOnes, BARO_DIGIT_H);
  lv_img_set_offset_y(img_baroTens, BARO_DIGIT_H);

  initEspNowClient();
}

void loop() {
  static uint32_t lastTick = millis();
  const uint32_t now = millis();
  uint32_t dt = now - lastTick;
  lastTick = now;

  lv_tick_inc(dt);

  static uint32_t lastUpdatedAt = 0;
  if (now - lastUpdatedAt > 40 && hasNewMessage) {
    hasNewMessage = false;
    lastUpdatedAt = now;
    updateRendering();
  }

  lv_timer_handler();     // Refresh LVGL
}