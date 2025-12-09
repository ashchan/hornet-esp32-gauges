#pragma once
#include <cstdint>

#pragma pack(push, 1)
struct Message {
  uint16_t type;     // 1 = heartbeat
  uint16_t seq;      // increments
  uint32_t ms;       // millis() at send time
};
#pragma pack(pop)