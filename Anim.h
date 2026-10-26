// ============================================================================
//  Anim.h  -  "The animation toolbox"
//
//  WHAT THIS FILE IS FOR:
//  It gives us a few reusable tools for making smooth animation on the CYD.
//  Nothing in here knows what a pumpkin is. Any design (pumpkin, skull,
//  robot, cat...) can be built with these same tools.
//
//  THE BIG IDEA - WHY NOT JUST DRAW ON THE SCREEN?
//  If we erase a shape on the real screen and redraw it in a new spot, for a
//  split second the viewer sees the empty background. That shows up as
//  FLICKER. Instead we draw each frame on an invisible "canvas" in memory
//  (TFT_eSPI calls this a SPRITE), and when the picture is finished we copy
//  the whole canvas to the screen in one go. The viewer only ever sees
//  finished pictures. This trick is called DOUBLE BUFFERING.
//
//  WHY NOT ONE CANVAS FOR THE WHOLE SCREEN?
//  A full 240x320 canvas needs 240*320*2 = 153,600 bytes of memory, and the
//  ESP32 can't give us one block that big. So we only make canvases for the
//  parts that MOVE (an eye, a mouth) and draw everything else once.
//  Each moving part gets its own LAYER: a rectangle of the screen plus the
//  canvas that is drawn into it.
// ============================================================================

#ifndef ANIM_H
#define ANIM_H

#include <Arduino.h>
#include <TFT_eSPI.h>

// ----------------------------------------------------------------------------
//  LAYER - one rectangle of the screen with its own off-screen canvas.
//
//  How to use one:
//     Layer eye(&tft);                 // make it (tell it which screen)
//     eye.begin(10, 40, 100, 100);     // x, y, width, height on the screen
//     eye.canvas.fillSprite(TFT_BLACK);// draw on the canvas...
//     eye.canvas.fillCircle(50, 50, 20, TFT_ORANGE);
//     eye.show();                      // ...then copy it to the screen
//
//  Coordinates you draw with are INSIDE the layer: (0,0) is the layer's own
//  top-left corner, not the screen's.
// ----------------------------------------------------------------------------
class Layer {
public:
  int x = 0, y = 0;        // where the layer sits on the screen
  int w = 0, h = 0;        // its size in pixels
  TFT_eSprite canvas;      // the invisible picture we draw on

  // The canvas needs to know which screen it will eventually be copied to.
  Layer(TFT_eSPI *screen) : canvas(screen) {}

  // Reserve memory for the canvas. Returns true if it worked.
  bool begin(int x_, int y_, int w_, int h_) {
    x = x_;  y = y_;  w = w_;  h = h_;

    // First try full color (16 bits = 65,536 colors, 2 bytes per pixel).
    canvas.setColorDepth(16);
    if (canvas.createSprite(w, h) != nullptr) return true;

    // Not enough memory? Try 8-bit color (256 colors, half the memory).
    // Colors look slightly less smooth but the animation still works.
    canvas.setColorDepth(8);
    if (canvas.createSprite(w, h) != nullptr) {
      Serial.printf("Layer %dx%d: low memory, using 8-bit color\n", w, h);
      return true;
    }

    Serial.printf("Layer %dx%d: NOT ENOUGH MEMORY - make it smaller\n", w, h);
    return false;
  }

  // Copy the finished canvas onto the real screen.
  void show() {
    if (canvas.created()) canvas.pushSprite(x, y);
  }

  // Handy: the middle of the layer, in the layer's own coordinates.
  int cx() const { return w / 2; }
  int cy() const { return h / 2; }
};

// ----------------------------------------------------------------------------
//  EVERY - a simple timer that says "yes" once every N milliseconds.
//
//     Every frameTimer(33);            // about 30 times per second
//     if (frameTimer.ready()) { ... }  // true once every 33 ms
//
//  We use millis() (milliseconds since power-on) instead of delay(), because
//  delay() freezes the whole program; with a timer, other things keep going.
// ----------------------------------------------------------------------------
class Every {
public:
  uint32_t period;          // how many ms between "yes" answers
  uint32_t last = 0;        // when we last said "yes"

  Every(uint32_t ms) : period(ms) {}

  bool ready() {
    uint32_t now = millis();
    if (now - last >= period) {   // subtraction works even when millis wraps
      last = now;
      return true;
    }
    return false;
  }
};

