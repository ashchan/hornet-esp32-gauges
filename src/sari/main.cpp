// F18_SARI_v2
// ESP32-S3 + ST7701 480x480 + LVGL8 + DCS-BIOS
// Author: Shef, https://github.com/shef-code/F18_SARI_v2
#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <lvgl.h>
#include <math.h>

#include "I2C_Driver.h"
#include "TCA9554PWR.h"
#include "Display_ST7701.h"
#include "message.h"

extern "C" {
  extern const lv_img_dsc_t SARIBackground;
  extern const lv_img_dsc_t SARIBug;
  extern const lv_img_dsc_t RateOfTurn;
  extern const lv_img_dsc_t SlipBall;
  extern const lv_img_dsc_t Bank;
  extern const lv_img_dsc_t SARIGlobe;
  extern const lv_img_dsc_t VerticalPointer;
  extern const lv_img_dsc_t HorizontalPointer;
  extern const lv_img_dsc_t SARICaged;
}

// ----------------- Config -----------------
#define DISP_WIDTH   480
#define DISP_HEIGHT  480

static constexpr int  BUG_Y_OFFSET   = -18;
static constexpr int  GLOBE_Y_OFFSET = -33;
static constexpr int  BANK_Y_OFFSET  = -33;  // negative = up, positive = down

static constexpr int  HP_X_OFFSET = 0;    // horizontal pointer base (active)
static constexpr int  HP_Y_OFFSET = -15;

static constexpr int  VP_X_OFFSET = 0;    // vertical pointer base (active)
static constexpr int  VP_Y_OFFSET = -15;

static constexpr int  SARICAGED_X_OFFSET = 157;
static constexpr int  SARICAGED_Y_OFFSET = -96;

static constexpr int  PITCH_MAX_DEG  = 90;
static constexpr int  PITCH_DIR      = +1;     // +1 = globe moves down on pitch up

// 300x900 art => 900/180 = 5 px/°. We want 3 px/°  ==> scale = 0.6
static constexpr float PITCH_SCALE   = 0.6f;   // 3 px/°
static int             PITCH_PX_PER_DEG = 3;   // set in setup from art * scale
static int             PITCH_MAX_PX      = PITCH_MAX_DEG * 3;

static constexpr int   SLIP_MAX_X_PX  = 90;
static constexpr int   SLIP_BASE_Y    = 152;
static constexpr float SLIP_MAX_Y_UP  = 7.0f;
static constexpr int   ROT_MAX_X_PX   = 75;
static constexpr int   ROT_BASE_Y     = 200;

// Pointer travel tuning
// 1) HorizontalPointer: translate UP by up to this many pixels (from HP_Y_OFFSET).
static constexpr int   HP_MAX_UP_PX        = 165;  // tweak to taste

// 2) VerticalPointer: at max input, LEFT edge sits -5 px off the display.
static constexpr int   VP_OFF_LEFT_EDGE_PX = -10;   // left edge target at max

static constexpr long  DCS_MID_CODE   = 32782;
static constexpr long  DCS_HALF_SPAN  = 32768;

// Pivot fine-tune relative to image center
static constexpr int   GLOBE_CAL_X    = 0;
static constexpr int   GLOBE_CAL_Y    = 0;

// Extra rotation applied to the GLOBE ONLY (tenths of degrees); 1800 = 180°
static constexpr int   GLOBE_ANGLE_OFFSET_TENTHS = 1800;

// ----------------- LVGL state -----------------
static lv_disp_draw_buf_t draw_buf;
#define BUF_LINES 64
static lv_color_t buf1[DISP_WIDTH * BUF_LINES];

static lv_obj_t *scr                = nullptr;
static lv_obj_t *bug_img            = nullptr;
static lv_obj_t *slipBall_img       = nullptr;
static lv_obj_t *rateOfTurn_img     = nullptr;
static lv_obj_t *bank_img           = nullptr;
static lv_obj_t *globe_holder       = nullptr;  // fixed viewport
static lv_obj_t *globe_img          = nullptr;  // rotated & translated child
static lv_obj_t *verticalPointer_img   = nullptr;
static lv_obj_t *horizontalPointer_img = nullptr;
static lv_obj_t *sariCaged_img         = nullptr;

// Geometry/cache
static int globe_w = 0, globe_h = 0;
static int globe_center_x = 0, globe_base_y = 0;

