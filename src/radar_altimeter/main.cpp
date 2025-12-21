#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <lvgl.h>
#include "Display_ST77916.h"
#include "message.h"

// LVGL bitmaps
#include "radarAltBackground.c"
#include "radarAltNeedle.c"
#include "radarAltMinHeight.c"
#include "radarAltOff.c"
#include "RedLedOff.c"
#include "GreenLedOff.c"
#include "RedLedOn.c"
#include "GreenLedOn.c"


#define DISP_WIDTH  360
#define DISP_HEIGHT 360

// LVGL draw buffers
static lv_color_t buf1[DISP_WIDTH * 40];
static lv_color_t buf2[DISP_WIDTH * 40];

// ===== Globals =====
lv_obj_t *img_radarAltBackground;
lv_obj_t *img_radarAltNeedle;
lv_obj_t *img_radarAltMinHeight;
lv_obj_t *img_radarAltOff;
lv_obj_t *img_RedLed;
lv_obj_t *img_GreenLed;

// Center and radius
const int16_t center_x = DISP_WIDTH / 2;
const int16_t center_y = DISP_HEIGHT / 2;

void my_disp_flush(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
  LCD_addWindow(area->x1, area->y1, area->x2, area->y2, (uint16_t*)color_p);
  lv_disp_flush_ready(disp);
}

static RadarAltimeterMessage lastMessage = {};
volatile bool hasNewMessage = false;

void updateRendering() {
  lv_img_set_angle(img_radarAltNeedle, map(lastMessage.altPtr, 3450, 65530, 0, 3200));
  lv_img_set_angle(img_radarAltMinHeight, map(lastMessage.minHeightPtr, 1800, 65530, 0, 3200));

  if (lastMessage.greenLamp == 1) {
    lv_img_set_src(img_GreenLed, &GreenLedOn);
  } else {
    lv_img_set_src(img_GreenLed, &GreenLedOff);
  }

  if (lastMessage.warnLt == 1) {
    lv_img_set_src(img_RedLed, &RedLedOn);
  } else {
    lv_img_set_src(img_RedLed, &RedLedOff);
  }

  const int16_t H = (int16_t)radarAltOff.header.h;
  const int16_t OFF_EXTRA = 5;

  // Translate Y goes from -(H+5) (off) to 0 (fully visible)
  int32_t ty = - (int32_t)H - OFF_EXTRA + ((int32_t)(H + OFF_EXTRA) * (int32_t)lastMessage.offFlag) / 65535;

  // Clamp (just in case)
  if (ty > 0) {
    ty = 0;
  }
  if (ty < -((int32_t)H + OFF_EXTRA)) {
    ty = -((int32_t)H + OFF_EXTRA);
  }

  lv_obj_set_style_translate_y(img_radarAltOff, (int16_t)ty, 0);
}

static void initEspNowClient() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(ESP_CHANNEL, WIFI_SECOND_CHAN_NONE);

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
    case MessageCategory::RadarAltimeter:
      if (len != (int)sizeof(RadarAltimeterMessage)) {
        return;
      }
      lastMessage = *reinterpret_cast<const RadarAltimeterMessage *>(data);
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


  // OFF flag (final resting position is centered with +69px)
  img_radarAltOff = lv_img_create(lv_scr_act());
  lv_img_set_src(img_radarAltOff, &radarAltOff);
  lv_obj_align(img_radarAltOff, LV_ALIGN_CENTER, 0, 69);

  // Start fully hidden behind the gauge face
  const int16_t OFF_EXTRA = 5;
  lv_obj_set_style_translate_y(img_radarAltOff,-(int16_t)radarAltOff.header.h - OFF_EXTRA, 0);


  // ===== Radar background =====
  img_radarAltBackground = lv_img_create(lv_scr_act());
  lv_img_set_src(img_radarAltBackground, &radarAltBackground);
  lv_obj_align(img_radarAltBackground, LV_ALIGN_CENTER, 0, 0);

  // ===== LEDs =====
  img_RedLed = lv_img_create(lv_scr_act());
  lv_img_set_src(img_RedLed, &RedLedOff);
  lv_obj_align(img_RedLed, LV_ALIGN_CENTER, -72, 0);

  img_GreenLed = lv_img_create(lv_scr_act());
  lv_img_set_src(img_GreenLed, &GreenLedOff);
  lv_obj_align(img_GreenLed, LV_ALIGN_CENTER, 72, 0);

  // ===== Altitude Min Height Indicator =====
  img_radarAltMinHeight = lv_img_create(lv_scr_act());
  lv_img_set_src(img_radarAltMinHeight, &radarAltMinHeight);

  // Position pointer at top center (example)
  int16_t x = center_x - radarAltMinHeight.header.w / 2;
  int16_t y = 0; // distance from top edge
  lv_obj_set_pos(img_radarAltMinHeight, x, y);

  // Set pivot to bottom center
  lv_point_t pivot = { (lv_coord_t)(radarAltMinHeight.header.w / 2), (lv_coord_t)(radarAltMinHeight.header.h) };
  lv_img_set_pivot(img_radarAltMinHeight, pivot.x, pivot.y);

  // ===== Radar Altimeter Needle =====
  img_radarAltNeedle = lv_img_create(lv_scr_act());
  lv_img_set_src(img_radarAltNeedle, &radarAltNeedle);
  lv_obj_align(img_radarAltNeedle, LV_ALIGN_CENTER, 0, -48);

  // Set pivot explicitly to hub center
  lv_img_set_pivot(img_radarAltNeedle, 36, 123);

  // Start needle at 0°
  lv_img_set_angle(img_radarAltNeedle, 0);

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
