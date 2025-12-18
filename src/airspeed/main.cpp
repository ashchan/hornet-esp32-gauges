// Designed for Waveshare 1.85" round lcd without touch screen
// https://www.waveshare.com/wiki/ESP32-S3-LCD-1.85
//
// LVGL version is 8.3.11
// Display_ST77916, esp_lcd_st77916, I2C_Driver files sourced from WaveShare's demo code for the display

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <lvgl.h>
#include "Display_ST77916.h"
#include "message.h"

// LVGL bitmaps
#include "airSpeedIndicatorBG.c"
#include "airspeedNeedle.c"

#define DISP_WIDTH  360
#define DISP_HEIGHT 360


// LVGL draw buffers
static lv_color_t buf1[DISP_WIDTH * 40];
static lv_color_t buf2[DISP_WIDTH * 40];


// ===== Globals =====
lv_obj_t *imgBackground;
lv_obj_t *imgNeedle;


// Center and radius
const int16_t center_x = DISP_WIDTH / 2;
const int16_t center_y = DISP_HEIGHT / 2;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  LCD_addWindow(area->x1, area->y1, area->x2, area->y2, (uint16_t*)color_p);
  lv_disp_flush_ready(disp);
}

bool hasNewMessage = false;
IntegerMessage lastMessage = {};
void updateRendering() {
  lv_img_set_angle(imgNeedle, map(lastMessage.value, 0, 65530, 0, 3500));
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
    IntegerMessage message{};
    switch (hdr->category) {
    case MessageCategory::Integer:
      if (len != (int)sizeof(IntegerMessage)) {
        return;
      }
      message = *reinterpret_cast<const IntegerMessage *>(data);
      if (message.name == ValueName::Airspeed) {
        lastMessage = message;
        hasNewMessage = true;
      }
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
  imgBackground = lv_img_create(lv_scr_act());
  lv_img_set_src(imgBackground, &airSpeedIndicatorBG);
  lv_obj_align(imgBackground, LV_ALIGN_CENTER, 0, 0);

  // ===== Needle =====
  imgNeedle = lv_img_create(lv_scr_act());
  lv_img_set_src(imgNeedle, &airspeedNeedle);

  // Set the pivot to the bottom center of the image
  lv_point_t pivot = {
    (lv_coord_t)(airspeedNeedle.header.w / 2),   // horizontally centered
    (lv_coord_t)(airspeedNeedle.header.h / 2)    // very bottom
  };
  lv_img_set_pivot(imgNeedle, pivot.x, pivot.y);

  // Align so the pivot (bottom center) is at the gauge center
  lv_obj_align(imgNeedle, LV_ALIGN_CENTER, 0, 0);

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