// ----------------- DCS -> UI Mailboxes (ISR writes; loop reads) -----------------
static volatile int16_t  mb_pitch_x10   = 0;             // tenths of degrees
static volatile int16_t  mb_bank_x10    = 0;             // tenths of degrees (0..3600, oriented)
static volatile uint16_t mb_rot_raw     = DCS_MID_CODE;
static volatile uint16_t mb_slip_raw    = DCS_MID_CODE;
static volatile uint16_t mb_bug_raw     = DCS_MID_CODE;  // manual pitch bug
static volatile uint16_t mb_cage_raw    = 0;             // 0..65535 for SARICaged
static volatile uint16_t mb_pointerHorRaw  = 0;          // raw DCS (32767..65535 active)
static volatile uint16_t mb_pointerVerRaw  = 0;          // raw DCS (0..32767 active)

// UI copies (non-volatile)
static int16_t  ui_pitch_x10 = 0;
static int16_t  ui_bank_x10  = 0;
static uint16_t ui_rot_raw   = DCS_MID_CODE;
static uint16_t ui_slip_raw  = DCS_MID_CODE;
static uint16_t ui_bug_raw   = DCS_MID_CODE;
static uint16_t ui_cage_raw  = 0;
static uint16_t ui_pointerHorRaw = 0;
static uint16_t ui_pointerVerRaw = 0;

// ----------------- Utils -----------------
static inline int clampInt(int v, int lo, int hi) { return (v < lo) ? lo : (v > hi) ? hi : v; }

// Map v from [in_min..in_max] to [out_min..out_max], clamped; 64-bit math to avoid overflow
static inline int map_u16_range(uint16_t v, uint16_t in_min, uint16_t in_max, int out_min, int out_max) {
  if (in_max <= in_min) return out_min;
  if (v <= in_min) return out_min;
  if (v >= in_max) return out_max;
  int64_t num = (int64_t)(v - in_min) * (out_max - out_min);
  int64_t den = (int64_t)(in_max - in_min);
  return (int)(out_min + (num / den));
}

static void my_disp_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p) {
  LCD_addWindow((uint16_t)area->x1, (uint16_t)area->y1,
                (uint16_t)area->x2, (uint16_t)area->y2, (uint8_t*)color_p);
  lv_disp_flush_ready(drv);
}

// ----------------- Core: roll-compensated pitch -----------------
static void update_globe_position(int16_t pitch_x10, int16_t bank_x10) {
  const int pitch_pixels = clampInt(PITCH_DIR * (pitch_x10 * PITCH_PX_PER_DEG) / 10,
                                    -PITCH_MAX_PX, PITCH_MAX_PX);

  const int pivot_x = (globe_w / 2) + GLOBE_CAL_X;
  const int pivot_y = (globe_h / 2) + GLOBE_CAL_Y;
  const int base_img_x = (globe_w / 2) - pivot_x;   // pivot at holder center
  const int base_img_y = (globe_h / 2) - pivot_y;

  // Translate along bitmap’s local vertical axis to keep motion “up” relative to the rolled globe
  const float bank_rad = bank_x10 * 0.0017453292519943296f; // pi/1800
  const int dx = (int)lrintf(-sinf(bank_rad) * pitch_pixels);
  const int dy = (int)lrintf( cosf(bank_rad) * pitch_pixels);

  lv_obj_set_pos(globe_img, base_img_x + dx, base_img_y + dy);
}

// ----------------- DCS-BIOS callbacks (ISR-safe: NO LVGL HERE) -----------------
void onSaiSlipBallChange(unsigned int v) {
  mb_slip_raw = (uint16_t)v;
}

void onSaiBankChange(unsigned int v) {
  // 0..65535 → 0..3600 (tenths°), then rotate 180° for panel orientation
  int32_t t = ((int32_t)v * 3600) / 65535;
  t = (t + 1800) % 3600;
  mb_bank_x10 = (int16_t)t;
}

void onSaiRateOfTurnChange(unsigned int v) {
  mb_rot_raw = (uint16_t)v;
}

void onSaiManPitchAdjChange(unsigned int v) {
  mb_bug_raw = (uint16_t)v;
}

void onSaiPitchChange(unsigned int v) {
  // 0..65535 → -900..+900 tenths deg
  mb_pitch_x10 = (int16_t)map((int)v, 0, 65535, -900, 900);
}

// SARICaged (store raw, mapping/anim in UI thread)
void onSaiAttWarningFlagChange(unsigned int v) {
  mb_cage_raw = (uint16_t)v;
}

