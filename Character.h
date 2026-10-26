// ============================================================================
//  Character.h  -  "The brain"
//
//  WHAT THIS FILE IS FOR:
//  It decides WHAT the character is doing: when to blink, where to look,
//  how wide the mouth is open, how bright the candle glow is, what mood it
//  is in. It does NOT draw anything.
//
//  WHY SEPARATE THE BRAIN FROM THE DRAWING?
//  The design file (Design_Pumpkin.h) only reads these numbers and paints
//  them. So if the class picks a completely different look, you swap the
//  design file and keep all of this behavior for free. Every design gets
//  the same blinking, glancing and talking.
//
//  ALL VALUES ARE SIMPLE NUMBERS, mostly from 0.0 to 1.0:
//     blink      0 = eyes open,     1 = eyes fully shut
//     lookX      -1 = far left,     0 = center,    +1 = far right
//     lookY      -1 = up,           0 = center,    +1 = down
//     mouthOpen  0 = closed,        1 = wide open
//     glow       0 = dim ember,     1 = bright flame
// ============================================================================

#ifndef CHARACTER_H
#define CHARACTER_H

#include "Anim.h"

// The moods the character can be in. A design decides what each looks like.
enum Mood {
  MOOD_NORMAL,
  MOOD_HAPPY,
  MOOD_ANGRY,
  MOOD_SLEEPY
};

class Character {
public:
  // ---- The numbers the design reads (see the table above) ----
  float blink     = 0.0f;
  float lookX     = 0.0f;
  float lookY     = 0.0f;
  float mouthOpen = 0.0f;
  float glow      = 0.7f;
  Mood  mood      = MOOD_NORMAL;
  bool  talking   = false;

  // ---- Commands you can give it from the main sketch ----

  // Start flapping the mouth for 'ms' milliseconds (0 = until stopTalking()).
  void talk(uint32_t ms = 0) {
    talking = true;
    talkStopAt = (ms == 0) ? 0 : millis() + ms;
  }

  void stopTalking() { talking = false; }

  // Use this LATER to sync the mouth to real audio: pass the loudness of
  // the sound right now (0.0 quiet ... 1.0 loud), every frame while it plays.
  // It overrides the automatic "pretend talking" for a short moment.
  void setMouthLevel(float level) {
    externalLevel = constrain(level, 0.0f, 1.0f);
    externalUntil = millis() + 150;
  }

  void setMood(Mood m) { mood = m; }

  // Make it look at a spot right now (e.g. where someone tapped).
  void lookAt(float x, float y, uint32_t holdMs = 1500) {
    lookTargetX = constrain(x, -1.0f, 1.0f);
    lookTargetY = constrain(y, -1.0f, 1.0f);
    nextLookAt  = millis() + holdMs;
  }

  // Call once in setup().
  void begin() {
    lastUpdate = millis();
    scheduleNextBlink();
  }

  // Call once per frame. Works out how much time passed and moves every
  // value a little toward where it is heading.
  void update() {
    uint32_t now = millis();
    float dt = (now - lastUpdate) / 1000.0f;   // seconds since last frame
    lastUpdate = now;
    if (dt > 0.1f) dt = 0.1f;                  // don't jump after a pause

    updateBlink(now);
    updateLook(now, dt);
    updateMouth(now, dt);
    updateGlow(now, dt);
  }

private:
  uint32_t lastUpdate = 0;

  // ---------------------------------------------------------------- BLINK
  // A blink is a quick close-then-open. Between blinks we wait a random
  // time, because perfectly regular blinking looks robotic.
  uint32_t nextBlinkAt = 0;
  uint32_t blinkStart  = 0;
  bool     blinking    = false;
  bool     doubleBlink = false;
  static const uint32_t BLINK_MS = 180;        // length of one blink

  void scheduleNextBlink() {
    uint32_t wait = (mood == MOOD_SLEEPY) ? random(1500, 3500)
                                          : random(2000, 6000);
    nextBlinkAt = millis() + wait;
  }

