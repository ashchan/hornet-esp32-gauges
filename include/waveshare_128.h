#define USER_SETUP_ID 401

#define GC9A01_DRIVER

#define TFT_WIDTH  240
#define TFT_HEIGHT 240

// Correct pins for Waveshare ESP32-S3-LCD-1.28 (2023–2025 version)
#define TFT_MOSI 11
#define TFT_SCLK 10
#define TFT_CS    9
#define TFT_DC    8
#define TFT_RST  12

#define TFT_BL   40                  // Backlight pin
#define TFT_BACKLIGHT_ON HIGH        // HIGH = backlight on

#define TFT_MISO -1                  // Not used

#define SPI_FREQUENCY  27000000     // ← this is the magic line
#define SPI_READ_FREQUENCY  20000000

#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_GFXFF
#define SMOOTH_FONT

#define USE_HSPI_PORT   // maps TFT_eSPI to SPI2_HOST on ESP32-S3