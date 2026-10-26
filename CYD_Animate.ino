// ============================================================================
//  CYD_Animate.ino  -  Animated character for the Cheap Yellow Display
//
//  THE SETUP:  two CYDs, one behind each eye of the pumpkin, each with a
//  speaker plugged into its SPEAKER connector and a microSD card of voice
//  clips. The LEFT eye is the leader: it decides what the character does
//  and radios it to the RIGHT eye, so both eyes move as one face.
//  Upload this same sketch to both boards; only FACE_PART changes.
//
//  FILES IN THIS FOLDER:
//    CYD_Animate.ino     this file: settings, setup(), loop(), test controls
//    Anim.h              animation toolbox (layers, timers, smooth motion)
//    Character.h         the "brain": blinking, glancing, talking, moods
//    Design_Pumpkin.h    the "painter": what the face looks like
//    EyeLink.h           radio link that keeps the two eyes in sync
//    Voice.h             plays voice clips from the SD card on the speaker
//    img2cyd.py          (runs on a computer) turns drawings into Images
//    CYD_Pins.h, Config.h, User_Setup.h
//                        copy these three from CYD_Template
//                        (User_Setup.h also goes into the TFT_eSPI library)
//
//  HOW A FRAME IS MADE (about 30 times every second):
//    LEFT eye:   1. brain updates   2. paint the eye   3. radio the numbers
//    RIGHT eye:  1. receive numbers 2. paint the eye
//
//  TEST CONTROLS (on the LEFT eye):
//    Press the BOOT button  -> say a random clip
//    Serial Monitor (115200 baud), type and press Enter:
//      1-9 = say that clip   p = say a random clip   t = pretend-talk 3 s
//      n = normal   h = happy   a = angry   z = sleepy
//      l = look left   r = look right   c = look center
// ============================================================================

// ----------------------------------------------------------------------------
//  SETTINGS - these MUST come before the #include lines below.
// ----------------------------------------------------------------------------

// Which part is THIS board?
//   1 = LEFT eye  (the leader)     2 = RIGHT eye (the follower)
//   0 = full face on one screen    3 = mouth on its own screen
#define FACE_PART   1

#define USE_VOICE   1       // 1 = play voice clips from the SD card
#define VOICE_VOLUME 200    // 0 (silent) ... 255 (loudest)

// Every so often the character speaks on its own (leader only).
// A random wait between these two numbers of seconds. 0 = never.
#define CHATTER_MIN_S  20
#define CHATTER_MAX_S  60

#define FRAME_RATE  30      // frames per second (30 looks smooth)

// ----------------------------------------------------------------------------
#include <SPI.h>
#include <TFT_eSPI.h>

// SAFETY CHECK: TFT_eSPI reads its settings from ITS OWN folder, not ours.
// If it isn't using the CYD settings it talks to the wrong pins and the
// screen just glows white. Catch that here with a clear message instead.
#if !defined(TFT_MOSI) || TFT_MOSI != 13 || TFT_SCLK != 14 || TFT_CS != 15 || TFT_DC != 2
#error "TFT_eSPI is not using the CYD settings. Copy User_Setup.h into the TFT_eSPI library folder (libraries/TFT_eSPI/User_Setup.h), replacing the one there, then compile again."
#endif

#include "CYD_Pins.h"
#include "Config.h"
#include "Anim.h"
#include "Character.h"
#include "Design_Pumpkin.h"   // <- swap this line to change the design

// The two eye settings use the radio; the others are single screens.
#define USE_LINK   (FACE_PART == PART_LEFT_EYE || FACE_PART == PART_RIGHT_EYE)
#define IS_LEADER  (FACE_PART != PART_RIGHT_EYE)

#if USE_LINK
#include "EyeLink.h"
#endif
#if USE_VOICE
#include "Voice.h"
#endif

// In case your Config.h doesn't have a brightness setting yet.
#ifndef BACKLIGHT_LEVEL
#define BACKLIGHT_LEVEL 255
#endif

TFT_eSPI  tft;                         // the real screen
Character pumpkin;                     // the brain
Every     frameTimer(1000 / FRAME_RATE);

uint32_t nextChatterAt = 0;            // when to speak on our own next

// For the Serial Monitor: how many frames we drew in the last 5 seconds.
uint32_t framesDrawn = 0;
Every    fpsReport(5000);

