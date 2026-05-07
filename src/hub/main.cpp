#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>
#include <cstdint>

#include "message.h"
#include "dcsbios_handler.h"
#include "DCS_State_Checker.h"

static void addPeer(const uint8_t mac[6]) {
  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, mac, 6);
  peer.channel = 0;
  peer.encrypt = false;
  esp_err_t err = esp_now_add_peer(&peer);
  //Serial.printf("esp_now_add_peer: %s\n", esp_err_to_name(err));
}

static uint8_t BROADCAST_MAC[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF }; // Broadcast address

static constexpr size_t ESPNOW_MAX_PAYLOAD = 250;
static constexpr uint8_t TX_QUEUE_CAP = 24;

struct TxPacket {
  uint16_t key;
  uint8_t len;
  uint8_t data[ESPNOW_MAX_PAYLOAD];
};

static TxPacket txQueue[TX_QUEUE_CAP]{};
static uint8_t txTail = 0;
static uint8_t txCount = 0;
static volatile bool sendInFlight = false;
static volatile bool sendCompletionPending = false;
static volatile esp_now_send_status_t lastSendCompletionStatus = ESP_NOW_SEND_FAIL;
static uint32_t nextSendAttemptAt = 0;

static uint16_t computePacketKey(const uint8_t* data, size_t len) {
  if (len < sizeof(MessageHeader)) {
    return 0xFFFF;
  }

  const MessageHeader* header = reinterpret_cast<const MessageHeader*>(data);
  if (header->category == MessageCategory::Integer && len >= sizeof(IntegerMessage)) {
    const IntegerMessage* msg = reinterpret_cast<const IntegerMessage*>(data);
    return static_cast<uint16_t>(0x100u | static_cast<uint8_t>(msg->name));
  }

  return static_cast<uint16_t>(header->category);
}

static bool enqueuePacket(const uint8_t* data, size_t len) {
  if (!data || len == 0 || len > ESPNOW_MAX_PAYLOAD) {
    return false;
  }

  const uint16_t key = computePacketKey(data, len);
  const uint8_t startOffset = sendInFlight ? 1 : 0;

  for (uint8_t i = startOffset; i < txCount; ++i) {
    uint8_t idx = (txTail + i) % TX_QUEUE_CAP;
    if (txQueue[idx].key == key) {
      txQueue[idx].key = key;
      txQueue[idx].len = static_cast<uint8_t>(len);
      memcpy(txQueue[idx].data, data, len);
      return true;
    }
  }

  if (txCount >= TX_QUEUE_CAP) {
    return false;
  }

  uint8_t idx = (txTail + txCount) % TX_QUEUE_CAP;
  txQueue[idx].key = key;
  txQueue[idx].len = static_cast<uint8_t>(len);
  memcpy(txQueue[idx].data, data, len);
  txCount++;
  return true;
}

static void handleSendCompletion() {
  if (!sendCompletionPending) {
    return;
  }

  sendCompletionPending = false;
  sendInFlight = false;

  if (txCount > 0) {
    txTail = (txTail + 1) % TX_QUEUE_CAP;
    txCount--;
  }
}

static void trySendNext() {
  if (sendInFlight || txCount == 0) {
    return;
  }

  const uint32_t now = millis();
  if (now < nextSendAttemptAt) {
    return;
  }

  TxPacket& pkt = txQueue[txTail];
  const uint8_t* dest = BROADCAST_MAC;
  esp_err_t e = esp_now_send(dest, pkt.data, pkt.len);

  if (e == ESP_OK) {
    sendInFlight = true;
    nextSendAttemptAt = 0;
  } else {
    // Back off slightly if the Wi-Fi task is temporarily out of buffers.
    nextSendAttemptAt = now + 10;
  }
}

static void onEspNowSendComplete(const esp_now_send_info_t* tx_info, esp_now_send_status_t status) {
  lastSendCompletionStatus = status;
  sendCompletionPending = true;
}

static void initEspNow() {
  delay(1000);
  WiFi.mode(WIFI_STA);
  esp_wifi_set_channel(ESP_CHANNEL, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_max_tx_power(ESP_MAX_TX_POWER);
  esp_err_t err = esp_now_init();
  //Serial.printf("esp_now_init: %s\n", esp_err_to_name(err));

  if (err == ESP_OK) {
    esp_now_register_send_cb(onEspNowSendComplete);
  }

  delay(100);
  addPeer(BROADCAST_MAC);
}

template<typename T>
static void sendMessage(const T& m) {
  enqueuePacket(reinterpret_cast<const uint8_t*>(&m), sizeof(m));
  trySendNext();
}

static void sendIntegerMessage(ValueName name, uint16_t value) {
  IntegerMessage m{};
  m.header.ms = millis();
  m.name = name;
  m.value = value;
  sendMessage(m);
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

static const uint32_t messageInterval = 40; // 1000 / messageInterval Hz max
static const uint32_t ifeiMessageInterval = 200;
static uint32_t lastSendAt = 0;
static uint32_t lastIfeiSendAt = 0;

void setup() {
  static_assert(sizeof(IntegerMessage) <= ESPNOW_MAX_PAYLOAD, "IntegerMessage too large for ESP-NOW");
  static_assert(sizeof(AltimeterMessage) <= ESPNOW_MAX_PAYLOAD, "AltimeterMessage too large for ESP-NOW");
  static_assert(sizeof(RadarAltimeterMessage) <= ESPNOW_MAX_PAYLOAD, "RadarAltimeterMessage too large for ESP-NOW");
  static_assert(sizeof(IfeiMessage) <= ESPNOW_MAX_PAYLOAD, "IfeiMessage too large for ESP-NOW");
  static_assert(sizeof(SaiMessage) <= ESPNOW_MAX_PAYLOAD, "SaiMessage too large for ESP-NOW");
  DcsBios::setup();
  initEspNow();
  delay(300);
  ifeiShowGameInfo();
}

void loop() {
  handleSendCompletion();
  trySendNext();
  DcsBios::loop();

  const uint32_t now = millis();
  DcsState dcsState = getDcsState();

  if (dcsState == DcsState::HUNG) {
    cleanup();
    ifeiShowGameInfo();
  }

  // Limit IFEI refresh rate due to its data size and update frequency
  if (now - lastIfeiSendAt > ifeiMessageInterval && !isEqualIfeiMessage(ifei, previousIfei)) {
    previousIfei = ifei;
    previousIfei.header.ms = millis();
    sendMessage(previousIfei);
    lastIfeiSendAt = now;
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

    if (!isEqualRadarAltimeterMessage(radarAltimeter, previousRadarAltimeter)) {
      previousRadarAltimeter = radarAltimeter;
      previousRadarAltimeter.header.ms = millis();
      sendMessage(previousRadarAltimeter);
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

    if (voltU != previousVoltU) {
      previousVoltU = voltU;
      sendIntegerMessage(ValueName::VoltU, voltU);
    }

    if (voltE != previousVoltE) {
      previousVoltE = voltE;
      sendIntegerMessage(ValueName::VoltE, voltE);
    }

    if (!isEqualAltimeterMessage(altimeter, previousAltimeter)) {
      previousAltimeter = altimeter;
      previousAltimeter.header.ms = millis();
      sendMessage(previousAltimeter);
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

    lastSendAt = now;
  }

  trySendNext();
}
