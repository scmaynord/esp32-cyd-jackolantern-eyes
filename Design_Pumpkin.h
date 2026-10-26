// ============================================================================
//  Design_Pumpkin.h  -  "The painter" (example design: classic jack-o'-lantern)
//
//  WHAT THIS FILE IS FOR:
//  Every frame it reads the Character's numbers (blink, lookX, mouthOpen,
//  glow, mood...) and paints a matching picture. Everything about how the
//  face LOOKS lives in this one file.
//
//  TO MAKE A DIFFERENT DESIGN:
//  Copy this file (e.g. Design_Skull.h), change the paint functions, and
//  change the #include line in CYD_Animate.ino. Keep the two functions at
//  the bottom - designBegin() and designDraw() - because the main sketch
//  calls them.
//
//  ONE SCREEN OR SEVERAL?
//  FACE_PART (set at the top of CYD_Animate.ino) picks what THIS board shows:
//     PART_FULL_FACE  - both eyes, nose and mouth on one screen
//     PART_LEFT_EYE   - one big eye   (put one CYD behind each eye hole);
//                       this board is the LEADER (see EyeLink.h)
//     PART_RIGHT_EYE  - one big eye, mirrored; FOLLOWS the left eye
//     PART_MOUTH      - one big mouth (screen turned sideways)
//  Every board runs the same code; only that one setting changes.
// ============================================================================

#ifndef DESIGN_PUMPKIN_H
#define DESIGN_PUMPKIN_H

#include "Anim.h"
#include "Character.h"

// ----------------------------------------------------------------------------
//  COLORS for this design. rgb(red, green, blue), each 0-255.
//  Try changing these! A green or purple flame is an easy way to stand out.
// ----------------------------------------------------------------------------
const uint16_t PUMPKIN_BG    = rgb(0,   0,   0);    // inside of the pumpkin
const uint16_t PUMPKIN_EMBER = rgb(150, 35,  0);    // dimmest flame color
const uint16_t PUMPKIN_FLAME = rgb(255, 120, 0);    // normal flame color
const uint16_t PUMPKIN_HOT   = rgb(255, 220, 90);   // hottest, brightest center
const uint16_t PUMPKIN_PUPIL = rgb(25,  5,   0);    // dark pupil

// The flame color right now, based on how bright the glow is (0..1).
inline uint16_t flameColor(float glow) {
  return blendColor(PUMPKIN_EMBER, PUMPKIN_FLAME, glow);
}

// ============================================================================
//  PAINT FUNCTIONS - each one draws a single face part onto a canvas,
//  sized to fit whatever canvas it is given (so the same code draws a
//  small eye on the full face or a giant eye on its own screen).
// ============================================================================

