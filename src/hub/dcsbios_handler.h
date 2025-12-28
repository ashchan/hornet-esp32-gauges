#pragma once

//#define Serial Serial0 // Redefine this if usb_cdc_on_boot is false, with SwitchScience ESPr Developer S3 TypeC (8331)
#define DCSBIOS_DEFAULT_SERIAL
#define DCSBIOS_DISABLE_SERVO
#include <DcsBios.h>
#include "message.h"

static MissionType missionType = MissionType::Other;
static AltimeterMessage altimeter{};
static RadarAltimeterMessage radarAltimeter{};
static IfeiMessage ifei{};
static SaiMessage sai{};
static uint16_t airspeed;
static uint16_t vsi;
static uint16_t voltU;
static uint16_t voltE;
static uint16_t hydIndBrake;
static uint16_t cabinAltIndicator;
static uint16_t hydPressL;
static uint16_t hydPressR;
static uint16_t instrumentLighting;
static uint16_t consoleLighting;

uint16_t parseU16(const char *s);
uint8_t parseU8(const char *s);

void reset() {
  altimeter = AltimeterMessage{};
  radarAltimeter = RadarAltimeterMessage{};
  ifei = IfeiMessage{};
  sai = SaiMessage{};
  airspeed = 0;
  vsi = 65535 / 2;
  voltU = 0;
  voltE = 0;
  hydIndBrake = 0;
  cabinAltIndicator = 0;
  hydPressL = 0;
  hydPressR = 0;
  instrumentLighting = 0;
  consoleLighting = 0;
}

#pragma region DCS Common Data
void onAcftNameBufferChange(char* newValue) {
  reset();
  missionType = strcmp(newValue, "FA-18C_hornet") == 0 ? MissionType::Hornet : MissionType::Other;
}
DcsBios::StringBuffer<16> AcftNameBuffer(MetadataStart_ACFT_NAME_A, onAcftNameBufferChange);

void onCockkpitLightModeSwChange(unsigned int newValue) {
  // TODO: other common message other than IFEI
  ifei.colorMode = newValue;
}
DcsBios::IntegerBuffer cockkpitLightModeSwBuffer(FA_18C_hornet_COCKKPIT_LIGHT_MODE_SW, onCockkpitLightModeSwChange);

void onInstrIntLtChange(unsigned int newValue) {
  instrumentLighting = newValue;
}
DcsBios::IntegerBuffer instrIntLtBuffer(FA_18C_hornet_INSTR_INT_LT, onInstrIntLtChange);

void onConsoleIntLtChange(unsigned int newValue) {
  consoleLighting = newValue;
}
DcsBios::IntegerBuffer consoleIntLtBuffer(FA_18C_hornet_CONSOLE_INT_LT, onConsoleIntLtChange);
#pragma endregion DCS Common Data

#pragma region Airspeed
void onStbyAsiAirspeedChange(unsigned int newValue) {
  airspeed = newValue;
}
DcsBios::IntegerBuffer stbyAsiAirspeedBuffer(FA_18C_hornet_STBY_ASI_AIRSPEED, onStbyAsiAirspeedChange);
#pragma endregion Airspeed

#pragma region Altimeter
void onStbyAlt100FtPtrChange(unsigned int newValue) {
  altimeter.alt100FtPtr = newValue;
}
DcsBios::IntegerBuffer stbyAlt100FtPtrBuffer(FA_18C_hornet_STBY_ALT_100_FT_PTR, onStbyAlt100FtPtrChange);

void onStbyAlt1000FtCntChange(unsigned int newValue) {
  altimeter.alt1000FtCnt = newValue;
}
DcsBios::IntegerBuffer stbyAlt1000FtCntBuffer(FA_18C_hornet_STBY_ALT_1000_FT_CNT, onStbyAlt1000FtCntChange);

void onStbyAlt10000FtCntChange(unsigned int newValue) {
  altimeter.alt10000FtCnt = newValue;
}
DcsBios::IntegerBuffer stbyAlt10000FtCntBuffer(FA_18C_hornet_STBY_ALT_10000_FT_CNT, onStbyAlt10000FtCntChange);

