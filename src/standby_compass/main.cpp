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
static uint32_t lastFrameUs = 0;
static const uint32_t FRAME_INTERVAL_US = 33333;  // ~30 FPS

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
  LCD_Init();
  Renderer_Init();
  lastFrameUs = micros();

  initEspNowClient();
}

void loop() {
  const uint32_t now = micros();
  const bool frameDue = (now - lastFrameUs) >= FRAME_INTERVAL_US;

  if (dirty && frameDue) {
    lastFrameUs += FRAME_INTERVAL_US;

    noInterrupts();
    const uint16_t h = headingRaw;
    dirty = false;
    interrupts();

    Renderer_UpdateHeadingRaw(h);
  }
}
