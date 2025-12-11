#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_timer.h>
#include <cstdint>

#include "message.h"
#include "dcsbios_handler.h"

static void addPeer(const uint8_t mac[6]) {
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_err_t err = esp_now_add_peer(&peer);
  //Serial.printf("esp_now_add_peer: %s\n", esp_err_to_name(err));
}

// static uint8_t DISPLAY_MAC[6] = { 0x28, 0x37, 0x2F, 0x84, 0x66, 0xC8 }; // IFEI display
static uint8_t BROADCAST_MAC[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; // Broadcast address

static void initEspNow() {
  delay(1000);
  WiFi.mode(WIFI_STA);
  esp_err_t err = esp_now_init();
  //Serial.printf("esp_now_init: %s\n", esp_err_to_name(err));

  esp_now_register_send_cb([](const uint8_t*, esp_now_send_status_t s){
    //Serial.printf("send_cb: %s\n", esp_err_to_name(s));
  });

  delay(100);
  addPeer(BROADCAST_MAC);
}

static void sendMessage(const Message& m) {
  const uint8_t *dest = BROADCAST_MAC;
  esp_err_t e = esp_now_send(dest, reinterpret_cast<const uint8_t *>(&m), sizeof(m));
  //Serial.printf("esp_now_send %s\n", esp_err_to_name(e));
}

static void sendIfeiMessage() {
  const uint8_t *dest = BROADCAST_MAC;
  ifei.header.category = MessageCategory::IFEI;
  ifei.header.ms = millis();
  esp_err_t e = esp_now_send(dest, reinterpret_cast<const uint8_t *>(&ifei), sizeof(ifei));
  //Serial.printf("esp_now_send %s\n", esp_err_to_name(e));
}

static constexpr uint32_t messageInterval = 250; // ms: XHz max
static uint32_t lastSendMs = 0;

void setup() {
  DcsBios::setup();
  initEspNow();
  delay(300);
}

void loop() {
  DcsBios::loop();

  const uint32_t now = millis();
  if ((uint32_t)(now - lastSendMs) >= messageInterval) {
    lastSendMs = now;
    sendIfeiMessage();
  }
}
