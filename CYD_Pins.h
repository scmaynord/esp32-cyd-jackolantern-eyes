// ============================================================================
//  CYD_Pins.h  -  "Where is everything wired?"
//
//  Board: ESP32-2432S028R, nicknamed the "Cheap Yellow Display" (CYD).
//  It is an ESP32 microcontroller with a 2.8 inch touchscreen bolted on.
//
//  WHAT THIS FILE IS FOR:
//  Every part on the board (screen, touch sensor, LED, speaker...) is wired
//  to a numbered pin on the ESP32 chip, called a GPIO ("General Purpose
//  Input/Output"). This file gives each of those numbers a readable NAME,
//  so in our code we can write CYD_LED_R instead of remembering "4".
//
//  You should NOT need to change this file. These wires are soldered onto
//  the circuit board and are the same on every CYD.
// ============================================================================

// "Include guard": if this file gets #included twice, the second copy is
// skipped. Without it, the compiler would complain that every name below
// was defined two times.
#ifndef CYD_PINS_H     // "if CYD_PINS_H has NOT been defined yet..."
#define CYD_PINS_H     // "...define it now, and read the rest of the file"

// HOW #define WORKS:
//   #define NAME value
// Before compiling, the computer does a find-and-replace: everywhere it sees
// NAME in our code, it swaps in the value. No memory is used; it is just a
// nickname for a number.

// ----------------------------------------------------------------------------
//  SCREEN SIZE
//  The screen is a grid of tiny dots called pixels.
//  Held upright (portrait, USB port at the bottom) it is 240 wide x 320 tall.
//  Position (0,0) is the TOP-LEFT corner. X grows to the right, Y grows DOWN.
// ----------------------------------------------------------------------------
#define CYD_PANEL_WIDTH   240   // pixels across
#define CYD_PANEL_HEIGHT  320   // pixels top to bottom

// ----------------------------------------------------------------------------
//  DISPLAY PINS
//  The ESP32 talks to the screen using "SPI", a fast way of sending data
//  over a few wires. The ESP32 has two SPI "lanes"; the screen uses the one
//  called HSPI. (These same numbers are also in User_Setup.h, which is where
//  the screen library actually reads them from.)
// ----------------------------------------------------------------------------
#define CYD_TFT_MISO  12   // Master In,  Slave Out: data FROM screen TO ESP32
#define CYD_TFT_MOSI  13   // Master Out, Slave In:  data FROM ESP32 TO screen
#define CYD_TFT_SCLK  14   // Serial Clock: a ticking signal that keeps both sides in step
#define CYD_TFT_CS    15   // Chip Select: "hey screen, I'm talking to YOU now"
#define CYD_TFT_DC     2   // Data/Command: tells the screen if bytes are pixels or instructions
#define CYD_TFT_RST   -1   // Reset pin. -1 means "not connected" (screen resets with the board)
#define CYD_TFT_BL    21   // BackLight: the light behind the screen. HIGH = on, LOW = off

// ----------------------------------------------------------------------------
//  TOUCH SENSOR PINS  (chip name: XPT2046)
//  The touch sensor is a SEPARATE chip on its OWN set of wires, so it gets
//  its own SPI connection (we use the ESP32's other lane, called VSPI).
// ----------------------------------------------------------------------------
#define CYD_TOUCH_IRQ   36   // Interrupt ReQuest: goes LOW the moment a finger presses
#define CYD_TOUCH_MOSI  32   // data from ESP32 to touch chip
#define CYD_TOUCH_MISO  39   // data from touch chip to ESP32
#define CYD_TOUCH_CLK   25   // clock signal for the touch chip
#define CYD_TOUCH_CS    33   // chip select for the touch chip

// ----------------------------------------------------------------------------
//  TOUCH CALIBRATION
//  The touch chip does NOT report pixels. It reports "raw" numbers from about
//  0 to 4095. These four values say which raw numbers line up with the EDGES
//  of the screen, so we can convert raw numbers into pixel positions.
//
//  Every screen is a little different. If taps land in the wrong spot:
//    1. In Config.h set CALIBRATE_TOUCH to 1 and upload.
//    2. Open Tools > Serial Monitor (115200 baud).
//    3. Tap each corner and write down the smallest/largest x and y printed.
//    4. Put those numbers here.
// ----------------------------------------------------------------------------
#define CYD_TOUCH_MIN_X  200    // raw X at the left edge
#define CYD_TOUCH_MAX_X  3800   // raw X at the right edge
#define CYD_TOUCH_MIN_Y  200    // raw Y at the top edge
#define CYD_TOUCH_MAX_Y  3800   // raw Y at the bottom edge

// ----------------------------------------------------------------------------
//  MICRO SD CARD SLOT  (on the back of the board)
//  Useful for storing pictures or sound files. Not used in this template.
// ----------------------------------------------------------------------------
#define CYD_SD_CS    5    // chip select for the SD card
#define CYD_SD_MOSI 23    // data to the card
#define CYD_SD_MISO 19    // data from the card
#define CYD_SD_SCK  18    // clock for the card

// ----------------------------------------------------------------------------
//  RGB LED  (small light on the back: Red, Green, Blue)
//  It is wired "backwards" (common anode):
//    LOW  = light ON
//    HIGH = light OFF
//  Turning on two colors at once mixes them (red + green = yellow).
// ----------------------------------------------------------------------------
#define CYD_LED_R  4    // red part of the LED
#define CYD_LED_G 16    // green part of the LED
#define CYD_LED_B 17    // blue part of the LED

// ----------------------------------------------------------------------------
//  LIGHT SENSOR  (LDR = Light Dependent Resistor, on the front near the screen)
//  Read it with analogRead() to get a number from 0 to 4095.
//  Watch out: it is backwards too. BRIGHT room = SMALL number, DARK = BIG number.
// ----------------------------------------------------------------------------
#define CYD_LDR   34

// ----------------------------------------------------------------------------
//  SPEAKER CONNECTOR  (small 2-pin plug labelled SPEAK)
//  Plug in a small speaker. A tiny amplifier on the board makes it louder.
// ----------------------------------------------------------------------------
#define CYD_SPEAKER 26

// ----------------------------------------------------------------------------
//  BOOT BUTTON  (the button labelled BOOT)
//  Reads LOW while pressed. You can use it as an extra button in your program.
// ----------------------------------------------------------------------------
#define CYD_BOOT_BTN 0

// ----------------------------------------------------------------------------
//  SPARE PINS for your own wires (sensors, servos, extra LEDs...)
//  They come out on the small connectors on the edge of the board:
//    P3 connector:  GND, IO35, IO22, IO21
//    CN1 connector: GND, IO22, IO27, 3.3V
//  Note: IO21 is the backlight, so do not use it for anything else.
// ----------------------------------------------------------------------------
#define CYD_GPIO_35  35   // INPUT ONLY (can read a sensor, cannot power an LED)
#define CYD_GPIO_22  22   // input or output
#define CYD_GPIO_27  27   // input or output

#endif // CYD_PINS_H   (end of the include guard that started at the top)