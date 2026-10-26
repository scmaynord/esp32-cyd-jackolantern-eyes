// ============================================================================
//  User_Setup.h  -  Settings for the TFT_eSPI screen library
//
//  TFT_eSPI is the library (pre-written code) that draws text and shapes on
//  the screen. It works with hundreds of different screens, so it needs to
//  be told WHICH screen we have and HOW it is wired. That is this file.
//
//  !!! IMPORTANT - THIS FILE DOES NOTHING IF IT STAYS IN YOUR SKETCH FOLDER !!!
//  The library only looks inside its OWN folder. To use it:
//    1. Find   Documents/Arduino/libraries/TFT_eSPI/
//    2. Rename the User_Setup.h already there to User_Setup_ORIGINAL.h (backup)
//    3. Copy THIS file into that folder.
//    4. Repeat after updating the TFT_eSPI library (updates overwrite it).
//  Symptom if you forget: the screen stays white or black after uploading.
// ============================================================================

// Include guard: stops this file from being read twice.
#ifndef USER_SETUP_H
#define USER_SETUP_H

// A label the library can print to say which setup file it is using.
#define USER_SETUP_INFO "CYD_ESP32-2432S028R"

// ----------------------------------------------------------------------------
//  SCREEN DRIVER
//  The "driver" is the chip inside the screen that controls the pixels.
//  Most CYDs use an ILI9341. Some newer boards (two USB ports) use an ST7789.
//  If colors look wrong or the picture is scrambled: put // in front of the
//  ILI9341 line and remove the // in front of the ST7789 line.
// ----------------------------------------------------------------------------
#define ILI9341_2_DRIVER
// #define ST7789_DRIVER

// Size of the screen in pixels (when held upright).
#define TFT_WIDTH  240
#define TFT_HEIGHT 320

// ----------------------------------------------------------------------------
//  BACKLIGHT
// ----------------------------------------------------------------------------
#define TFT_BL   21               // the backlight is on pin 21
#define TFT_BACKLIGHT_ON HIGH     // sending HIGH to that pin turns the light on

// ----------------------------------------------------------------------------
//  SCREEN WIRING  (same numbers as in CYD_Pins.h - see there for what each means)
// ----------------------------------------------------------------------------
#define TFT_MISO 12   // data from the screen
#define TFT_MOSI 13   // data to the screen
#define TFT_SCLK 14   // clock
#define TFT_CS   15   // chip select
#define TFT_DC    2   // data/command
#define TFT_RST  -1   // reset (-1 = not connected)

// The ESP32 has two SPI "lanes". The screen is wired to the one called HSPI.
#define USE_HSPI_PORT

// ----------------------------------------------------------------------------
//  TOUCH
//  TFT_eSPI has its own touch code, but it expects the touch chip to share
//  the screen's wires. On the CYD it does NOT, so we leave this switched off
//  (the // makes it a comment) and use the XPT2046_Touchscreen library instead.
//  You may see a warning about TOUCH_CS when compiling. It is safe to ignore.
// ----------------------------------------------------------------------------
// #define TOUCH_CS 33

// ----------------------------------------------------------------------------
//  FONTS
//  Each line loads a font into the program. In the code, the LAST number in
//  tft.drawString("Hi", x, y, 2) picks which font to use.
//  Remove fonts you do not use to make your program smaller.
// ----------------------------------------------------------------------------
#define LOAD_GLCD    // Font 1: tiny 6x8 pixel letters
#define LOAD_FONT2   // Font 2: small, 16 pixels tall (used in the template)
#define LOAD_FONT4   // Font 4: medium, 26 pixels tall
#define LOAD_FONT6   // Font 6: large numbers only, 48 pixels tall
#define LOAD_FONT7   // Font 7: digital-clock style numbers
#define LOAD_FONT8   // Font 8: very large numbers only, 75 pixels tall
#define LOAD_GFXFF   // FreeFonts: extra fonts like FreeSans, FreeSerif
#define SMOOTH_FONT  // allows smooth (anti-aliased) fonts loaded from files

// ----------------------------------------------------------------------------
//  SPEED
//  How fast data travels on the wires, in Hertz (ticks per second).
//  55000000 = 55 million per second. If the screen shows glitches or random
//  colored dots, try lowering SPI_FREQUENCY to 40000000.
// ----------------------------------------------------------------------------
#define SPI_FREQUENCY       55000000   // speed when drawing to the screen
#define SPI_READ_FREQUENCY  20000000   // speed when reading from the screen
#define SPI_TOUCH_FREQUENCY  2500000   // speed for touch (not used here)

#endif // USER_SETUP_H