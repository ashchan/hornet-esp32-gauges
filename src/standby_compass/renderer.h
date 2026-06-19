#pragma once

#include <stdint.h>

void Renderer_Init(void);
void Renderer_UpdateHeading(uint16_t heading);
void Renderer_UpdateHeadingCentiDegrees(uint32_t headingCentiDegrees);
void Renderer_UpdateHeadingRaw(uint16_t headingRaw);
