#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <lvgl.h>
#include "LVGL_Driver.h"
#include "message.h"
#include "renderer.h"

static volatile uint16_t brightness = 0;
static volatile uint16_t heading = 0; // 0-359
static volatile bool dirty = true;
static uint32_t lastFrameMs = 0;
static const uint32_t FRAME_INTERVAL_MS = 33;  // ~30 FPS

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
        heading = message.value;
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

  LCD_Init();
  Lvgl_Init();
  Renderer_Init();

  initEspNowClient();
}

void loop() {
  Timer_Loop();

  const uint32_t now = millis();
  const bool frameDue = (now - lastFrameMs) >= FRAME_INTERVAL_MS;

  if (dirty && frameDue) {
    noInterrupts();
    const uint16_t h = heading;
    const uint16_t b = brightness;
    dirty = false;
    interrupts();

    lastFrameMs = now;
  }
}
