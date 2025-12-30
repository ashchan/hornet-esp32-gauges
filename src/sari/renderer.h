#pragma once

#include "message.h"

#define DEFAULT_BRIGHTNESS 40

void initRenderer();
void render(SaiMessage message);
void setBrightness(uint16_t value = DEFAULT_BRIGHTNESS);