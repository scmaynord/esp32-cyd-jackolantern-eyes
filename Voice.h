// ============================================================================
//  Voice.h  -  "The character's voice"
//
//  WHAT THIS FILE IS FOR:
//  Plays recorded voice clips from the microSD card through the speaker
//  plugged into the CYD's SPEAKER connector, and measures how LOUD the
//  sound is at each moment. The brain uses that loudness to make the eyes
//  react while the character speaks (brighter flame, wider pupils).
//
//  PREPARING CLIPS:
//  1. Record or generate the lines (any app). Keep each one short.
//  2. Save them as WAV files named  voice1.wav, voice2.wav, voice3.wav ...
//     (no gaps in the numbering) in the top folder of a microSD card.
//     Best format: mono, 22050 Hz, 8-bit or 16-bit PCM.
//       Audacity:  Tracks > Mix > Mix Stereo Down to Mono,
//                  set Project Rate to 22050, File > Export > WAV
//       ffmpeg:    ffmpeg -i line.mp3 -ac 1 -ar 22050 -c:a pcm_u8 voice1.wav
//  3. Put the SAME file names on BOTH eyes' cards. (Trick: the files can be
//     different! Same name, different recording = a stereo or echo effect.)
//
//  HOW SOUND COMES OUT:
//  The ESP32 has a DAC ("Digital to Analog Converter") on pin 26 that turns
//  numbers 0-255 into a voltage. 128 = silence (speaker cone in the middle),
//  0 and 255 = pushed all the way in or out. Playing thousands of these
//  numbers per second makes sound. A helper inside the chip called DMA
//  feeds the numbers to the DAC on time, so our code only has to keep its
//  small "to play" list topped up.
//
//  Playback runs as a separate TASK on the ESP32's other processor core,
//  so the animation never stutters while sound is playing.
//
//  NEEDS: ESP32 Arduino core version 3.0 or newer (Tools > Board > Boards
//  Manager > esp32 by Espressif).
// ============================================================================

#ifndef VOICE_H
#define VOICE_H

#include <SPI.h>
#include <SD.h>
#include "driver/dac_continuous.h"
#include "CYD_Pins.h"

#if ESP_ARDUINO_VERSION_MAJOR < 3
#error "Voice.h needs ESP32 Arduino core 3.0 or newer (Boards Manager > esp32)"
#endif

#ifndef VOICE_VOLUME
#define VOICE_VOLUME 200        // 0 = silent ... 255 = loudest (may distort)
#endif

// ----------------------------------------------------------------------------
//  READING A WAV FILE
//  A WAV file starts with a header: little labelled sections ("chunks") that
//  say how the sound is stored. "fmt " tells us the format; "data" is the
//  actual sound. We read the header, then pull out the sound in small bites.
// ----------------------------------------------------------------------------
struct WavInfo {
  uint32_t sampleRate   = 0;    // samples per second, e.g. 22050
  uint16_t channels     = 0;    // 1 = mono, 2 = stereo
  uint16_t bitsPerSample= 0;    // 8 or 16
  uint32_t dataBytesLeft= 0;    // how much sound is left to read
};

// Little helpers: WAV numbers are stored lowest byte first ("little endian").
inline uint32_t readU32(File &f) { uint8_t b[4] = {0}; f.read(b, 4);
  return b[0] | (b[1] << 8) | (b[2] << 16) | ((uint32_t)b[3] << 24); }
inline uint16_t readU16(File &f) { uint8_t b[2] = {0}; f.read(b, 2);
  return b[0] | (b[1] << 8); }