void onStbyPressSet0Change(unsigned int newValue) {
  altimeter.pressSet0 = newValue;
}
DcsBios::IntegerBuffer stbyPressSet0Buffer(FA_18C_hornet_STBY_PRESS_SET_0, onStbyPressSet0Change);

void onStbyPressSet1Change(unsigned int newValue) {
  altimeter.pressSet1 = newValue;
}
DcsBios::IntegerBuffer stbyPressSet1Buffer(FA_18C_hornet_STBY_PRESS_SET_1, onStbyPressSet1Change);

void onStbyPressSet2Change(unsigned int newValue) {
  altimeter.pressSet2 = newValue;
}
DcsBios::IntegerBuffer stbyPressSet2Buffer(FA_18C_hornet_STBY_PRESS_SET_2, onStbyPressSet2Change);
#pragma endregion Altimeter

#pragma region Vertical Velocity Indicator
void onVsiChange(unsigned int newValue) {
  vsi = newValue;
}
DcsBios::IntegerBuffer vsiBuffer(FA_18C_hornet_VSI, onVsiChange);
#pragma endregion Vertical Velocity Indicator

#pragma region Battery Voltage
void onVoltUChange(unsigned int v) {
  voltU = v;
}
DcsBios::IntegerBuffer voltUBuffer(FA_18C_hornet_VOLT_U, onVoltUChange);

void onVoltEChange(unsigned int v) {
  voltE = v;
}
DcsBios::IntegerBuffer voltEBuffer(FA_18C_hornet_VOLT_E, onVoltEChange);
#pragma endregion Battery Voltage

#pragma region Brake Pressure
void onHydIndBrakeChange(unsigned int v) {
  hydIndBrake = v;
}
DcsBios::IntegerBuffer hydIndBrakeBuffer(FA_18C_hornet_HYD_IND_BRAKE, onHydIndBrakeChange);
#pragma endregion Brake Pressure

#pragma region Cabin Pressure
void onCabinAltIndicatorChange(unsigned int newValue) {
  cabinAltIndicator = newValue;
}
DcsBios::IntegerBuffer cabinAltIndicatorBuffer(FA_18C_hornet_PRESSURE_ALT, onCabinAltIndicatorChange);
#pragma endregion Cabin Pressure

#pragma region Hydraulics Pressure
void onHydPressLChange(unsigned int newValue) {
  hydPressL = newValue;
}
DcsBios::IntegerBuffer hydPressLBuffer(FA_18C_hornet_HYD_IND_LEFT, onHydPressLChange);

void onHydPressRChange(unsigned int newValue) {
  hydPressR = newValue;
}
DcsBios::IntegerBuffer hydPressRBuffer(FA_18C_hornet_HYD_IND_RIGHT, onHydPressRChange);
#pragma endregion Hydraulics Pressure

#pragma region Radar Altimeter
void onRadaltMinHeightPtrChange(unsigned int newValue) {
  radarAltimeter.minHeightPtr = newValue;
}
DcsBios::IntegerBuffer radaltMinHeightPtrBuffer(FA_18C_hornet_RADALT_MIN_HEIGHT_PTR, onRadaltMinHeightPtrChange);

void onRadaltOffFlagChange(unsigned int newValue) {
  radarAltimeter.offFlag = newValue;
}
DcsBios::IntegerBuffer radaltOffFlagBuffer(FA_18C_hornet_RADALT_OFF_FLAG, onRadaltOffFlagChange);

void onRadaltGreenLampChange(unsigned int newValue) {
  radarAltimeter.greenLamp = newValue;
}
DcsBios::IntegerBuffer radaltGreenLampBuffer(FA_18C_hornet_RADALT_GREEN_LAMP, onRadaltGreenLampChange);

void onLowAltWarnLtChange(unsigned int newValue) {
  radarAltimeter.warnLt = newValue;
}
DcsBios::IntegerBuffer lowAltWarnLtBuffer(FA_18C_hornet_LOW_ALT_WARN_LT, onLowAltWarnLtChange);

void onRadaltAltPtrChange(unsigned int newValue) {
  radarAltimeter.altPtr = newValue;
}
DcsBios::IntegerBuffer radaltAltPtrBuffer(FA_18C_hornet_RADALT_ALT_PTR, onRadaltAltPtrChange);
#pragma endregion Radar Altimeter

