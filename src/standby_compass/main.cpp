#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <lvgl.h>
#include "LVGL_Driver.h"
#include "message.h"
#include "renderer.h"

static volatile uint16_t brightness = 0;
static volatile uint16_t headingRaw = 0; // 0-65535
static volatile bool dirty = true;
static volatile bool brightnessDirty = true;
static uint32_t lastFrameUs = 0;
static const uint32_t FRAME_INTERVAL_US = 33333;  // ~30 FPS
static const bool kTestLoopEnabled = false;
static const uint32_t kTestHeadingRawPerSecond = 5461; // ~30 degrees/second
static const uint8_t kMinimumBacklight = 10;
static const uint8_t kMaximumBacklight = 80;

static void setCompassBrightness(uint16_t value) {
  static uint8_t oldValue = 0xff;
  const uint8_t newValue = (uint8_t)map(value, 0, 65535, kMinimumBacklight, kMaximumBacklight);
  if (oldValue != newValue) {
    oldValue = newValue;
    Set_Backlight(newValue);
  }
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
    IntegerMessage message{};
    switch (hdr->category) {
    case MessageCategory::Integer:
      if (len != (int)sizeof(IntegerMessage)) {
        return;
      }
      message = *reinterpret_cast<const IntegerMessage *>(data);
      if (message.name == ValueName::MagneticHeading) {
        headingRaw = message.value;
        dirty = true;
      } else if (message.name == ValueName::InstrumentLighting) {
        brightness = message.value;
        brightnessDirty = true;
      }
      break;
    default:
      return;
    }
  });
}

void setup() {
  Serial.begin(115200);

  delay(1000);
  LCD_Backlight = kMinimumBacklight;
  LCD_Init();
  setCompassBrightness(brightness);
  Renderer_Init();
  lastFrameUs = micros();

  if (!kTestLoopEnabled) {
    initEspNowClient();
  }
}

void loop() {
  const uint32_t now = micros();
  const bool frameDue = (now - lastFrameUs) >= FRAME_INTERVAL_US;

  if (brightnessDirty) {
    noInterrupts();
    const uint16_t b = brightness;
    brightnessDirty = false;
    interrupts();

    setCompassBrightness(b);
  }

  if (kTestLoopEnabled && frameDue) {
    lastFrameUs += FRAME_INTERVAL_US;

    static uint32_t testHeadingRaw = 0;
    testHeadingRaw = (testHeadingRaw + (kTestHeadingRawPerSecond / 30)) & 0xffff;
    Renderer_UpdateHeadingRaw((uint16_t)testHeadingRaw);
    return;
  }

  if (dirty && frameDue) {
    lastFrameUs += FRAME_INTERVAL_US;

    noInterrupts();
    const uint16_t h = headingRaw;
    dirty = false;
    interrupts();

    Renderer_UpdateHeadingRaw(h);
  }
}
