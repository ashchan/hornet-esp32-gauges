#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

#define DCSBIOS_DEFAULT_SERIAL
#define DCSBIOS_DISABLE_SERVO
#include <DcsBios.h>

#include "message.h"

static constexpr bool USE_BROADCAST = false;
static uint8_t DISPLAY_MAC[6] = { 0x28, 0x37, 0x2F, 0x84, 0x66, 0xC8 }; // IFEI display
// Rate limit for heartbeat messages to clients (visible + low bandwidth)
static constexpr uint32_t HEARTBEAT_INTERVAL_MS = 250; // 4 Hz max


static volatile bool sawTraffic = false;
static uint32_t lastSendMs = 0;
static uint16_t seq = 0;

void onAddressChange(unsigned int/* newValue*/) {
    sawTraffic = true;
}
DcsBios::IntegerBuffer addressBuffer(0x0000, 0xffff, 0, onAddressChange);

// Optional: send status callback (helps debugging; safe even if unused)
static void onSent(const uint8_t* mac, esp_now_send_status_t status) {
  (void)mac;
  (void)status;
  // If you want debug later, you can toggle a pin here or add a counter.
}

// ------------------- ESP-NOW INIT -------------------
static void addPeer(const uint8_t mac[6]) {
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;      // 0 = current channel
  peer.encrypt = false;  // set true + keys if you later want encryption
  esp_err_t err = esp_now_add_peer(&peer);
  (void)err;
}

static void initEspNow() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect(true);

  if (esp_now_init() != ESP_OK) {
    // Hard fail: nothing useful without ESP-NOW
    while (true) delay(1000);
  }

  esp_now_register_send_cb(onSent);
  addPeer(DISPLAY_MAC);
}

static void sendHeartbeat() {
  Message m{};
  m.type = 1;
  m.seq = ++seq;
  m.ms = millis();

  const uint8_t *dest = DISPLAY_MAC;
  esp_now_send(dest, reinterpret_cast<const uint8_t *>(&m), sizeof(m));
}

void setup() {
  DcsBios::setup();
  initEspNow();
}

void loop() {
  DcsBios::loop();
  const uint32_t now = millis();
  if (sawTraffic && (now - lastSendMs) >= HEARTBEAT_INTERVAL_MS) {
      sawTraffic = false;
      lastSendMs = now;
      sendHeartbeat();
  }
}
