#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
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

  delay(100);
  addPeer(BROADCAST_MAC);
}

template<typename T>
static void sendMessage(const T& m) {
  const uint8_t *dest = BROADCAST_MAC;
  esp_err_t e = esp_now_send(dest, reinterpret_cast<const uint8_t *>(&m), sizeof(m));
  //Serial.printf("esp_now_send %s\n", esp_err_to_name(e));
}

static void sendIntegerMessage(ValueName name, uint16_t value) {
  IntegerMessage m{};
  m.header.ms = millis();
  m.name = name;
  m.value = value;
  sendMessage(m);
}

void setup() {
  DcsBios::setup();
  initEspNow();
  delay(300);
}

static IfeiMessage previousIfei{};
static AltimeterMessage previousAltimeter{};
static RadarAltimeterMessage previousRadarAltimeter{};
static uint16_t previousAirspeed;
static uint16_t previousVsi;
static uint16_t previousVoltU;
static uint16_t previousVoltE;
static uint16_t previousHydIndBrake;
static uint16_t previousCabinAltIndicator;
static uint16_t previousHydPressL;
static uint16_t previousHydPressR;

void loop() {
  DcsBios::loop();

  static uint32_t messageInterval = 40; // 1000 / messageInterval Hz max
  static uint32_t ifeiMessageInterval = 250;
  static uint32_t lastSendAt = 0;
  static uint32_t lastIfeiSendAt = 0;
  const uint32_t now = millis();

  if (now - lastIfeiSendAt < ifeiMessageInterval) {
    return;
  }

  // Limit IFEI refresh rate due to its data size and update frequency
  if (now - lastSendAt > messageInterval * 5 && !isEqualIfeiMessage(ifei, previousIfei)) {
    previousIfei = ifei;
    previousIfei.header.ms = millis();
    sendMessage(previousIfei);
  }
  lastIfeiSendAt = now;

  if (now - lastSendAt < messageInterval) {
    return;
  }

  if (!isEqualAltimeterMessage(altimeter, previousAltimeter)) {
    previousAltimeter = altimeter;
    previousAltimeter.header.ms = millis();
    sendMessage(previousAltimeter);
  }

  if (!isEqualRadarAltimeterMessage(radarAltimeter, previousRadarAltimeter)) {
    previousRadarAltimeter = radarAltimeter;
    previousRadarAltimeter.header.ms = millis();
    sendMessage(previousRadarAltimeter);
  }

  if (airspeed != previousAirspeed) {
    previousAirspeed = airspeed;
    sendIntegerMessage(ValueName::Airspeed, airspeed);
  }

  if (vsi != previousVsi) {
    previousVsi = vsi;
    sendIntegerMessage(ValueName::VerticalVelocityIndicator, vsi);
  }

  if (voltU != previousVoltU) {
    previousVoltU = voltU;
    sendIntegerMessage(ValueName::VoltU, voltU);
  }

  if (voltE != previousVoltE) {
    previousVoltE = voltE;
    sendIntegerMessage(ValueName::VoltE, voltE);
  }

  if (hydIndBrake != previousHydIndBrake) {
    previousHydIndBrake = hydIndBrake;
    sendIntegerMessage(ValueName::BrakePressure, hydIndBrake);
  }

  if (cabinAltIndicator != previousCabinAltIndicator) {
    previousCabinAltIndicator = cabinAltIndicator;
    sendIntegerMessage(ValueName::CabinAltitudeIndicator, cabinAltIndicator);
  }

  if (hydPressL != previousHydPressL) {
    previousHydPressL = hydPressL;
    sendIntegerMessage(ValueName::HydraulicPressureLeft, hydPressL);
  }

  if (hydPressR != previousHydPressR) {
    previousHydPressR = hydPressR;
    sendIntegerMessage(ValueName::HydraulicPressureRight, hydPressR);
  }

  lastSendAt = now;
}
