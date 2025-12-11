#pragma once

#define Serial Serial0 // SwitchScience ESPr Developer S3 TypeC (8331)
#define DCSBIOS_DEFAULT_SERIAL
#define DCSBIOS_DISABLE_SERVO
#include <DcsBios.h>
#include "message.h"

static IfeiMessage ifei{};

uint16_t parseU16(const char *s) {
  if (!s) {
    return 0;
  }

  // skip leading spaces
  while (*s == ' ' || *s == '\t') {
    ++s;
  }

  if (*s == '\0') {
    return 0;
  }

  uint32_t v = 0;
  bool any = false;

  while (*s >= '0' && *s <= '9') {
    any = true;
    v = v * 10u + (uint32_t)(*s - '0');
    if (v > 65535u) {
      v = 65535u; // clamp to uint16_t max
      break;
    }
    ++s;
  }

  if (!any) {
    return 0; // no digits found
  }

  return (uint16_t)v;
}

uint8_t parseU8(const char *s) {
  if (!s) {
    return 0;
  }

  // skip leading spaces
  while (*s == ' ' || *s == '\t') {
    ++s;
  }

  if (*s == '\0') {
    return 0;
  }

  uint16_t v = 0;
  bool any = false;

  while (*s >= '0' && *s <= '9') {
    any = true;
    v = (uint16_t)(v * 10u + (uint16_t)(*s - '0'));
    if (v > 255u) {
      v = 255u; // clamp to uint8_t max
      break;
    }
    ++s;
  }

  if (!any) {
    return 0; // no digits found
  }

  return (uint8_t)v;
}

//=============================== COMMON DATA  ===============================
//################## Light Modes  ##################

void onCockkpitLightModeSwChange(unsigned int newValue) {
  // TODO: implement if needed
}

DcsBios::IntegerBuffer cockkpitLightModeSwBuffer(FA_18C_hornet_COCKKPIT_LIGHT_MODE_SW, onCockkpitLightModeSwChange);

//=============================== IFEI  ===============================
//################## RPM  ##################
void onIfeiRpmLChange(char* newValue) {
  ifei.rpmL = parseU16(newValue);
}
DcsBios::StringBuffer<3> ifeiRpmLBuffer(FA_18C_hornet_IFEI_RPM_L_A, onIfeiRpmLChange);

void onIfeiRpmRChange(char* newValue) {
  ifei.rpmR = parseU16(newValue);
}
DcsBios::StringBuffer<3> ifeiRpmRBuffer(FA_18C_hornet_IFEI_RPM_R_A, onIfeiRpmRChange);

