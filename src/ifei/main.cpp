#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include "message.h"
#include "renderer.h"

static IfeiMessage lastMessage{};
static volatile uint32_t lastMessageMs = 0;

static void initEspNowClient() {
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(ESP_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_max_tx_power(ESP_MAX_TX_POWER);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb([](const uint8_t* mac, const uint8_t* data, int len) {
    if (len < (int)sizeof(MessageHeader)) {
      return;
    }

    const MessageHeader* hdr = reinterpret_cast<const MessageHeader*>(data);
    if (hdr->category == MessageCategory::IFEI) {
      lastMessage = *reinterpret_cast<const IfeiMessage *>(data);
      lastMessageMs = millis();
    }
  });
}

void setup() {
  Serial.begin(115200);
  initIfeiRenderer();
  initEspNowClient();
}

void loop() {
  const uint32_t now = millis();

  if (now - (uint32_t)lastMessageMs > 100) {
    lastMessageMs = now;
    renderIfeiMessage(lastMessage);
  }
}
