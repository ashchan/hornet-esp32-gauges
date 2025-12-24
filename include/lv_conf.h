#pragma once
#ifndef LV_CONF_H
#define LV_CONF_H

/* ============================================================
 * Basic configuration
 * ============================================================ */

/* Use 16-bit color (RGB565) — best for ESP32 LCDs */
#define LV_COLOR_DEPTH 16

/* Many ESP32 LCD drivers expect swapped color bytes (RGB565 endian swap).
 * If your colors look wrong (blue/red swapped or weird), toggle this.
 */
#define LV_COLOR_16_SWAP 0

/* Default refresh period */
#define LV_DISP_DEF_REFR_PERIOD 30

/* ============================================================
 * Memory
 * ============================================================ */

/* Use LVGL's built-in allocator.
 * If you want LVGL to allocate from PSRAM, you can override LV_MEM_CUSTOM.
 */
#define LV_MEM_CUSTOM 0
#define LV_MEM_SIZE (64U * 1024U)   /* 64 KB internal heap for LVGL objects */

#define LV_USE_LOG 1
#define LV_LOG_LEVEL LV_LOG_LEVEL_WARN

/* ============================================================
 * Fonts
 * ============================================================ */

#define LV_FONT_MONTSERRAT_14  1
#define LV_FONT_MONTSERRAT_48  1
#define LV_FONT_DEFAULT &lv_font_montserrat_14

/* Disable other fonts to keep flash small */
#define LV_FONT_MONTSERRAT_8   0
#define LV_FONT_MONTSERRAT_10  0
#define LV_FONT_MONTSERRAT_12  0
#define LV_FONT_MONTSERRAT_16  0
#define LV_FONT_MONTSERRAT_18  0
#define LV_FONT_MONTSERRAT_20  0
#define LV_FONT_MONTSERRAT_22  0
#define LV_FONT_MONTSERRAT_24  0
#define LV_FONT_MONTSERRAT_26  0
#define LV_FONT_MONTSERRAT_28  0
#define LV_FONT_MONTSERRAT_30  0
#define LV_FONT_MONTSERRAT_32  0
#define LV_FONT_MONTSERRAT_34  0
#define LV_FONT_MONTSERRAT_36  0
#define LV_FONT_MONTSERRAT_38  0
#define LV_FONT_MONTSERRAT_40  0
#define LV_FONT_MONTSERRAT_42  0
#define LV_FONT_MONTSERRAT_44  0
#define LV_FONT_MONTSERRAT_46  0

/* ============================================================
 * Core features
 * ============================================================ */

#define LV_USE_DRAW_SW 1
#define LV_USE_OS 0

/* Enable the widgets you’re likely using */
#define LV_USE_LABEL 1
#define LV_USE_ARC 1
#define LV_USE_LINE 1
#define LV_USE_IMG 1
#define LV_USE_BTN 1
#define LV_USE_OBJ 1
#define LV_USE_BAR 1
#define LV_USE_SLIDER 1
#define LV_USE_SWITCH 1

/* Canvas is optional but useful — you used it earlier */
#define LV_USE_CANVAS 1

/* Optional: enable image decoder (PNG/JPG etc.) if you need it */
#define LV_USE_IMG_DECODER 1
#define LV_IMG_CACHE_DEF_SIZE 1

/* ============================================================
 * Input devices (touch etc.)
 * ============================================================ */
#define LV_USE_INDEV 1

/* ============================================================
 * Performance helpers
 * ============================================================ */
#define LV_USE_CACHE 1

#endif /*LV_CONF_H*/