  void updateBlink(uint32_t now) {
    if (!blinking && (int32_t)(now - nextBlinkAt) >= 0) {
      blinking    = true;
      blinkStart  = now;
      doubleBlink = (random(100) < 20);        // 20% chance of a double blink
    }

    // A sleepy character keeps its eyes half shut even between blinks.
    float resting = (mood == MOOD_SLEEPY) ? 0.55f : 0.0f;

    if (blinking) {
      float t = (now - blinkStart) / (float)BLINK_MS;   // 0..1 through blink
      if (t >= 1.0f) {
        if (doubleBlink) { doubleBlink = false; blinkStart = now; t = 0; }
        else { blinking = false; scheduleNextBlink(); blink = resting; return; }
      }
      // Up to 1 at the halfway point, then back down: a "tent" shape.
      float closed = 1.0f - fabsf(t * 2.0f - 1.0f);
      blink = max(resting, easeInOut(closed));
    } else {
      blink = resting;
    }
  }

  // ----------------------------------------------------------------- LOOK
  // Every so often pick a new spot to look at, then glide the eyes there.
  // Most of the time it looks back toward the middle.
  float    lookTargetX = 0, lookTargetY = 0;
  uint32_t nextLookAt  = 0;

  void updateLook(uint32_t now, float dt) {
    if ((int32_t)(now - nextLookAt) >= 0) {
      if (random(100) < 40) {                  // 40%: back to center
        lookTargetX = 0;  lookTargetY = 0;
      } else {                                 // 60%: somewhere random
        lookTargetX = randomFloat(-1.0f, 1.0f);
        lookTargetY = randomFloat(-0.6f, 0.6f);
      }
      nextLookAt = now + random(700, 3000);
    }
    lookX = approach(lookX, lookTargetX, 9.0f, dt);  // quick glance
    lookY = approach(lookY, lookTargetY, 9.0f, dt);
  }

  // ---------------------------------------------------------------- MOUTH
  // "Pretend talking": every syllable picks a new random mouth opening.
  // Real speech opens and closes about 4 to 8 times per second.
  uint32_t talkStopAt     = 0;
  uint32_t nextSyllableAt = 0;
  float    mouthTarget    = 0;
  float    externalLevel  = 0;
  uint32_t externalUntil  = 0;

  void updateMouth(uint32_t now, float dt) {
    if (talking && talkStopAt != 0 && (int32_t)(now - talkStopAt) >= 0) {
      talking = false;
    }

    if ((int32_t)(now - externalUntil) < 0) {
      mouthTarget = externalLevel;             // real audio is in charge
    } else if (talking) {
      if ((int32_t)(now - nextSyllableAt) >= 0) {
        // Mostly open, sometimes a short closed gap between words.
        mouthTarget    = (random(100) < 15) ? 0.0f : randomFloat(0.3f, 1.0f);
        nextSyllableAt = now + random(70, 170);
      }
    } else {
      mouthTarget = 0.0f;
    }
    mouthOpen = approach(mouthOpen, mouthTarget, 25.0f, dt);  // very snappy
  }

  // ----------------------------------------------------------------- GLOW
  // A candle never burns perfectly steady. We pick a new random brightness
  // many times per second and drift toward it: that makes a soft flicker.
  float    glowTarget = 0.7f;
  uint32_t nextFlickerAt = 0;

  void updateGlow(uint32_t now, float dt) {
    if ((int32_t)(now - nextFlickerAt) >= 0) {
      float base = (mood == MOOD_ANGRY) ? 0.85f :
                   (mood == MOOD_SLEEPY) ? 0.35f : 0.65f;
      glowTarget = constrain(base + randomFloat(-0.25f, 0.25f), 0.0f, 1.0f);
      if (random(100) < 3) glowTarget = 0.1f;  // rare big gutter
      nextFlickerAt = now + random(50, 160);
    }
    // Talking makes the flame jump a little brighter.
    float target = min(1.0f, glowTarget + mouthOpen * 0.2f);
    glow = approach(glow, target, 12.0f, dt);
  }
};

#endif // CHARACTER_H