// ---- EYE -------------------------------------------------------------------
// isLeft tells us which side the nose is on, so moods can tilt toward it.
void paintEye(TFT_eSprite &canvas, int w, int h, const Character &c,
              bool isLeft) {
  canvas.fillSprite(PUMPKIN_BG);

  float s  = min(w, h) * 0.46f;          // "size": half the eye's width
  int   cx = w / 2;                      // center of the canvas
  int   cy = h / 2 + (int)(s * 0.1f);

  // The eye is a triangle. Corner points:
  int baseY  = cy + (int)(s * 0.55f);    // bottom edge
  int apexY  = cy - (int)(s * 0.75f);    // top point
  int leftX  = cx - (int)s;
  int rightX = cx + (int)s;
  int inward = isLeft ? 1 : -1;          // +1 = nose is to the right

  // 1. Outer flame triangle.
  canvas.fillTriangle(leftX, baseY, rightX, baseY, cx, apexY,
                      flameColor(c.glow));

  // 2. Smaller, hotter triangle inside it - makes it look lit from within.
  //    It flares white-hot when the character speaks (mouthOpen = loudness).
  int inset = (int)(s * 0.35f);
  float heat = max(c.glow, c.mouthOpen);
  canvas.fillTriangle(leftX + inset, baseY - inset / 3,
                      rightX - inset, baseY - inset / 3,
                      cx, apexY + inset,
                      blendColor(PUMPKIN_FLAME, PUMPKIN_HOT, heat));

  // 3. Pupil: a tall slit that slides around with lookX / lookY, and
  //    widens with every syllable while the character is speaking.
  int px = cx + (int)(c.lookX * s * 0.40f);
  int py = cy + (int)(s * 0.05f) + (int)(c.lookY * s * 0.20f);
  float slit = (c.mood == MOOD_ANGRY ? 0.08f : 0.13f) + c.mouthOpen * 0.12f;
  canvas.fillEllipse(px, py, (int)(s * slit),
                     (int)(s * (0.30f + c.mouthOpen * 0.06f)), PUMPKIN_PUPIL);

  // 4. Mood: we "carve away" part of the eye by painting background over it.
  if (c.mood == MOOD_ANGRY) {
    // A wedge across the top that slopes down toward the nose = a frown.
    int far  = cx - inward * (int)(s * 1.3f);
    int near = cx + inward * (int)(s * 1.3f);
    canvas.fillTriangle(far, apexY - 5, near, apexY - 5,
                        near, cy + (int)(s * 0.05f), PUMPKIN_BG);
  } else if (c.mood == MOOD_HAPPY) {
    // Scoop the bottom out with a circle: the eye becomes a "^" arch.
    canvas.fillCircle(cx, baseY + (int)(s * 0.55f), (int)(s * 0.85f),
                      PUMPKIN_BG);
  }

  // 5. Eyelid: when blinking, cover the eye from the top down.
  if (c.blink > 0.02f) {
    int lidY = apexY + (int)((baseY - apexY + 2) * c.blink);
    canvas.fillRect(0, 0, w, lidY, PUMPKIN_BG);

    // A thin glowing edge along the bottom of the lid, only as wide as
    // the triangle is at that height (the triangle widens toward the bottom).
    if (lidY < baseY) {
      float frac = (lidY - apexY) / (float)(baseY - apexY);
      int half = (int)(s * frac);
      canvas.fillRect(cx - half, lidY, half * 2, 2, PUMPKIN_EMBER);
    }
  }
}

// ---- NOSE ------------------------------------------------------------------
void paintNose(TFT_eSprite &canvas, int w, int h, const Character &c) {
  canvas.fillSprite(PUMPKIN_BG);
  canvas.fillTriangle(w / 2, 2, 4, h - 3, w - 5, h - 3, flameColor(c.glow));
}

// ---- MOUTH -----------------------------------------------------------------
// We paint the mouth one vertical stripe at a time, left to right. For each
// stripe we work out where the top and bottom lip are. Doing it this way
// makes curved, stretchy shapes easy.
void paintMouth(TFT_eSprite &canvas, int w, int h, const Character &c) {
  canvas.fillSprite(PUMPKIN_BG);

  float mouthW = w * 0.92f;
  int   left   = (int)((w - mouthW) / 2);
  float cx     = w / 2.0f;

  // How much the mouth curves: + is a smile (U shape), - is a frown.
  float smile = 0.5f;
  if (c.mood == MOOD_HAPPY)  smile = 1.0f;
  if (c.mood == MOOD_ANGRY)  smile = -0.35f;
  if (c.mood == MOOD_SLEEPY) smile = 0.15f;
  float bend = smile * h * 0.22f;

  // How thick the mouth is in the middle: thin slit when closed,
  // tall when open.
  float thick = blend(h * 0.14f, h * 0.62f, c.mouthOpen);

  // Middle line of the mouth at horizontal position u (-1 left .. +1 right).
  // We remember these so the teeth can find the lips afterward.
  auto curveAt = [&](float u) { return 1.0f - u * u; };  // 0 at ends, 1 middle
  auto midAt   = [&](float u) {
    return h * 0.42f - bend * 0.5f + bend * curveAt(u);
  };
  auto thickAt = [&](float u) { return thick * (0.25f + 0.75f * curveAt(u)); };

  for (int x = left; x < left + (int)mouthW; x++) {
    float u   = (x - cx) / (mouthW / 2);         // -1 .. +1
    float mid = midAt(u);
    float t   = thickAt(u);
    int top    = (int)(mid - t * 0.4f);
    int bottom = (int)(mid + t * 0.6f);

    // Brighter toward the middle, where the "candle" is.
    uint16_t col = blendColor(flameColor(c.glow), PUMPKIN_HOT,
                              curveAt(u) * curveAt(u) * 0.6f * c.glow);
    canvas.drawFastVLine(x, top, bottom - top, col);
  }

  // Teeth: blocks of background color "carved" back into the mouth.
  // Each tooth fills about a third of the mouth's height at that spot, so
  // the teeth shrink when the mouth closes instead of cutting it in pieces.
  int toothW = (int)(mouthW * 0.09f);

  // Two teeth hanging from the top lip...
  for (float u : { -0.45f, 0.45f }) {
    int x      = (int)(cx + u * mouthW / 2) - toothW / 2;
    int top    = (int)(midAt(u) - thickAt(u) * 0.4f);
    int toothH = (int)(thickAt(u) * 0.35f);
    canvas.fillRect(x, top - 1, toothW, toothH + 1, PUMPKIN_BG);
  }
  // ...and one sticking up from the bottom lip in the middle.
  {
    int x      = (int)cx - toothW / 2;
    int bottom = (int)(midAt(0) + thickAt(0) * 0.6f);
    int toothH = (int)(thickAt(0) * 0.35f);
    canvas.fillRect(x, bottom - toothH, toothW + 2, toothH + 1, PUMPKIN_BG);
  }
}