#pragma region IFEI
//################## RPM  ##################
void onIfeiRpmLChange(char* newValue) {
  ifei.rpmL = parseU8(newValue);
}
DcsBios::StringBuffer<3> ifeiRpmLBuffer(FA_18C_hornet_IFEI_RPM_L_A, onIfeiRpmLChange);

void onIfeiRpmRChange(char* newValue) {
  ifei.rpmR = parseU8(newValue);
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

//Colon 1
void onIfeiDd1Change(char* newValue) {
  ifei.dd1 = (uint8_t)newValue[0];
}
DcsBios::StringBuffer<1> ifeiDd1Buffer(FA_18C_hornet_IFEI_DD_1_A, onIfeiDd1Change);

//Colon 2
void onIfeiDd2Change(char* newValue) {
  ifei.dd2 = (uint8_t)newValue[0];
}
DcsBios::StringBuffer<1> ifeiDd2Buffer(FA_18C_hornet_IFEI_DD_2_A, onIfeiDd2Change);

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

//Colon 1
void onIfeiDd3Change(char* newValue) {
  ifei.dd3 = (uint8_t)newValue[0];
}
DcsBios::StringBuffer<1> ifeiDd3Buffer(FA_18C_hornet_IFEI_DD_3_A, onIfeiDd3Change);

//Colon 2
void onIfeiDd4Change(char* newValue) {
  ifei.dd4 = (uint8_t)newValue[0];
}
DcsBios::StringBuffer<1> ifeiDd4Buffer(FA_18C_hornet_IFEI_DD_4_A, onIfeiDd4Change);

//################## Display Brightness ##################
//Only changes in night mode

void onIfeiDispIntLtChange(unsigned int newValue) {
  ifei.dispIntLt = newValue;
}
DcsBios::IntegerBuffer ifeiDispIntLtBuffer(FA_18C_hornet_IFEI_DISP_INT_LT_A, 0xffff, 0, onIfeiDispIntLtChange);
#pragma endregion IFEI

#pragma region SARI
void onSaiSlipBallChange(unsigned int v) {
  sai.slipBall = v;
}
DcsBios::IntegerBuffer saiSlipBallBuffer(FA_18C_hornet_SAI_SLIP_BALL, onSaiSlipBallChange);

void onSaiBankChange(unsigned int v) {
  sai.bank = v;
}
DcsBios::IntegerBuffer saiBankBuffer(FA_18C_hornet_SAI_BANK, onSaiBankChange);

void onSaiRateOfTurnChange(unsigned int v) {
  sai.rateOfTurn = v;
}
DcsBios::IntegerBuffer saiRateOfTurnBuffer(FA_18C_hornet_SAI_RATE_OF_TURN, onSaiRateOfTurnChange);

void onSaiManPitchAdjChange(unsigned int v) {
  sai.manPitchAdj = v;
}
DcsBios::IntegerBuffer saiManPitchAdjBuffer(FA_18C_hornet_SAI_MAN_PITCH_ADJ, onSaiManPitchAdjChange);

void onSaiPitchChange(unsigned int v) {
  sai.pitch = v;
}
DcsBios::IntegerBuffer saiPitchBuffer(FA_18C_hornet_SAI_PITCH, onSaiPitchChange);

void onSaiAttWarningFlagChange(unsigned int v) {
  sai.attWarningFlag = v;
}
DcsBios::IntegerBuffer saiAttWarningFlagBuffer(FA_18C_hornet_SAI_ATT_WARNING_FLAG, onSaiAttWarningFlagChange);

void onSaiPointerHorChange(unsigned int v) {
  sai.pointerHor = v;
}
DcsBios::IntegerBuffer saiPointerHorBuffer(FA_18C_hornet_SAI_POINTER_HOR, onSaiPointerHorChange);

void onSaiPointerVerChange(unsigned int v) {
  sai.pointerVer = v;
}
DcsBios::IntegerBuffer saiPointerVerBuffer(FA_18C_hornet_SAI_POINTER_VER, onSaiPointerVerChange);

#pragma endregion SARI

#pragma region Helpers
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
#pragma endregion Helpers
