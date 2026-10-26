// ============================================================================
//  EyeLink.h  -  "How the two eyes stay in sync"
//
//  THE PROBLEM:
//  Each eye is a separate CYD with its own brain. Left alone, they would
//  blink at different times and look in different directions - creepy in
//  the wrong way!
//
//  THE SOLUTION - LEADER AND FOLLOWER:
//  The LEFT eye is the LEADER. It runs the brain (Character.h) and, every
//  frame, radios the brain's numbers to the right eye. The RIGHT eye is the
//  FOLLOWER: it skips its own brain and just paints whatever it receives.
//  Both eyes paint the same numbers, so they blink and glance together.
//  If the radio goes quiet for half a second, the follower switches to its
//  own brain so it never freezes, then rejoins when messages come back.
//
//  THE RADIO - ESP-NOW:
//  ESP-NOW is built into every ESP32. It uses the Wi-Fi radio, but talks
//  board-to-board directly: no router, no password, no internet, and a
//  message arrives in a few milliseconds. Perfect for outdoors.
//  (Bluetooth could work too, but it needs pairing, uses much more memory,
//  and is harder to get right.)
//
//  We BROADCAST: the leader shouts to every ESP32 in range. To ignore other
//  projects nearby, every message starts with our GROUP_ID. If two classes
//  build pumpkins with this code, give each pumpkin a different GROUP_ID.
// ============================================================================

#ifndef EYELINK_H
#define EYELINK_H

#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "Character.h"

#define EYE_GROUP_ID   0x4A    // change if another pumpkin uses this code nearby
#define EYE_CHANNEL    1       // radio channel 1-13; both eyes must match
#define LINK_TIMEOUT_MS 500    // follower waits this long before going solo

// ----------------------------------------------------------------------------
//  THE MESSAGE. "packed" means: no hidden gaps between the fields, so both
//  boards agree on exactly where each byte is.
// ----------------------------------------------------------------------------
enum PacketKind : uint8_t {
  PACKET_STATE = 1,      // "here is what the face looks like right now"
  PACKET_PLAY  = 2       // "start playing voice clip number N"
};

struct __attribute__((packed)) EyePacket {
  uint8_t group;         // must equal EYE_GROUP_ID or we ignore it
  uint8_t kind;          // PACKET_STATE or PACKET_PLAY
  uint8_t mood;
  uint8_t clip;          // clip number (for PACKET_PLAY)
  uint8_t playId;        // changes each new play, so repeats are ignored
  float   blink, lookX, lookY, glow, mouthOpen;
};

// ----------------------------------------------------------------------------
//  Behind-the-scenes storage. Messages arrive on a different task than
//  loop(), so we guard the shared copy with a lock ("mutex"): only one side
//  can touch it at a time, so we never read half an old and half a new one.
// ----------------------------------------------------------------------------
namespace eyelink {
  const uint8_t   BROADCAST[6] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
  portMUX_TYPE    lock = portMUX_INITIALIZER_UNLOCKED;
  EyePacket       lastState;            // newest state message
  volatile uint32_t lastStateAt = 0;    // millis() when it arrived
  volatile int    pendingClip = -1;     // clip waiting to be played, or -1
  uint8_t         lastPlayId  = 0;      // to ignore repeated play messages
  uint8_t         nextPlayId  = 1;
  bool            wasLinked   = false;

  // Called by the radio every time ANY ESP-NOW message arrives.
  void handle(const uint8_t *data, int len) {
    if (len != sizeof(EyePacket)) return;           // wrong size: not ours
    EyePacket p;
    memcpy(&p, data, sizeof(p));
    if (p.group != EYE_GROUP_ID) return;            // another project

    portENTER_CRITICAL(&lock);
    if (p.kind == PACKET_STATE) {
      lastState   = p;
      lastStateAt = millis();
    } else if (p.kind == PACKET_PLAY && p.playId != lastPlayId) {
      lastPlayId  = p.playId;
      pendingClip = p.clip;
    }
    portEXIT_CRITICAL(&lock);
  }