// ----------------------------------------------------------------------------
//  SMOOTH MOTION HELPERS
//  Real things don't teleport. These functions move a value a little bit
//  toward where it wants to be on every frame, which looks natural.
// ----------------------------------------------------------------------------

// Move 'current' toward 'target'. 'speed' = how snappy (try 2 to 30).
// 'dt' = seconds since the last frame, so the speed is the same whether
// the program runs at 20 or 60 frames per second.
inline float approach(float current, float target, float speed, float dt) {
  float fraction = 1.0f - expf(-speed * dt);  // 0..1, bigger when dt is bigger
  return current + (target - current) * fraction;
}

// Blend between a and b. t = 0 gives a, t = 1 gives b, t = 0.5 halfway.
// (Mathematicians call this "linear interpolation". We don't name it "lerp"
// because newer C++ already has a std::lerp, and two functions with the same
// name confuse the compiler.)
inline float blend(float a, float b, float t) {
  return a + (b - a) * t;
}

// Smooth start AND smooth stop: input 0..1, output 0..1, gentle at both ends.
inline float easeInOut(float t) {
  t = constrain(t, 0.0f, 1.0f);
  return t * t * (3.0f - 2.0f * t);
}

// A random decimal number between lo and hi.
inline float randomFloat(float lo, float hi) {
  return lo + (hi - lo) * (random(10001) / 10000.0f);
}

// ----------------------------------------------------------------------------
//  COLOR HELPERS
//  The screen stores colors as RGB565: 5 bits red, 6 bits green, 5 bits blue
//  packed into one 16-bit number.
// ----------------------------------------------------------------------------

// Build an RGB565 color from normal 0-255 red, green, blue values.
inline uint16_t rgb(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

// Mix two colors. t = 0 gives colorA, t = 1 gives colorB.
// Great for glowing / fading effects.
inline uint16_t blendColor(uint16_t colorA, uint16_t colorB, float t) {
  t = constrain(t, 0.0f, 1.0f);
  // Unpack each color into its red, green and blue parts...
  int rA = (colorA >> 11) & 0x1F, gA = (colorA >> 5) & 0x3F, bA = colorA & 0x1F;
  int rB = (colorB >> 11) & 0x1F, gB = (colorB >> 5) & 0x3F, bB = colorB & 0x1F;
  // ...mix each part separately...
  int r = rA + (int)((rB - rA) * t);
  int g = gA + (int)((gB - gA) * t);
  int b = bA + (int)((bB - bA) * t);
  // ...and pack them back together.
  return (r << 11) | (g << 5) | b;
}

// ----------------------------------------------------------------------------
//  PICTURES (made with the img2cyd.py converter)
//
//  If students DRAW their design (in any paint program) instead of building
//  it from circles and triangles, img2cyd.py turns the PNG or GIF into a .h
//  file holding an Image. One Image can hold several FRAMES (like a flip
//  book), e.g. a mouth in 4 positions from closed to wide open.
// ----------------------------------------------------------------------------
struct Image {
  uint16_t width;          // size of ONE frame
  uint16_t height;
  uint16_t frameCount;     // how many frames are stacked in 'pixels'
  bool     hasTransparency;// true = skip pixels that match 'transparentColor'
  uint16_t transparentColor;
  const uint16_t *pixels;  // all frames, one after another, row by row
};

// Draw one frame of an Image onto a layer's canvas with its top-left at (x,y).
// Transparent pixels are skipped, so the background shows through.
inline void drawImage(TFT_eSprite &canvas, int x, int y,
                      const Image &img, int frame = 0) {
  frame = constrain(frame, 0, img.frameCount - 1);
  const uint16_t *p = img.pixels + (uint32_t)frame * img.width * img.height;

  for (int row = 0; row < img.height; row++) {
    for (int col = 0; col < img.width; col++) {
      uint16_t color = pgm_read_word(p++);   // read one pixel from flash
      if (img.hasTransparency && color == img.transparentColor) continue;
      canvas.drawPixel(x + col, y + row, color);
    }
  }
}

// Same thing, but centered on (cx, cy) instead of using the top-left corner.
inline void drawImageCentered(TFT_eSprite &canvas, int cx, int cy,
                              const Image &img, int frame = 0) {
  drawImage(canvas, cx - img.width / 2, cy - img.height / 2, img, frame);
}

#endif // ANIM_H
