/*
F/A-18 Battery Gauge — GC9A01 1.28" Round TFT (240x240) + ESP32 ( Origianlly based on Tanarg dev board)
Library: TFT_eSPI (configure your GC9A01 and pins in User_Setup.h)

Key stability changes:
- NO drawing in DCS-BIOS callbacks (ISR-safe): callbacks only store values + mark dirty.
- Sprites created once in setup() and reused; no per-frame create/delete heap churn.
- Render throttled in loop() (~30 FPS) with a watchdog refresh to avoid “blanking”.  We'll see how well it works!!
*/

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <TFT_eSPI.h>
#include "128/TFT_helper.h"
#include "message.h"

#include "BatteryBackground.h" // uint16_t Battery[240*240]
#include "Needle.h"            // uint16_t Needle[15*88]

static const uint8_t  COLOR_DEPTH = 16;
static const uint16_t CANVAS_W = 240, CANVAS_H = 240;

// ── TFT & Sprites ───────────────────────────────────────────────────────────────
TFT_eSPI tft = TFT_eSPI();
TFT_eSprite gaugeBack(&tft);   // full-frame canvas
TFT_eSprite needleU(&tft);     // left needle
TFT_eSprite needleE(&tft);     // right needle

// ── State updated by DCS callbacks (ISR-safe) ───────────────────────────────────
static volatile uint16_t rawU = 0;
static volatile uint16_t rawE = 0;
static volatile bool dirty = false;
static volatile uint16_t brightness = 0;

// ── Render scheduler ───────────────────────────────────────────────────────────
static uint32_t lastFrameMs = 0;
static const uint32_t FRAME_INTERVAL_MS = 33;  // ~30 FPS
static const uint32_t WATCHDOG_MS       = 500; // force refresh if DCS active

// ── Forward decls ──────────────────────────────────────────────────────────────
static inline int16_t map_u(uint16_t v) { return map(v, 0, 65535, -150, -30); } // left needle (U)
static inline int16_t map_e(uint16_t v) { return map(v, 0, 65535,  150,  30); } // right needle (E)
void renderGauge(int16_t angleU, int16_t angleE);
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
      if (message.name == ValueName::VoltE) {
        rawE = message.value;
        dirty = true;
      }
      if (message.name == ValueName::VoltE) {
        rawU = message.value;
        dirty = true;
      }
      if (message.name == ValueName::ConsoleLighting) {
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
  // Serial for debug (optional)
  Serial.begin(115200);

  tft.begin();
  tft.fillScreen(TFT_BLACK);
  setBrightness(brightness);
  tft.setSwapBytes(true); // for 16-bit image arrays

  // Create sprites once; reuse forever
  gaugeBack.setColorDepth(COLOR_DEPTH);
  gaugeBack.createSprite(CANVAS_W, CANVAS_H);
  gaugeBack.setSwapBytes(true);
  gaugeBack.setPivot(CANVAS_W/2, CANVAS_H/2);

  needleU.setColorDepth(COLOR_DEPTH);
  needleU.createSprite(15, 88);
  needleU.setSwapBytes(true);
  needleU.setPivot(7, 84);
  needleU.pushImage(0, 0, 15, 88, Needle);

  needleE.setColorDepth(COLOR_DEPTH);
  needleE.createSprite(15, 88);
  needleE.setSwapBytes(true);
  needleE.setPivot(7, 84);
  needleE.pushImage(0, 0, 15, 88, Needle);

  // First paint
  renderGauge(map_u(rawU), map_e(rawE));

  initEspNowClient();
}

// ── Main loop ──────────────────────────────────────────────────────────────────
void loop() {
  const uint32_t now = millis();
  const bool frameDue = (now - lastFrameMs) >= FRAME_INTERVAL_MS;
  const bool watchdogKick = (now - lastFrameMs) >= WATCHDOG_MS;

  if ((dirty && frameDue) || watchdogKick) {
    // Snapshot volatile values once (avoid tearing)
    noInterrupts();
    const uint16_t u = rawU;
    const uint16_t e = rawE;
    dirty = false;
    interrupts();

    renderGauge(map_u(u), map_e(e));
    setBrightness(brightness);
    lastFrameMs = now;
  }

  yield();
 }

// ── Rendering (heavy work lives here, not in callbacks) ────────────────────────
void renderGauge(int16_t angleU, int16_t angleE) {
  // Clear canvas + draw static background
  //gaugeBack.fillSprite(TFT_BLACK);
  gaugeBack.pushImage(0, 0, CANVAS_W, CANVAS_H, Battery);

  // Rotate needles into the canvas (transparent background)
  needleU.pushRotated(&gaugeBack, angleU, TFT_TRANSPARENT);
  needleE.pushRotated(&gaugeBack, angleE, TFT_TRANSPARENT);

  // Push full frame to screen
  gaugeBack.pushSprite(0, 0);
}

// ── Optional: range test (manual BIT) ──────────────────────────────────────────
void bitTest() {
  for (int i = 0; i <= 120; i += 5) {
    renderGauge(map(i, 0, 120, -150, -30), map(i, 0, 120, 150, 30));
    delay(10);
  }
  for (int i = 120; i >= 0; i -= 5) {
    renderGauge(map(i, 0, 120, -150, -30), map(i, 0, 120, 150, 30));
    delay(10);
  }
}