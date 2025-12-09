#ifndef HELPER_H
#define HELPER_H

#include <cctype>
#include <cstring>
#include "display_driver.h"

//Variables

//Clock
String TC_H = "00";  //TOP clock hours
String TC_Dd1 = ":"; //TOP clock colon 1 
String TC_M = "00";  //TOP clock minutes
String TC_Dd2 = ":"; //TOP clock colon 2
String TC_S = "00";  //TOP clock seconds

String LC_H = "0 ";   //Lower clock hours
String LC_Dd1 = ":"; //Lower clock colon 1
String LC_M = "00";  //Lower clock minutes
String LC_Dd2 = ":"; //Lower clock colon 2
String LC_S = "00";  //Lower clock seconds

// IFEI COLOR MODE

unsigned int ifei_color = 0xFFFFFFU;
const unsigned int color_day = 0xFFFFFFU;
const unsigned int color_NIGHT = 0x1CDD2AU;

// NOZL Gauge elements visibility
bool NOZL_pointer_visible = true;
bool NOZR_pointer_visible = true;
bool NOZL_scale_visible = true;
bool NOZR_scale_visible = true;
bool NOZL_numbers_visible = true;
bool NOZR_numbers_visible = true;

// lighting test switch active
bool test_switch_enabled = false;


//Aircraft 
bool ishornet = false;
// demo variables
unsigned long nozzle_update = 0;
bool demo_forward = true;
bool reset = false;
int demo_counter = 0;

// Create tft screen 
LGFX tft;

// Create sprites 

LGFX_Sprite TWOD(&tft);   // Data digits sprite (RPM/OIL)
LGFX_Sprite THREED(&tft); // Data digits sprite (TEMP/FF)
LGFX_Sprite LABELS(&tft); // Text label sprite 
LGFX_Sprite ffT(&tft);    // Special text sprite label for fuel flow
LGFX_Sprite Fuel(&tft);   //Fuel sprite
LGFX_Sprite CLOCK(&tft);  //Clock sprite 

LGFX_Sprite NOZL_IMAGE[27] //Left nozzel gauge sprites
{ LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft)
  };
  
LGFX_Sprite NOZR_IMAGE[27] //Right nozzel gauge sprites
{
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft),
  LGFX_Sprite(&tft)
};

LGFX_Sprite TAG(&tft); //Special Tags sprite (L/R/Z)

// Create data structure for display elements
struct display_element{
  int sprite_width;
  int sprite_hight;
  int pos_x;
  int pos_y;
  int textalign;
  LGFX_Sprite* sprite;
  char value[8];
};

// Enumeration of display elements
enum Display_Name{
  RPML,
  RPMR,
  RPMT,
  TMPL,
  TMPR,
  TMPT,
  FFL,
  FFR,
  FFTU,
  FFTL,
  OILL,
  OILR,
  OILT,
  NOZL,
  NOZR,
  NOZT,
  FUELU,
  FUELL,
  BINGO,
  BINGOT,
  CLOCKU,
  CLOCKL,
  ZULU,
  L,
  R
};

// Functions
bool copy_and_trim_spaces(const char* src, char* dest) {
  size_t outSize = 8;
  if (!dest) return false;
  dest[0] = '\0';
  if (!src) return false;

  while (*src && std::isspace((unsigned char)*src)) src++;
  if (!*src) return false;

  const char* end = src + std::strlen(src);
  while (end > src && std::isspace((unsigned char)end[-1])) end--;

  size_t n = (size_t)(end - src);
  if (n >= outSize) n = outSize - 1;

  std::memcpy(dest, src, n);
  dest[n] = '\0';
  return true;
}

#endif
