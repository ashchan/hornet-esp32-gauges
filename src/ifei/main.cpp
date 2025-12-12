#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include "message.h"
#include "renderer.h"
#define Serial Serial0
IfeiMessage lastMessage = {};
static volatile uint32_t lastMessageMs = 0;

static void initEspNowClient() {
  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
    return;
  }

  esp_now_register_recv_cb([](const uint8_t* mac, const uint8_t* data, int len) {
    if (len < (int)sizeof(MessageHeader)) {
      return;
    }

    const MessageHeader* hdr = reinterpret_cast<const MessageHeader*>(data);
    switch (hdr->category) {
    case MessageCategory::IFEI:
      if (len != (int)sizeof(IfeiMessage)) {
        return;
      }
      lastMessage = *reinterpret_cast<const IfeiMessage *>(data);
      break;
    case MessageCategory::Common:
      if (len != (int)sizeof(Message)) {
        return;
      }
      // auto *msg = reinterpret_cast<const Message *>(data);
      // TODO
      break;
    default:
      // ignore
      return;
    }

    lastMessageMs = millis();
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