// ============================================================================
//  LAYOUT - where each part goes on the screen for each FACE_PART choice.
//  Layer sizes are kept under ~100 KB each so the ESP32 has enough memory.
// ============================================================================
#define PART_FULL_FACE  0
#define PART_LEFT_EYE   1
#define PART_RIGHT_EYE  2
#define PART_MOUTH      3

#ifndef FACE_PART
#define FACE_PART PART_FULL_FACE
#endif

// The layers this board uses. Ones we don't need stay as nullptr (nothing).
Layer *leftEyeLayer  = nullptr;
Layer *rightEyeLayer = nullptr;
Layer *noseLayer     = nullptr;
Layer *mouthLayer    = nullptr;

// Make a layer and reserve its memory in one step.
Layer *makeLayer(TFT_eSPI &tft, int x, int y, int w, int h) {
  Layer *layer = new Layer(&tft);
  layer->begin(x, y, w, h);
  return layer;
}

// Called once from setup(). Picks the screen direction and builds layers.
void designBegin(TFT_eSPI &tft) {
#if FACE_PART == PART_FULL_FACE
  tft.setRotation(0);                         // portrait: 240 wide, 320 tall
  tft.fillScreen(PUMPKIN_BG);
  leftEyeLayer  = makeLayer(tft,   8,  30, 108, 100);
  rightEyeLayer = makeLayer(tft, 124,  30, 108, 100);
  noseLayer     = makeLayer(tft,  98, 145,  44,  36);
  mouthLayer    = makeLayer(tft,   8, 195, 224, 110);
#elif FACE_PART == PART_LEFT_EYE
  tft.setRotation(0);
  tft.fillScreen(PUMPKIN_BG);
  leftEyeLayer  = makeLayer(tft, 10, 50, 220, 220);
#elif FACE_PART == PART_RIGHT_EYE
  tft.setRotation(0);
  tft.fillScreen(PUMPKIN_BG);
  rightEyeLayer = makeLayer(tft, 10, 50, 220, 220);
#elif FACE_PART == PART_MOUTH
  tft.setRotation(1);                         // landscape: 320 wide, 240 tall
  tft.fillScreen(PUMPKIN_BG);
  mouthLayer    = makeLayer(tft, 10, 40, 300, 160);
#endif
}

// Called every frame from loop(). Paint each layer, then show it.
void designDraw(const Character &c) {
  if (leftEyeLayer) {
    paintEye(leftEyeLayer->canvas, leftEyeLayer->w, leftEyeLayer->h, c, true);
    leftEyeLayer->show();
  }
  if (rightEyeLayer) {
    paintEye(rightEyeLayer->canvas, rightEyeLayer->w, rightEyeLayer->h, c, false);
    rightEyeLayer->show();
  }
  if (noseLayer) {
    paintNose(noseLayer->canvas, noseLayer->w, noseLayer->h, c);
    noseLayer->show();
  }
  if (mouthLayer) {
    paintMouth(mouthLayer->canvas, mouthLayer->w, mouthLayer->h, c);
    mouthLayer->show();
  }
}

#endif // DESIGN_PUMPKIN_H
