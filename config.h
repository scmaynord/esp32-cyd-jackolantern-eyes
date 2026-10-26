// ============================================================================
//  Config.h  -  "Settings for YOUR project"
//
//  This is the file you are EXPECTED to edit. Change the Wi-Fi name, colors,
//  screen direction, etc. here instead of hunting through the main sketch.
//  (The hardware wiring lives in CYD_Pins.h and should not change.)
// ============================================================================

// Include guard: stops this file from being read twice (see CYD_Pins.h).
#ifndef CONFIG_H
#define CONFIG_H

// ----------------------------------------------------------------------------
//  DISPLAY SETTINGS
// ----------------------------------------------------------------------------

// Which way the picture faces. Rotate the board to match:
//   0 = portrait  (tall),  USB port at the bottom
//   1 = landscape (wide),  USB port on the right
//   2 = portrait,          upside down
//   3 = landscape,         USB port on the left
#define DISPLAY_ROTATION  0

// Screen brightness: 0 = off (black), 255 = full brightness.
#define BACKLIGHT_LEVEL   255

// ----------------------------------------------------------------------------
//  COLORS
//  The screen uses "RGB565" colors: one 16-bit number holds Red, Green and
//  Blue. We write them in hexadecimal (base 16), which is what "0x" means.
//    0x0000 = every bit off = BLACK
//    0xFFFF = every bit on  = WHITE
//  Other handy ones if you want to experiment later:
//    0xF800 = red    0x07E0 = green    0x001F = blue    0xFFE0 = yellow
// ----------------------------------------------------------------------------
//#define COLOR_BG      0x0000   // BackGround color: black
//#define COLOR_TEXT    0xFFFF   // text and outline color: white

// ----------------------------------------------------------------------------
//  WI-FI SETTINGS
//  The board only works with 2.4 GHz Wi-Fi networks (not 5 GHz).
// ----------------------------------------------------------------------------

// 1 = try to connect to Wi-Fi when the board starts
// 0 = skip Wi-Fi completely (faster startup, fine for offline projects)
#define USE_WIFI            1

// The network name and password, inside quotation marks.
// Do not share sketches with your real password in them!
#define WIFI_SSID           "your-ssid"         // SSID = the Wi-Fi network's name
#define WIFI_PASSPHRASE     "your-passphrase"   // the Wi-Fi password

// How long to keep trying before giving up, in milliseconds.
// 1000 milliseconds = 1 second, so 10000 = 10 seconds.
#define WIFI_TIMEOUT_MS     10000

// ----------------------------------------------------------------------------
//  TOUCH SETTINGS
// ----------------------------------------------------------------------------

// 1 = print the touch chip's raw numbers to the Serial Monitor on every tap.
//     Use this when calibrating (see CYD_Pins.h). Set back to 0 when done.
#define CALIBRATE_TOUCH     0

// "Debounce": one finger tap can look like many quick touches to the chip.
// We ignore any new touch that comes sooner than this many milliseconds
// after the last one, so one tap = one action.
#define TOUCH_DEBOUNCE_MS   150

#endif // CONFIG_H