// Horizontal pointer RAW (active range 32767..65535)
void onSaiPointerHorChange(unsigned int v) {
  mb_pointerHorRaw = (uint16_t)v;
}

// Vertical pointer RAW (active range 0..32767)
void onSaiPointerVerChange(unsigned int v) {
  mb_pointerVerRaw = (uint16_t)v;
}

static SaiMessage lastMessage = {};
uint16_t brightness = 0;
volatile bool hasNewMessage = false;

#define DEFAULT_BRIGHTNESS 10
void setBrightness(uint16_t value = DEFAULT_BRIGHTNESS) {
  static uint16_t oldValue = DEFAULT_BRIGHTNESS;
  uint16_t newValue = map(value, 0, 65535, DEFAULT_BRIGHTNESS, 80);
  if (oldValue != newValue) {
    oldValue = newValue;
    Set_Backlight(newValue);
  }
}

static void updateRendering() {
  onSaiPitchChange(lastMessage.pitch);
  onSaiBankChange(lastMessage.bank);
  onSaiSlipBallChange(lastMessage.slipBall);
  onSaiRateOfTurnChange(lastMessage.rateOfTurn);
  onSaiManPitchAdjChange(lastMessage.manPitchAdj);
  onSaiAttWarningFlagChange(lastMessage.attWarningFlag);
  onSaiPointerHorChange(lastMessage.pointerHor);
  onSaiPointerVerChange(lastMessage.pointerVer);

  noInterrupts();
  int16_t  pitch_x10  = mb_pitch_x10;
  int16_t  bank_x10   = mb_bank_x10;
  uint16_t rot_raw    = mb_rot_raw;
  uint16_t slip_raw   = mb_slip_raw;
  uint16_t bug_raw    = mb_bug_raw;
  uint16_t cage_raw   = mb_cage_raw;
  uint16_t pHorRaw    = mb_pointerHorRaw;  // 32767..65535 active
  uint16_t pVerRaw    = mb_pointerVerRaw;  // 0..32767 active
  interrupts();

  // Store copies
  ui_pitch_x10  = pitch_x10;
  ui_bank_x10   = bank_x10;
  ui_rot_raw    = rot_raw;
  ui_slip_raw   = slip_raw;
  ui_bug_raw    = bug_raw;
  ui_cage_raw   = cage_raw;
  ui_pointerHorRaw = pHorRaw;
  ui_pointerVerRaw = pVerRaw;

  // ---- Apply to LVGL (safe context) ----

  // Bank rotation: bank tape follows DCS; globe is flipped 180° from it
  lv_img_set_angle(bank_img, ui_bank_x10);
  lv_img_set_angle(globe_img, ui_bank_x10);

  // Globe pitch translation (roll-compensated)
  update_globe_position(ui_pitch_x10, ui_bank_x10);

  // Rate of Turn lateral motion
  {
    long delta = (long)ui_rot_raw - DCS_MID_CODE;
    long x = (DCS_HALF_SPAN != 0) ? (delta * ROT_MAX_X_PX) / DCS_HALF_SPAN : 0;
    x = clampInt((int)x, -ROT_MAX_X_PX, ROT_MAX_X_PX);
    lv_obj_align(rateOfTurn_img, LV_ALIGN_CENTER, (int)x, ROT_BASE_Y);
  }

  // Slip Ball (x + slight y rise when off-center)
  {
    long delta = (long)ui_slip_raw - DCS_MID_CODE;
    long x = (DCS_HALF_SPAN != 0) ? (delta * SLIP_MAX_X_PX) / DCS_HALF_SPAN : 0;
    x = clampInt((int)x, -SLIP_MAX_X_PX, SLIP_MAX_X_PX);
    float ratio = (float)abs((int)x) / (float)SLIP_MAX_X_PX;
    int y = SLIP_BASE_Y - (int)(ratio * SLIP_MAX_Y_UP);
    lv_obj_align(slipBall_img, LV_ALIGN_CENTER, (int)x, y);
  }

  // Bug (manual pitch adjust)
  {
    static constexpr int MAX_PX = 35;
    long delta = (long)ui_bug_raw - DCS_MID_CODE;
    long y = (DCS_HALF_SPAN != 0) ? (delta * MAX_PX) / DCS_HALF_SPAN : 0;
    y = -y;
    int y_out = clampInt((int)y, -MAX_PX, MAX_PX) + BUG_Y_OFFSET;
    lv_obj_align(bug_img, LV_ALIGN_CENTER, 0, y_out);
  }

  // ----------------- POINTER LOGIC with split ranges -----------------

  // 1) HorizontalPointer: active only when raw in [32767..65535]
  //    Map to UP travel [0..HP_MAX_UP_PX] (screen Y decreases upward)
  {
    // Clamp to input domain to be safe
    const uint16_t IN_MIN = 32767u;
    const uint16_t IN_MAX = 65535u;
    int up_px = map_u16_range(ui_pointerHorRaw, IN_MIN, IN_MAX, 0, HP_MAX_UP_PX);
    int y = HP_Y_OFFSET - up_px;  // up is negative Y
    lv_obj_align(horizontalPointer_img, LV_ALIGN_CENTER, HP_X_OFFSET, y);
  }

  // 2) VerticalPointer: active only when raw in [0..32767]
  //    Interpolate center X from base_center_x (at 0) to target_center_x (at 32767)
  {
    const uint16_t IN_MIN = 0u;
    const uint16_t IN_MAX = 32767u;

    const int base_center_x = (DISP_WIDTH / 2) + VP_X_OFFSET;
    const int vp_w = VerticalPointer.header.w;
    const int target_center_x = VP_OFF_LEFT_EDGE_PX + (vp_w / 2);

    // Compute interpolated center X
    int center_x = base_center_x + map_u16_range(ui_pointerVerRaw, IN_MIN, IN_MAX,
                                                 target_center_x - base_center_x, 0);

    // Convert back to LVGL align offset (relative to screen center)
    int dx = center_x - (DISP_WIDTH / 2);
    lv_obj_align(verticalPointer_img, LV_ALIGN_CENTER, dx, VP_Y_OFFSET);
  }
  // ------------------------------------------------------

  // SARICaged animation (DCS 0 -> -900, 65535 -> 0), but init hard-set
  {
    static int16_t last_caged_angle = 0;  // match setup()
    static bool cage_inited = false;

    // Correct mapping: 0..65535  ->  -900..0 (tenths of degrees)
    int16_t target = (int16_t)(-900 + ((int32_t)ui_cage_raw * 900) / 65535);

    if (!cage_inited) {
      lv_img_set_angle(sariCaged_img, target);
      last_caged_angle = target;
      cage_inited = true;
    } else if (abs(target - last_caged_angle) >= 5) {
      lv_anim_del(sariCaged_img, (lv_anim_exec_xcb_t)lv_img_set_angle);
      lv_anim_t a; lv_anim_init(&a);
      lv_anim_set_var(&a, sariCaged_img);
      lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_img_set_angle);
      lv_anim_set_values(&a, last_caged_angle, target);
      lv_anim_set_time(&a, 250);
      lv_anim_set_path_cb(&a, lv_anim_path_ease_in_out);
      lv_anim_start(&a);
      last_caged_angle = target;
    }
  }

  setBrightness(brightness);
}

