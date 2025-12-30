#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "message.h"
#include "renderer.h"


static portMUX_TYPE msgMux = portMUX_INITIALIZER_UNLOCKED;
static SaiMessage lastMessage{};
uint16_t brightness = 0;
volatile bool hasNewMessage = false;

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
      portENTER_CRITICAL_ISR(&msgMux);
      lastMessage = *reinterpret_cast<const SaiMessage *>(data);
      portEXIT_CRITICAL_ISR(&msgMux);
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

void setup() {
  Serial.begin(115200);

  initEspNowClient();

  initRenderer();
}

void loop() {
  const uint32_t now = millis();
  static uint32_t lastUpdatedAt = 0;
  if (now - lastUpdatedAt > 40 && hasNewMessage) {
    hasNewMessage = false;
    lastUpdatedAt = now;
    render(lastMessage);
  }
}