void handleTestControls();             // these are written further down
void speak(int clip);
void scheduleChatter();

void setup() {
  Serial.begin(115200);

  // Seed the random numbers from the ESP32's hardware noise, so each board
  // blinks and glances differently instead of repeating the same pattern.
  randomSeed(esp_random());

  tft.init();
  analogWrite(CYD_TFT_BL, BACKLIGHT_LEVEL);   // screen brightness

  pinMode(CYD_BOOT_BTN, INPUT_PULLUP);        // BOOT button reads LOW when pressed

  designBegin(tft);                            // picks rotation, makes layers
  pumpkin.begin();

#if USE_LINK
  eyeLinkBegin();
  Serial.println(IS_LEADER ? "I am the LEFT eye (leader)"
                           : "I am the RIGHT eye (follower)");
#endif
#if USE_VOICE
  voiceBegin();
#endif
  scheduleChatter();

  Serial.printf("Ready. Free memory: %u bytes\n", ESP.getFreeHeap());
}

void loop() {
  handleTestControls();

#if USE_VOICE && USE_LINK && !IS_LEADER
  // Follower: did the leader tell us to start a clip?
  int clip = eyeLinkTakePlayRequest();
  if (clip > 0) voicePlay(clip);
#endif

  // Leader: time to say something on our own?
  if (IS_LEADER && CHATTER_MIN_S > 0 &&
      (int32_t)(millis() - nextChatterAt) >= 0) {
#if USE_VOICE
    if (!voice::playing) speak(voiceRandomClip());
#else
    speak(0);
#endif
    scheduleChatter();
  }

  // Only draw when it's time for the next frame. The rest of the time the
  // loop is free to do other things.
  if (frameTimer.ready()) {
    bool following = false;
#if USE_LINK && !IS_LEADER
    following = eyeLinkReceiveState(pumpkin);  // copy the leader's numbers
#endif
    if (!following) {                          // we are the brain
#if USE_VOICE
      if (voice::playing) pumpkin.setMouthLevel(voice::level);
#endif
      pumpkin.update();
    }

    designDraw(pumpkin);

#if USE_LINK && IS_LEADER
    eyeLinkSendState(pumpkin);                 // tell the right eye
#endif
    framesDrawn++;
  }

  if (fpsReport.ready()) {
    Serial.printf("%.1f frames per second\n", framesDrawn / 5.0f);
    framesDrawn = 0;
  }
}

// Say clip number 'clip' on this board AND (if we lead) on the other eye.
// Clip 0 means "no clips available": just pretend-talk with the eyes.
void speak(int clip) {
  if (clip <= 0) {
    pumpkin.talk(2500);
    return;
  }
#if USE_VOICE
  voicePlay(clip);
#endif
#if USE_LINK && IS_LEADER
  eyeLinkSendPlay(clip);
#endif
}

void scheduleChatter() {
  if (CHATTER_MIN_S > 0) {
    nextChatterAt = millis() + random(CHATTER_MIN_S, CHATTER_MAX_S + 1) * 1000UL;
  }
}

// Read the BOOT button and the Serial Monitor so we can test behaviors.
void handleTestControls() {
  static uint32_t lastPress = 0;
  if (digitalRead(CYD_BOOT_BTN) == LOW && millis() - lastPress > 1000) {
    lastPress = millis();
#if USE_VOICE
    speak(voiceRandomClip());
#else
    speak(0);
#endif
  }

  if (Serial.available()) {
    char cmd = Serial.read();
    if (cmd >= '1' && cmd <= '9') { speak(cmd - '0'); return; }
    switch (cmd) {
#if USE_VOICE
      case 'p': speak(voiceRandomClip());       break;
#endif
      case 't': pumpkin.talk(3000);             break;
      case 'n': pumpkin.setMood(MOOD_NORMAL);   break;
      case 'h': pumpkin.setMood(MOOD_HAPPY);    break;
      case 'a': pumpkin.setMood(MOOD_ANGRY);    break;
      case 'z': pumpkin.setMood(MOOD_SLEEPY);   break;
      case 'l': pumpkin.lookAt(-1, 0);          break;
      case 'r': pumpkin.lookAt( 1, 0);          break;
      case 'c': pumpkin.lookAt( 0, 0);          break;
    }
  }
}