void onIfeiRpmTextureChange(char* newValue) {
  ifei.rpmTex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiRpmTextureBuffer(FA_18C_hornet_IFEI_RPM_TEXTURE_A, onIfeiRpmTextureChange);

// ################## TEMP  ##################
//Left
void onIfeiTempLChange(char* newValue) {
  ifei.tempL = parseU16(newValue);
}
DcsBios::StringBuffer<3> ifeiTempLBuffer(FA_18C_hornet_IFEI_TEMP_L_A, onIfeiTempLChange);

//Right
void onIfeiTempRChange(char* newValue) {
  ifei.tempR = parseU16(newValue);
}
DcsBios::StringBuffer<3> ifeiTempRBuffer(FA_18C_hornet_IFEI_TEMP_R_A, onIfeiTempRChange);

//Texture
void onIfeiTempTextureChange(char* newValue) {
  ifei.tempTex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiTempTextureBuffer(FA_18C_hornet_IFEI_TEMP_TEXTURE_A, onIfeiTempTextureChange);

//################## SP CODES  ##################
//SP
void onIfeiSpChange(char* newValue) {
  strlcpy(ifei.sp, newValue, sizeof(ifei.sp));
}
DcsBios::StringBuffer<3> ifeiSpBuffer(FA_18C_hornet_IFEI_SP_A, onIfeiSpChange);

//Codes
void onIfeiCodesChange(char* newValue) {
  strlcpy(ifei.codes, newValue, sizeof(ifei.codes));
}
DcsBios::StringBuffer<3> ifeiCodesBuffer(FA_18C_hornet_IFEI_CODES_A, onIfeiCodesChange);

//################## FUEL FLOW  ##################
//LEFT
void onIfeiFfLChange(char* newValue) {
  ifei.ffL = parseU8(newValue);
}
DcsBios::StringBuffer<3> ifeiFfLBuffer(FA_18C_hornet_IFEI_FF_L_A, onIfeiFfLChange);

//Right
void onIfeiFfRChange(char* newValue) {
  ifei.ffR = parseU8(newValue);
}
DcsBios::StringBuffer<3> ifeiFfRBuffer(FA_18C_hornet_IFEI_FF_R_A, onIfeiFfRChange);

//Texture
void onIfeiFfTextureChange(char* newValue) {
  ifei.ffTex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiFfTextureBuffer(FA_18C_hornet_IFEI_FF_TEXTURE_A, onIfeiFfTextureChange);

//################## OIL  ##################
//Left
void onIfeiOilPressLChange(char* newValue) {
  ifei.oilPressL = parseU16(newValue);
}
DcsBios::StringBuffer<3> ifeiOilPressLBuffer(FA_18C_hornet_IFEI_OIL_PRESS_L_A, onIfeiOilPressLChange);

//Right
void onIfeiOilPressRChange(char* newValue) {
  ifei.oilPressR = parseU16(newValue);
}
DcsBios::StringBuffer<3> ifeiOilPressRBuffer(FA_18C_hornet_IFEI_OIL_PRESS_R_A, onIfeiOilPressRChange);

//Texture
void onIfeiOilTextureChange(char* newValue) {
  ifei.oilTex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiOilTextureBuffer(FA_18C_hornet_IFEI_OIL_TEXTURE_A, onIfeiOilTextureChange);

//################## NOZZEL Gauges  ##################
//Left
//Pointer position
void onExtNozzlePosLChange(unsigned int newValue) {
  ifei.extNozzlePosL = newValue;
}
DcsBios::IntegerBuffer extNozzlePosLBuffer(FA_18C_hornet_EXT_NOZZLE_POS_L_A, 0xffff, 0, onExtNozzlePosLChange);

//Pointer visibility
void onIfeiLpointerTextureChange(char* newValue) {
  ifei.lPointerTex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiLpointerTextureBuffer(FA_18C_hornet_IFEI_LPOINTER_TEXTURE_A, onIfeiLpointerTextureChange);

//Scale visibility
void onIfeiLscaleTextureChange(char* newValue) {
  ifei.lScaleTex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiLscaleTextureBuffer(FA_18C_hornet_IFEI_LSCALE_TEXTURE_A, onIfeiLscaleTextureChange);

//Scale numbers visibility
void onIfeiL100TextureChange(char* newValue) {
  ifei.l100Tex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiL100TextureBuffer(FA_18C_hornet_IFEI_L100_TEXTURE_A, onIfeiL100TextureChange);

//Right
//Pointer position
void onExtNozzlePosRChange(unsigned int newValue) {
  ifei.extNozzlePosR = newValue;
}
DcsBios::IntegerBuffer extNozzlePosRBuffer(FA_18C_hornet_EXT_NOZZLE_POS_R_A, 0xffff, 0, onExtNozzlePosRChange);

//Pointer visibility
void onIfeiRpointerTextureChange(char* newValue) {
  ifei.rPointerTex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiRpointerTextureBuffer(FA_18C_hornet_IFEI_RPOINTER_TEXTURE_A, onIfeiRpointerTextureChange);

//Scale visibility
void onIfeiRscaleTextureChange(char* newValue) {
  ifei.rScaleTex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiRscaleTextureBuffer(FA_18C_hornet_IFEI_RSCALE_TEXTURE_A, onIfeiRscaleTextureChange);

//Scale numbers visibility
void onIfeiR100TextureChange(char* newValue) {
  ifei.r100Tex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiR100TextureBuffer(FA_18C_hornet_IFEI_R100_TEXTURE_A, onIfeiR100TextureChange);

//################## FUEL  ##################
//Upper
void onIfeiFuelUpChange(char* newValue) {
  strlcpy(ifei.fuelUp, newValue, sizeof(ifei.fuelUp));
}
DcsBios::StringBuffer<6> ifeiFuelUpBuffer(FA_18C_hornet_IFEI_FUEL_UP_A, onIfeiFuelUpChange);

//Time mode
void onIfeiTChange(char* newValue) {
  strlcpy(ifei.t, newValue, sizeof(ifei.t));
}
DcsBios::StringBuffer<6> ifeiTBuffer(FA_18C_hornet_IFEI_T_A, onIfeiTChange);

//Tag L
void onIfeiLTextureChange(char* newValue) {
  ifei.lTex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiLTextureBuffer(FA_18C_hornet_IFEI_L_TEXTURE_A, onIfeiLTextureChange);

//Lower
void onIfeiFuelDownChange(char* newValue) {
  strlcpy(ifei.fuelDown, newValue, sizeof(ifei.fuelDown));
}
DcsBios::StringBuffer<6> ifeiFuelDownBuffer(FA_18C_hornet_IFEI_FUEL_DOWN_A, onIfeiFuelDownChange);

//Time mode
void onIfeiTimeSetModeChange(char* newValue) {
  strlcpy(ifei.timeSetMode, newValue, sizeof(ifei.timeSetMode));
}
DcsBios::StringBuffer<6> ifeiTimeSetModeBuffer(FA_18C_hornet_IFEI_TIME_SET_MODE_A, onIfeiTimeSetModeChange);

//Tag R
void onIfeiRTextureChange(char* newValue) {
  ifei.rTex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiRTextureBuffer(FA_18C_hornet_IFEI_R_TEXTURE_A, onIfeiRTextureChange);

//################## BINGO ##################
//Texture
void onIfeiBingoTextureChange(char* newValue) {
  ifei.bingoTex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiBingoTextureBuffer(FA_18C_hornet_IFEI_BINGO_TEXTURE_A, onIfeiBingoTextureChange);

//Digits
void onIfeiBingoChange(char* newValue) {
  ifei.bingo = parseU16(newValue);
}
DcsBios::StringBuffer<5> ifeiBingoBuffer(FA_18C_hornet_IFEI_BINGO_A, onIfeiBingoChange);

//################## CLOCK ##################
//Upper
//Hours
void onIfeiClockHChange(char* newValue) {
  ifei.clockH = parseU8(newValue);
}
DcsBios::StringBuffer<2> ifeiClockHBuffer(FA_18C_hornet_IFEI_CLOCK_H_A, onIfeiClockHChange);

//Minutes
void onIfeiClockMChange(char* newValue) {
  ifei.clockM = parseU8(newValue);
}
DcsBios::StringBuffer<2> ifeiClockMBuffer(FA_18C_hornet_IFEI_CLOCK_M_A, onIfeiClockMChange);

//Seconds
void onIfeiClockSChange(char* newValue) {
  ifei.clockS = parseU8(newValue);
}
DcsBios::StringBuffer<2> ifeiClockSBuffer(FA_18C_hornet_IFEI_CLOCK_S_A, onIfeiClockSChange);

/*
//Colon 1
void onIfeiDd1Change(char* newValue) {
}
DcsBios::StringBuffer<1> ifeiDd1Buffer(FA_18C_hornet_IFEI_DD_1_A, onIfeiDd1Change);

//Colon 2
void onIfeiDd2Change(char* newValue) {
DcsBios::StringBuffer<1> ifeiDd2Buffer(FA_18C_hornet_IFEI_DD_2_A, onIfeiDd2Change);
}
*/

//Tag Z
void onIfeiZTextureChange(char* newValue) {
  ifei.zTex = parseU8(newValue);
}
DcsBios::StringBuffer<1> ifeiZTextureBuffer(FA_18C_hornet_IFEI_Z_TEXTURE_A, onIfeiZTextureChange);

//Lower
//Hours
void onIfeiTimerHChange(char* newValue) {
  ifei.timerH = parseU8(newValue);
}
DcsBios::StringBuffer<2> ifeiTimerHBuffer(FA_18C_hornet_IFEI_TIMER_H_A, onIfeiTimerHChange);

//Minutes
void onIfeiTimerMChange(char* newValue) {
  ifei.timerM = parseU8(newValue);
}
DcsBios::StringBuffer<2> ifeiTimerMBuffer(FA_18C_hornet_IFEI_TIMER_M_A, onIfeiTimerMChange);

//Seconds
void onIfeiTimerSChange(char* newValue) {
  ifei.timerS = parseU8(newValue);
}
DcsBios::StringBuffer<2> ifeiTimerSBuffer(FA_18C_hornet_IFEI_TIMER_S_A, onIfeiTimerSChange);
/*
//Colon 1
void onIfeiDd3Change(char* newValue) {
}
DcsBios::StringBuffer<1> ifeiDd3Buffer(FA_18C_hornet_IFEI_DD_3_A, onIfeiDd3Change);

//Colon 2
void onIfeiDd4Change(char* newValue) {
}
DcsBios::StringBuffer<1> ifeiDd4Buffer(FA_18C_hornet_IFEI_DD_4_A, onIfeiDd4Change);
*/


//################## Display Brightness ##################
//Only changes in night mode

void onIfeiDispIntLtChange(unsigned int newValue) {
  ifei.dispIntLt = newValue;
}
DcsBios::IntegerBuffer ifeiDispIntLtBuffer(FA_18C_hornet_IFEI_DISP_INT_LT_A, 0xffff, 0, onIfeiDispIntLtChange);