  // The callback's shape changed in ESP32 Arduino core 3.0, so we provide
  // the right one for whichever version is installed.
#if ESP_ARDUINO_VERSION_MAJOR >= 3
  void onReceive(const esp_now_recv_info_t *info, const uint8_t *data, int len) {
    handle(data, len);
  }
#else
  void onReceive(const uint8_t *mac, const uint8_t *data, int len) {
    handle(data, len);
  }
#endif
}

// ----------------------------------------------------------------------------
//  Turn the radio on. Call once in setup() on BOTH eyes.
// ----------------------------------------------------------------------------
bool eyeLinkBegin() {
  WiFi.mode(WIFI_STA);                 // radio on, but not joining a network
  WiFi.disconnect();
  esp_wifi_set_channel(EYE_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("EyeLink: ESP-NOW failed to start");
    return false;
  }
  esp_now_register_recv_cb(eyelink::onReceive);

  // To SEND, ESP-NOW needs a "peer". The broadcast address = everyone.
  esp_now_peer_info_t peer = {};
  memcpy(peer.peer_addr, eyelink::BROADCAST, 6);
  peer.channel = 0;                    // 0 = whatever channel we're on
  peer.encrypt = false;
  peer.ifidx   = WIFI_IF_STA;
  esp_now_add_peer(&peer);

  Serial.printf("EyeLink ready on channel %d, group 0x%02X\n",
                EYE_CHANNEL, EYE_GROUP_ID);
  return true;
}

// ============================================================================
//  LEADER SIDE
// ============================================================================

// Send what the face looks like right now. Call once per frame.
void eyeLinkSendState(const Character &c) {
  EyePacket p = {};
  p.group = EYE_GROUP_ID;
  p.kind  = PACKET_STATE;
  p.mood  = (uint8_t)c.mood;
  p.blink = c.blink;   p.lookX = c.lookX;   p.lookY = c.lookY;
  p.glow  = c.glow;    p.mouthOpen = c.mouthOpen;
  esp_now_send(eyelink::BROADCAST, (const uint8_t *)&p, sizeof(p));
}

// Tell the follower to start a voice clip. Radio messages can get lost, so
// we send it 3 times; the playId lets the follower ignore the extra copies.
void eyeLinkSendPlay(int clip) {
  EyePacket p = {};
  p.group  = EYE_GROUP_ID;
  p.kind   = PACKET_PLAY;
  p.clip   = (uint8_t)clip;
  p.playId = eyelink::nextPlayId++;
  if (eyelink::nextPlayId == 0) eyelink::nextPlayId = 1;  // 0 = "none yet"
  for (int i = 0; i < 3; i++) {
    esp_now_send(eyelink::BROADCAST, (const uint8_t *)&p, sizeof(p));
    delay(2);
  }
}

// ============================================================================
//  FOLLOWER SIDE
// ============================================================================

// If a fresh state message has arrived, copy it into the character and
// return true. Return false if the leader has gone quiet.
bool eyeLinkReceiveState(Character &c) {
  EyePacket p;
  uint32_t  at;
  portENTER_CRITICAL(&eyelink::lock);
  p  = eyelink::lastState;
  at = eyelink::lastStateAt;
  portEXIT_CRITICAL(&eyelink::lock);

  bool linked = (at != 0) && (millis() - at < LINK_TIMEOUT_MS);

  if (linked != eyelink::wasLinked) {        // report changes once
    Serial.println(linked ? "EyeLink: following the left eye"
                          : "EyeLink: lost the left eye, running solo");
    eyelink::wasLinked = linked;
  }
  if (!linked) return false;

  c.mood  = (Mood)p.mood;
  c.blink = p.blink;   c.lookX = p.lookX;   c.lookY = p.lookY;
  c.glow  = p.glow;    c.mouthOpen = p.mouthOpen;
  return true;
}

// Returns a clip number the leader asked us to play, or -1 if none.
int eyeLinkTakePlayRequest() {
  portENTER_CRITICAL(&eyelink::lock);
  int clip = eyelink::pendingClip;
  eyelink::pendingClip = -1;
  portEXIT_CRITICAL(&eyelink::lock);
  return clip;
}

#endif // EYELINK_H