static void initEspNowClient() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(ESP_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_max_tx_power(ESP_MAX_TX_POWER);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb([](const esp_now_recv_info_t* info, const uint8_t* data, int len) {
    if (len < (int)sizeof(MessageHeader)) {
      return;
    }

    const MessageHeader* hdr = reinterpret_cast<const MessageHeader*>(data);
    if (hdr->category ==  MessageCategory::SAI) {
      lastMessage = *reinterpret_cast<const SaiMessage *>(data);
      hasNewMessage = true;
    }
    if (hdr->category == MessageCategory::Integer) {
      IntegerMessage message = *reinterpret_cast<const IntegerMessage *>(data);
      if (message.name == ValueName::InstrumentLighting) {
        brightness = message.value;
        hasNewMessage = true;
      }
    }
  });
}


// ----------------- Setup / Loop -----------------
void setup() {
  Serial.begin(115200);

  I2C_Init();
  TCA9554PWR_Init(0x00);
  Set_EXIO(EXIO_PIN8, Low);   // silence buzzer

  LCD_Init();
  Backlight_Init();
  setBrightness();

  lv_init();
  lv_disp_draw_buf_init(&draw_buf, buf1, nullptr, DISP_WIDTH * BUF_LINES);

  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res      = DISP_WIDTH;
  disp_drv.ver_res      = DISP_HEIGHT;
  disp_drv.draw_buf     = &draw_buf;
  disp_drv.flush_cb     = my_disp_flush;
  disp_drv.full_refresh = 0;
  lv_disp_drv_register(&disp_drv);

  scr = lv_obj_create(nullptr);
  lv_disp_load_scr(scr);
  lv_obj_set_style_bg_color(scr, lv_color_black(), LV_PART_MAIN);
  lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, LV_PART_MAIN);

  globe_w = SARIGlobe.header.w;   // 300
  globe_h = SARIGlobe.header.h;   // 900

  PITCH_PX_PER_DEG = (int)lrintf((globe_h / 180.0f) * PITCH_SCALE); // 900/180 * 0.6 = 3
  PITCH_MAX_PX     = PITCH_MAX_DEG * PITCH_PX_PER_DEG;              // 270

  globe_holder = lv_obj_create(scr);
  lv_obj_remove_style_all(globe_holder);
  lv_obj_set_size(globe_holder, globe_w, globe_h);
  globe_center_x = (DISP_WIDTH  - globe_w) / 2;
  globe_base_y   = (DISP_HEIGHT - globe_h) / 2 + GLOBE_Y_OFFSET;
  lv_obj_set_pos(globe_holder, globe_center_x, globe_base_y);

  globe_img = lv_img_create(globe_holder);
  lv_img_set_src(globe_img, &SARIGlobe);
  {
    const int pivot_x = (globe_w / 2) + GLOBE_CAL_X;
    const int pivot_y = (globe_h / 2) + GLOBE_CAL_Y;
    lv_img_set_pivot(globe_img, pivot_x, pivot_y);
    lv_obj_set_pos(globe_img, (globe_w / 2) - pivot_x, (globe_h / 2) - pivot_y);
  }

  lv_obj_t *sari_bg = lv_img_create(scr);
  lv_img_set_src(sari_bg, &SARIBackground);
  lv_obj_center(sari_bg);

  bug_img = lv_img_create(scr);
  lv_img_set_src(bug_img, &SARIBug);
  lv_obj_align(bug_img, LV_ALIGN_CENTER, 0, BUG_Y_OFFSET);

  slipBall_img = lv_img_create(scr);
  lv_img_set_src(slipBall_img, &SlipBall);
  lv_obj_align(slipBall_img, LV_ALIGN_CENTER, 0, SLIP_BASE_Y);

  rateOfTurn_img = lv_img_create(scr);
  lv_img_set_src(rateOfTurn_img, &RateOfTurn);
  lv_obj_align(rateOfTurn_img, LV_ALIGN_CENTER, 0, ROT_BASE_Y);

  bank_img = lv_img_create(scr);
  lv_img_set_src(bank_img, &Bank);
  lv_obj_align(bank_img, LV_ALIGN_CENTER, 0, BANK_Y_OFFSET);
  lv_img_set_pivot(bank_img, Bank.header.w / 2, Bank.header.h / 2);

  verticalPointer_img = lv_img_create(scr);
  lv_img_set_src(verticalPointer_img, &VerticalPointer);
  lv_obj_align(verticalPointer_img, LV_ALIGN_CENTER, VP_X_OFFSET, VP_Y_OFFSET);

  horizontalPointer_img = lv_img_create(scr);
  lv_img_set_src(horizontalPointer_img, &HorizontalPointer);
  lv_obj_align(horizontalPointer_img, LV_ALIGN_CENTER, HP_X_OFFSET, HP_Y_OFFSET);

  sariCaged_img = lv_img_create(scr);
  lv_img_set_src(sariCaged_img, &SARICaged);
  lv_obj_align(sariCaged_img, LV_ALIGN_CENTER, SARICAGED_X_OFFSET, SARICAGED_Y_OFFSET);
  lv_img_set_pivot(sariCaged_img, SARICaged.header.w, 0); // top-right
  lv_img_set_angle(sariCaged_img, 0);

  // Seed UI with neutral values and apply once
  mb_pitch_x10 = 0;
  mb_bank_x10  = 0;
  mb_rot_raw   = DCS_MID_CODE;
  mb_slip_raw  = DCS_MID_CODE;
  mb_bug_raw   = DCS_MID_CODE;
  mb_cage_raw  = 0;

  // Start pointers at their inactive ends
  mb_pointerHorRaw = 32767; // at/below this = no up travel
  mb_pointerVerRaw = 32767;     // zero left push

  // Force neutral position
  uint16_t DCS_MID = 32782;
  onSaiPitchChange(DCS_MID);
  onSaiBankChange(DCS_MID);
  onSaiSlipBallChange(DCS_MID);
  onSaiRateOfTurnChange(DCS_MID);
  onSaiManPitchAdjChange(DCS_MID);
  onSaiAttWarningFlagChange(65535);
  updateRendering();

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
