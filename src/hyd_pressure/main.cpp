/*
F/A-18 Hydraulic Pressure Gauge — GC9A01 1.28" Round TFT (240x240) + ESP32 (Tanarg)
Library: TFT_eSPI (configure panel & pins in User_Setup.h)

Stability design:
- No drawing in DCS-BIOS callbacks (ISR-safe): callbacks store raw + mark dirty
- Sprites created once and reused (vs create/delete churn)
- ~30 FPS render throttle + 500 ms watchdog refresh while DCS active
*/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <TFT_eSPI.h>
#include "128/TFT_helper.h"
#include "message.h"

// ── Assets ─────────────────────────────────────────────────────────────────────
#include "hydPressBackground.h" // uint16_t/uint8_t array (sized 240x240)
#include "hydPressNeedle1.h"    // 33x120
#include "hydPressNeedle2.h"    // 33x120

// ── Display constants ──────────────────────────────────────────────────────────
const byte colorDepth = 16;            // keep original: 8 bpp sprites
static const uint16_t W = 240, H = 240;

// ── TFT & Sprites ──────────────────────────────────────────────────────────────
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite gaugeBack(&tft);  // background canvas
TFT_eSprite needle1(&tft);    // left/right as per art
TFT_eSprite needle2(&tft);

// ── DCS-driven state (updated in ISR-safe callbacks) ───────────────────────────
static volatile uint16_t raw1 = 0;      // left hyd (0x750e)
static volatile uint16_t raw2 = 0;      // right hyd (0x7510)
static volatile uint16_t brightness = 0;
static volatile bool dirty = true;
static volatile uint32_t lastDcsMs = 0;

// ── Render scheduler ───────────────────────────────────────────────────────────
static uint32_t lastFrameMs = 0;
static const uint32_t FRAME_INTERVAL_MS = 33;  // ~30 FPS
static const uint32_t WATCHDOG_MS       = 500; // periodic refresh while active

// ── Mapping: DCS raw -> gauge angle (degrees) ──────────────────────────────────
// Original mapping preserved: 0..65535 -> -280..40
static inline int16_t map_hyd(uint16_t v) {
  return map(v, 0, 65535, -280, 40);
}

// ── Forward decls ──────────────────────────────────────────────────────────────
void renderGauge(int16_t a1, int16_t a2);
void bitTest();

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
    IntegerMessage message{};
    switch (hdr->category) {
    case MessageCategory::Integer:
      if (len != (int)sizeof(IntegerMessage)) {
        return;
      }
      message = *reinterpret_cast<const IntegerMessage *>(data);
      if (message.name == ValueName::HydraulicPressureLeft) {
        raw1 = message.value;
        dirty = true;
      }
      if (message.name == ValueName::HydraulicPressureRight) {
        raw2 = message.value;
        dirty = true;
      }
      if (message.name == ValueName::InstrumentLighting) {
        brightness = message.value;
        dirty = true;
      }
      break;
    default:
      return;
    }
  });
}

// ── Setup ──────────────────────────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  tft.init();
  tft.fillScreen(TFT_BLACK);
  setBrightness(brightness);

  // Create background canvas once
  gaugeBack.setColorDepth(colorDepth);
  gaugeBack.createSprite(W, H);
  gaugeBack.setSwapBytes(true);
  gaugeBack.setPivot(W/2, H/2);

  // Create needle sprites once (keep your original geometry & pivots)
  needle1.setColorDepth(colorDepth);
  needle1.createSprite(33, 120);
  needle1.setSwapBytes(true);
  needle1.setPivot(17, 103);
  needle1.pushImage(0, 0, 33, 120, hydPressNeedle1);

  needle2.setColorDepth(colorDepth);
  needle2.createSprite(33, 120);
  needle2.setSwapBytes(true);
  needle2.setPivot(17, 103);
  needle2.pushImage(0, 0, 33, 120, hydPressNeedle2);

  // Initial paint
  renderGauge(map_hyd(raw1), map_hyd(raw2));

  initEspNowClient();
}

// ── Main loop ──────────────────────────────────────────────────────────────────
void loop() {
  const uint32_t now = millis();
  const bool frameDue   = (now - lastFrameMs) >= FRAME_INTERVAL_MS;
  const bool dcsActive  = (now - lastDcsMs) < 2000;
  const bool watchdog   = dcsActive && ((now - lastFrameMs) >= WATCHDOG_MS);

  if ((dirty && frameDue) || watchdog) {
    // snapshot once to avoid tearing
    noInterrupts();
    const uint16_t r1 = raw1;
    const uint16_t r2 = raw2;
    dirty = false;
    interrupts();

    renderGauge(map_hyd(r1), map_hyd(r2));
    setBrightness(brightness);
    lastFrameMs = now;
  }
}

// ── Rendering (heavy work stays out of callbacks) ──────────────────────────────
void renderGauge(int16_t angle1, int16_t angle2) {
  gaugeBack.fillSprite(TFT_BLACK);
  gaugeBack.pushImage(0, 0, W, H, hydPressBackground);

  needle1.pushRotated(&gaugeBack, angle1, TFT_TRANSPARENT);
  needle2.pushRotated(&gaugeBack, angle2, TFT_TRANSPARENT);

  gaugeBack.pushSprite(0, 0);
}

// ── Manual sweep test (optional) ───────────────────────────────────────────────
void bitTest() {
  // Needle 1 sweep while needle 2 holds min (80° visual)
  for (int i = 0; i <= 320; i += 5) {
    renderGauge(map(i, 0, 320, -280, 40), 80);
    delay(10);
  }
  for (int i = 320; i >= 0; i -= 5) {
    renderGauge(map(i, 0, 320, -280, 40), 80);
    delay(10);
  }
  // Needle 2 sweep while needle 1 holds min (80° visual)
  for (int i = 0; i <= 320; i += 5) {
    renderGauge(80, map(i, 0, 320, -280, 40));
    delay(10);
  }
  for (int i = 320; i >= 0; i -= 5) {
    renderGauge(80, map(i, 0, 320, -280, 40));
    delay(10);
  }
}