#pragma once

#pragma pack(push, 1)
enum class MessageCategory : uint8_t {
  Common,
  IFEI,
  Airspeed,
  Altimeter,
  AttitudeIndicator,
  VerticalVelocityIndicator,
  RadarAltimeter,
  Battery,
  BrakePressure,
  CabinPressure,
  HydraulicPressure,
};

enum class MessageName : uint8_t;
const uint8_t MessageDataLimit = 48;

struct __attribute__((packed)) MessageHeader {
  MessageCategory category;
  uint32_t ms;       // millis() at send time
};

struct __attribute__((packed)) Message {
  MessageHeader header;
  MessageName name;
  char data[MessageDataLimit];
};

struct __attribute__((packed)) AltimeterMessage {
  MessageHeader header;

  uint16_t alt100FtPtr;
  uint16_t alt1000FtCnt;
  uint16_t alt10000FtCnt;
  uint16_t pressSet0;
  uint16_t pressSet1;
  uint16_t pressSet2;
};

struct __attribute__((packed)) RadarAltimeterMessage {
  MessageHeader header;

  uint16_t altPtr;
  uint16_t minHeightPtr;
  uint16_t offFlag;
  uint16_t greenLamp;
  uint16_t warnLt;
};

struct __attribute__((packed)) IfeiMessage {
  MessageHeader header;

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
#pragma pack(pop)

inline Message makeMessage(MessageCategory category,
                           MessageName name,
                           const char* data) {
  MessageHeader header{};
  header.category = category;
  header.ms = millis();
  Message m{};
  m.header = header;
  m.name = name;

  if (data && *data) {
    size_t n = strnlen(data, sizeof(m.data));
    assert(n < MessageDataLimit);
    memcpy(m.data, data, n);
  } else {
    memset(m.data, ' ', sizeof(m.data));
  }

  return m;
}

enum class MessageName: uint8_t {
  None = 0,
  Ifei,
};