// Read the header. Returns false if this isn't a WAV we can play.
bool wavReadHeader(File &f, WavInfo &info) {
  char id[4];
  if (f.read((uint8_t *)id, 4) != 4 || memcmp(id, "RIFF", 4) != 0) return false;
  readU32(f);                                         // total file size
  if (f.read((uint8_t *)id, 4) != 4 || memcmp(id, "WAVE", 4) != 0) return false;

  bool haveFormat = false;
  while (f.available() >= 8) {                        // walk through the chunks
    f.read((uint8_t *)id, 4);
    uint32_t size = readU32(f);

    if (memcmp(id, "fmt ", 4) == 0) {
      uint16_t format    = readU16(f);                // 1 = plain PCM
      info.channels      = readU16(f);
      info.sampleRate    = readU32(f);
      readU32(f);                                     // byte rate (unused)
      readU16(f);                                     // block align (unused)
      info.bitsPerSample = readU16(f);
      f.seek(f.position() + size - 16 + (size & 1));  // skip any extra bytes
      if (format != 1) return false;                  // compressed: can't play
      if (info.bitsPerSample != 8 && info.bitsPerSample != 16) return false;
      if (info.channels < 1 || info.channels > 2) return false;
      haveFormat = true;
    } else if (memcmp(id, "data", 4) == 0) {
      info.dataBytesLeft = size;
      return haveFormat;                              // ready to read sound
    } else {
      f.seek(f.position() + size + (size & 1));       // some other chunk: skip
    }
  }
  return false;
}

// Read up to 'maxSamples' sound samples, converted to what our DAC wants:
// mono, 8-bit, 0-255 with 128 = silence. Returns how many were read.
size_t wavReadSamples(File &f, WavInfo &info, uint8_t *out, size_t maxSamples) {
  const size_t bytesPerSample = (info.bitsPerSample / 8) * info.channels;
  uint8_t raw[512];
  size_t count = 0;

  while (count < maxSamples && info.dataBytesLeft >= bytesPerSample) {
    size_t want = min((maxSamples - count) * bytesPerSample, sizeof(raw));
    want = min(want, (size_t)info.dataBytesLeft);
    want -= want % bytesPerSample;                    // whole samples only
    size_t got = f.read(raw, want);
    got -= got % bytesPerSample;
    if (got == 0) break;
    info.dataBytesLeft -= got;

    for (size_t i = 0; i < got; i += bytesPerSample) {
      int mixed = 0;                                  // -32768 .. 32767
      for (int ch = 0; ch < info.channels; ch++) {
        if (info.bitsPerSample == 8) {
          // 8-bit WAV is 0..255 (128 = silence): make it -32768..32767
          mixed += ((int)raw[i + ch] - 128) << 8;
        } else {
          // 16-bit WAV is already -32768..32767, low byte first
          const uint8_t *s = &raw[i + ch * 2];
          mixed += (int16_t)(s[0] | (s[1] << 8));
        }
      }
      mixed /= info.channels;                         // stereo -> mono
      out[count++] = (uint8_t)((mixed >> 8) + 128);   // back to 0..255
    }
  }
  return count;
}

// ----------------------------------------------------------------------------
//  THE PLAYER
// ----------------------------------------------------------------------------
namespace voice {
  QueueHandle_t requests = nullptr;     // clip numbers waiting to be played
  volatile float level   = 0.0f;        // loudness right now, 0..1
  volatile bool  playing = false;
  volatile uint8_t volume = VOICE_VOLUME;
  int clipCount = 0;                    // how many voiceN.wav files we found

  SPIClass sdSpi(VSPI);                 // the SD card has its own SPI bus
  dac_continuous_handle_t dac = nullptr;
  uint32_t dacRate = 0;

  const size_t CHUNK = 256;             // samples per bite (~12 ms of sound)
  const size_t DMA_BUFFERS = 4;         // small = eyes react with little delay

  // (Re)start the DAC at a new sample rate.
  bool startDac(uint32_t rate) {
    if (dac && rate == dacRate) return true;
    if (dac) {                          // different rate: rebuild it
      dac_continuous_disable(dac);
      dac_continuous_del_channels(dac);
      dac = nullptr;
    }
    dac_continuous_config_t cfg = {};
    cfg.chan_mask = DAC_CHANNEL_MASK_CH1;        // channel 1 = GPIO 26 = speaker
    cfg.desc_num  = DMA_BUFFERS;
    cfg.buf_size  = CHUNK;
    cfg.freq_hz   = rate;
    cfg.offset    = 0;
    cfg.clk_src   = DAC_DIGI_CLK_SRC_APLL;       // audio clock: works 648 Hz+
    cfg.chan_mode = DAC_CHANNEL_MODE_SIMUL;
    if (dac_continuous_new_channels(&cfg, &dac) != ESP_OK) {
      Serial.printf("Voice: can't start the DAC at %u Hz\n", rate);
      dac = nullptr;
      return false;
    }
    dac_continuous_enable(dac);
    dacRate = rate;
    return true;
  }

