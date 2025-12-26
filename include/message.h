#pragma once

#include <cstring>

#ifndef ESP_CHANNEL
  #define ESP_CHANNEL 1
#endif

#ifndef ESP_MAX_TX_POWER
  #define ESP_MAX_TX_POWER 20
#endif

#pragma pack(push, 1)
enum class MessageCategory : uint8_t {
  Common,
  IFEI,
  Altimeter,
  RadarAltimeter,
  Integer,
  SAI,
};

enum class ValueName : uint8_t;

struct __attribute__((packed)) MessageHeader {
  MessageCategory category;
  uint32_t ms;       // millis() at send time
};

struct __attribute__((packed)) IntegerMessage {
  MessageHeader header{category: MessageCategory::Integer};
  ValueName name;
  uint16_t value;
};

struct __attribute__((packed)) AltimeterMessage {
  MessageHeader header{category: MessageCategory::Altimeter};

  uint16_t alt100FtPtr;
  uint16_t alt1000FtCnt;
  uint16_t alt10000FtCnt;
  uint16_t pressSet0;
  uint16_t pressSet1;
  uint16_t pressSet2;
};

struct __attribute__((packed)) RadarAltimeterMessage {
  MessageHeader header{category: MessageCategory::RadarAltimeter};

  uint16_t altPtr;
  uint16_t minHeightPtr;
  uint16_t offFlag;
  uint16_t greenLamp;
  uint16_t warnLt;
};

struct __attribute__((packed)) IfeiMessage {
  MessageHeader header{category: MessageCategory::IFEI};

  uint8_t clockH;
  uint8_t clockM;
  uint8_t clockS;

  uint8_t timerH;
  uint8_t timerM;
  uint8_t timerS;

  uint16_t bingo;

  uint8_t dd1; // ":"
  uint8_t dd2;
  uint8_t dd3;
  uint8_t dd4;

  uint8_t ffL;
  uint8_t ffR;

  uint8_t rpmL;
  uint8_t rpmR;

  uint16_t tempL;
  uint16_t tempR;

  uint16_t oilPressL;
  uint16_t oilPressR;
  uint16_t extNozzlePosL;
  uint16_t extNozzlePosR;

  char fuelUp[8];
  char fuelDown[8];
  char timeSetMode[8];
  char t[8];
  char sp[4];
  char codes[4];

  uint8_t bingoTex;
  uint8_t ffTex;

  uint8_t lTex;
  uint8_t l0Tex;
  uint8_t l50Tex;
  uint8_t l100Tex;
  uint8_t lPointerTex;
  uint8_t lScaleTex;

  uint8_t rTex;
  uint8_t r0Tex;
  uint8_t r50Tex;
  uint8_t r100Tex;
  uint8_t rPointerTex;
  uint8_t rScaleTex;

  uint8_t nozTex;
  uint8_t oilTex;
  uint8_t rpmTex;
  uint8_t tempTex;
  uint8_t zTex;

  uint16_t dispIntLt;
  uint16_t colorMode;
};

struct __attribute__((packed)) SaiMessage {
  MessageHeader header{category: MessageCategory::SAI};

  uint16_t slipBall;
  uint16_t bank;
  uint16_t rateOfTurn;
  uint16_t manPitchAdj;
  uint16_t pitch;
  uint16_t attWarningFlag;
  uint16_t pointerHor;
  uint16_t pointerVer;
};
#pragma pack(pop)

enum class MissionType : uint8_t {
  Hornet,
  Other, // Other aircraft types or mission end
};

enum class ValueName : uint8_t {
  Airspeed,
  VerticalVelocityIndicator,
  VoltU,
  VoltE,
  BrakePressure,
  CabinAltitudeIndicator,
  HydraulicPressureLeft,
  HydraulicPressureRight,
  InstrumentLighting,
  ConsoleLighting,
  MissionChanged,
};

static bool isEqualAltimeterMessage(const AltimeterMessage& a, const AltimeterMessage& b) {
  constexpr size_t off = offsetof(AltimeterMessage, alt100FtPtr);
  return std::memcmp(reinterpret_cast<const uint8_t*>(&a) + off,
                     reinterpret_cast<const uint8_t*>(&b) + off,
                     sizeof(AltimeterMessage) - off) == 0;
}

static bool isEqualRadarAltimeterMessage(const RadarAltimeterMessage& a, const RadarAltimeterMessage& b) {
  constexpr size_t off = offsetof(RadarAltimeterMessage, altPtr);
  return std::memcmp(reinterpret_cast<const uint8_t*>(&a) + off,
                     reinterpret_cast<const uint8_t*>(&b) + off,
                     sizeof(RadarAltimeterMessage) - off) == 0;
}

static bool isEqualIfeiMessage(const IfeiMessage& a, const IfeiMessage& b) {
  constexpr size_t off = offsetof(IfeiMessage, clockH);
  return std::memcmp(reinterpret_cast<const uint8_t*>(&a) + off,
                     reinterpret_cast<const uint8_t*>(&b) + off,
                     sizeof(IfeiMessage) - off) == 0;
}

static bool isEqualSaiMessage(const SaiMessage& a, const SaiMessage& b) {
  constexpr size_t off = offsetof(SaiMessage, slipBall);
  return std::memcmp(reinterpret_cast<const uint8_t*>(&a) + off,
                     reinterpret_cast<const uint8_t*>(&b) + off,
                     sizeof(SaiMessage) - off) == 0;
}
