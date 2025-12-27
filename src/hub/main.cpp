#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
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
  esp_wifi_set_channel(ESP_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_max_tx_power(ESP_MAX_TX_POWER);
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

static MissionType previousMissionType = MissionType::Other;
static IfeiMessage previousIfei{};
static AltimeterMessage previousAltimeter{};
static RadarAltimeterMessage previousRadarAltimeter{};
static SaiMessage previousSai{};
static uint16_t previousAirspeed;
static uint16_t previousVsi;
static uint16_t previousVoltU;
static uint16_t previousVoltE;
static uint16_t previousHydIndBrake;
static uint16_t previousCabinAltIndicator;
static uint16_t previousHydPressL;
static uint16_t previousHydPressR;
static uint16_t previousInstrumentLighting;
static uint16_t previousConsoleLighting;

static const uint32_t messageInterval = 33; // 1000 / messageInterval Hz max
static const uint32_t ifeiMessageInterval = 250;
static const uint32_t periodicMessageInterval = 5000;
static uint32_t lastSendAt = 0;
static uint32_t lastIfeiSendAt = 0;
static uint32_t lastPeriodicSendAt = 0; // To resend values that seldom change and might be missed (e.g. hyd pressure, battery, hyd indicator brake)

void loop() {
  DcsBios::loop();

  const uint32_t now = millis();

  if (missionType != previousMissionType) {
    // Any cleanup?
  }

  // Limit IFEI refresh rate due to its data size and update frequency
  if (now - lastIfeiSendAt > ifeiMessageInterval && !isEqualIfeiMessage(ifei, previousIfei)) {
    previousIfei = ifei;
    previousIfei.header.ms = millis();
    sendMessage(previousIfei);
    lastIfeiSendAt = now;
  }

  bool periodicSend = previousMissionType == MissionType::Hornet && now - lastPeriodicSendAt > periodicMessageInterval;
  if (periodicSend) {
    lastPeriodicSendAt = now;
  }

  if (now - lastSendAt > messageInterval) {
    if (missionType != previousMissionType) {
      previousMissionType = missionType;
      sendIntegerMessage(ValueName::MissionChanged, static_cast<uint8_t>(missionType));
    }

    if (instrumentLighting != previousInstrumentLighting) {
      previousInstrumentLighting = instrumentLighting;
      sendIntegerMessage(ValueName::InstrumentLighting, instrumentLighting);
    }

    if (consoleLighting != previousConsoleLighting) {
      previousConsoleLighting = consoleLighting;
      sendIntegerMessage(ValueName::ConsoleLighting, consoleLighting);
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

    if (!isEqualSaiMessage(sai, previousSai)) {
      previousSai = sai;
      previousSai.header.ms = millis();
      sendMessage(previousSai);
    }

    if (airspeed != previousAirspeed) {
      previousAirspeed = airspeed;
      sendIntegerMessage(ValueName::Airspeed, airspeed);
    }

    if (vsi != previousVsi) {
      previousVsi = vsi;
      sendIntegerMessage(ValueName::VerticalVelocityIndicator, vsi);
    }

    if (voltU != previousVoltU || periodicSend) {
      previousVoltU = voltU;
      sendIntegerMessage(ValueName::VoltU, voltU);
    }

    if (voltE != previousVoltE || periodicSend) {
      previousVoltE = voltE;
      sendIntegerMessage(ValueName::VoltE, voltE);
    }

    if (hydIndBrake != previousHydIndBrake || periodicSend) {
      previousHydIndBrake = hydIndBrake;
      sendIntegerMessage(ValueName::BrakePressure, hydIndBrake);
    }

    if (cabinAltIndicator != previousCabinAltIndicator || periodicSend) {
      previousCabinAltIndicator = cabinAltIndicator;
      sendIntegerMessage(ValueName::CabinAltitudeIndicator, cabinAltIndicator);
    }

    if (hydPressL != previousHydPressL || periodicSend) {
      previousHydPressL = hydPressL;
      sendIntegerMessage(ValueName::HydraulicPressureLeft, hydPressL);
    }

    if (hydPressR != previousHydPressR || periodicSend) {
      previousHydPressR = hydPressR;
      sendIntegerMessage(ValueName::HydraulicPressureRight, hydPressR);
    }

    lastSendAt = now;
  }
}