  // Fill every DMA buffer with 128 (= silence) so nothing buzzes afterwards.
  void writeSilence() {
    static uint8_t quiet[CHUNK];
    memset(quiet, 128, sizeof(quiet));
    for (size_t i = 0; i < DMA_BUFFERS + 1; i++) {
      dac_continuous_write(dac, quiet, CHUNK, nullptr, 100);
    }
  }

  void playClip(int clip) {
    char path[24];
    snprintf(path, sizeof(path), "/voice%d.wav", clip);
    File f = SD.open(path);
    if (!f) { Serial.printf("Voice: %s not found\n", path); return; }

    WavInfo info;
    if (!wavReadHeader(f, info) || !startDac(info.sampleRate)) {
      Serial.printf("Voice: %s is not a plain PCM WAV\n", path);
      f.close();
      return;
    }

    Serial.printf("Voice: playing %s (%u Hz)\n", path, info.sampleRate);
    playing = true;
    static uint8_t samples[CHUNK];

    while (true) {
      // A new request arrived? Stop this clip and let the next one start.
      if (uxQueueMessagesWaiting(requests) > 0) break;

      size_t n = wavReadSamples(f, info, samples, CHUNK);
      if (n == 0) break;                               // end of the clip

      // Measure loudness: average distance from silence (128).
      uint32_t total = 0;
      for (size_t i = 0; i < n; i++) total += abs((int)samples[i] - 128);
      float loud = (total / (float)n) / 40.0f;         // speech ~ 0.3..1.0
      level = constrain(loud, 0.0f, 1.0f);

      // Apply volume: shrink the distance from silence.
      uint8_t vol = volume;
      for (size_t i = 0; i < n; i++) {
        samples[i] = 128 + (((int)samples[i] - 128) * vol) / 255;
      }
      if (n < CHUNK) memset(samples + n, 128, CHUNK - n);

      // Hand it to the DMA. This waits until there is room, which is what
      // keeps our task running at exactly the right speed.
      dac_continuous_write(dac, samples, CHUNK, nullptr, -1);
    }

    f.close();
    writeSilence();
    level   = 0.0f;
    playing = false;
  }

  // The task: wait for a clip number, play it, repeat forever.
  void task(void *) {
    int clip;
    while (true) {
      if (xQueueReceive(requests, &clip, portMAX_DELAY) == pdTRUE) {
        playClip(clip);
      }
    }
  }
}

// ----------------------------------------------------------------------------
//  Call once in setup(). Returns true if the SD card was found.
// ----------------------------------------------------------------------------
bool voiceBegin() {
  voice::sdSpi.begin(CYD_SD_SCK, CYD_SD_MISO, CYD_SD_MOSI, CYD_SD_CS);
  if (!SD.begin(CYD_SD_CS, voice::sdSpi)) {
    Serial.println("Voice: no SD card - the character will stay quiet");
    return false;
  }

  // Count voice1.wav, voice2.wav, ... until one is missing.
  char path[24];
  while (true) {
    snprintf(path, sizeof(path), "/voice%d.wav", voice::clipCount + 1);
    if (!SD.exists(path)) break;
    voice::clipCount++;
  }
  Serial.printf("Voice: found %d clip(s)\n", voice::clipCount);

  if (voice::startDac(22050)) voice::writeSilence();   // start out quiet

  voice::requests = xQueueCreate(4, sizeof(int));
  // Run the player on core 0; the animation runs on core 1.
  xTaskCreatePinnedToCore(voice::task, "voice", 4096, nullptr, 5, nullptr, 0);
  return true;
}

// Ask for a clip to be played (1 = voice1.wav). Returns right away.
void voicePlay(int clip) {
  if (voice::requests) xQueueSend(voice::requests, &clip, 0);
}

// Pick a random clip number, or 0 if there are none.
int voiceRandomClip() {
  return voice::clipCount > 0 ? random(1, voice::clipCount + 1) : 0;
}

#endif // VOICE_H
