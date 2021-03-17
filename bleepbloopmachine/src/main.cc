#include "main.h"
#include "controls.h"

#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define SPEAKER_ENABLE 51

#define NEOPIXEL_PIN 8
#define NEOPIXEL_LENGTH 5

#define LIGHT_SENSOR A7
#define BATTERY_SENSOR A6

#define RIGHT_AUDIO_PIN A0
#define LEFT_AUDIO_PIN A1

#define SD_CS 4

#include "Adafruit_TinyUSB.h"
#include "AudioStream.h"
#include "output_dacs.h"

#include "freq.h"
#include <Adafruit_NeoPixel.h>
#include <Audio.h>

#define DISPLAY_CS 44
#define DISPLAY_RST 46
#define DISPLAY_DC 45
#define DISPLAY_BACKLIGHT 47
#define DISPLAY_HEIGHT 160
#define DISPLAY_WIDTH 128

Adafruit_ST7735 display =
    Adafruit_ST7735(&SPI1, DISPLAY_CS, DISPLAY_DC, DISPLAY_RST);

enum class Waveform {
  sine,
  sawtooth,
  square,
  triangle,
  arbitrary,
  pulse,
  sawtoothReverse,
  sampleHold
};

int wrapping_add(int cur, int max, int inc = 1, int min = 0) {
  auto nxt = cur + inc;
  return nxt <= max ? nxt : min;
}
int wrapping_sub(int cur, int max, int inc = 1, int min = 0) {
  auto nxt = cur - inc;
  return nxt >= min ? nxt : max;
}

String note_names[] = {"c",  "c#", "d",  "d#", "e",  "f",
                       "f#", "g",  "g#", "a",  "a#", "b"};
String octave_names[] = {"0", "1", "2", "3", "4", "5", "6", "7", "8"};

#define BLOCK_SIZE 20
#define GRID_OFFSET_X 36
#define GRID_OFFSET_Y 20
#define GRID_GAP 2
#define STANDARD_BLOCK_COLOR 0xfebb
#define TICK_BLOCK_COLOR 0xfc53

struct Point {
  int x;
  int y;
};

Point getCoord(int idx) {
  int x = idx % 4;
  int y = idx / 4;
  return Point{
    x : GRID_OFFSET_X + (x * (BLOCK_SIZE + GRID_GAP)),
    y : GRID_OFFSET_Y + (y * (BLOCK_SIZE + GRID_GAP))
  };
}

class SoundBlock {
public:
  int note = 0;
  int octave = 3;
  bool active = false;
  float attack = 10.0;
  float decay = 40.0;
  float delay = 0.0;
  Waveform waveform;
  float frequency() { return freq[note][octave]; }
  void activate(int n, int o) {
    note = n;
    octave = o;
    active = true;
  }
  void activate() { active = true; }
  void noteUp() {
    note = wrapping_add(note, 11);
  }
  void noteDown() { note = wrapping_sub(note, 11); }
  void octaveUp() { octave = wrapping_add(octave, 8); }
  void octaveDown() { octave = wrapping_sub(octave, 8); }
  SoundBlock cut() {
    active = false;
    SoundBlock s;
    s.note = note;
    s.octave = octave;
    s.active = true;
    s.attack = attack;
    s.decay = decay;
    s.delay = delay;
    return s;
  }
  SoundBlock() {}
};

class Wave {
public:
  Waveform form;
  String name;
  SoundBlock sounds[16];
  AudioSynthWaveform &synth;
  AudioEffectEnvelope &env;
  int rate = 4;
  int lastStepBlockIndex = 0;
  Wave(String n, AudioSynthWaveform &s, Waveform f, AudioEffectEnvelope &e)
      : name{n}, form{f}, synth{s}, env{e} {
    for (auto i = 0; i < 16; i++) {
      sounds[i] = SoundBlock();
    }
    synth.begin((int)form);
    synth.amplitude(1.0);
    env.sustain(0.0);
  }
  int stepBlockIndex(int currentStep) { return currentStep / rate % 16; }
  void play(int blockIndex) {
    auto sound = sounds[blockIndex];
    if (!sound.active) {
      return;
    }
    synth.frequency(sound.frequency());
    env.attack(sound.attack);
    env.decay(sound.decay);
    env.delay(sound.delay);
    env.noteOn();
  }
  void step(int currentTick) {
    auto idx = stepBlockIndex(currentTick);
    if (idx != lastStepBlockIndex) {
      lastStepBlockIndex = idx;
      play(idx);
    }
  }
  void updateDisplay(Adafruit_GFX *display, menu_mode mode) {
    for (int i = 0; i <= 0xf; i++) {
      if (mode == menu_mode::live) {
        auto sound = &sounds[i];
        if (sound->active) {
          Point point = getCoord(i);
          String note = note_names[sound->note];
          display->drawChar(point.x + 2, point.y + 2, note[0], ST7735_BLACK,
                            ST7735_BLACK, 1);
          if (note.length() == 2) {
            display->drawChar(point.x + 9, point.y + 2, '#', ST7735_BLACK,
                              ST7735_BLACK, 1);
          }
          display->fillRect(point.x + (sound->octave * 2), point.y, 2, 2,
                            sound->octave * 10);
        }
      } else if (mode == menu_mode::menu1) {
      } else if (mode == menu_mode::menu2) {
        }
    }
  }
};

// TODO make the synth and env and patch as part of creating a Wave
AudioSynthWaveform synth_q;
AudioSynthWaveform synth_p;
AudioSynthWaveform synth_s;
AudioSynthWaveform synth_n;

AudioEffectEnvelope env_q;
AudioEffectEnvelope env_p;
AudioEffectEnvelope env_s;
AudioEffectEnvelope env_n;

Wave wave_q("q", synth_q, Waveform::square, env_q);
Wave wave_p("p", synth_p, Waveform::pulse, env_p);
Wave wave_s("s", synth_s, Waveform::sine, env_s);
Wave wave_n("n", synth_n, Waveform::sampleHold, env_n);

class BleepBloopMachine {
public:
  menu_mode mode = menu_mode::live;
  float bpm = 120.0;
  // TODO ask abe how to write this without brackets
  long lastMicros = micros();
  long stepLength() { return ((1 / bpm) / 16) * 60000000; }
  int selectedSoundIndex = 0;
  int selectedMenu1Control = 0;
  int selectedMenu2Control = 0;
  Wave &selectedWave = wave_q;
  int selectedBlockIndex = 0;
  int lastSelectedBlockIndex = 0;
  int currentStep = 0;
  int lastNote = 0;
  int lastOctave = 3;
  SoundBlock clipboard;
  bool hasClipboard = false;
  void step() {
    auto m = micros();
    if (m - lastMicros >= stepLength()) {
      currentStep += 1;
      if (currentStep > 63) {
        currentStep = 0;
      }
      lastMicros = m;
      wave_q.step(currentStep);
      wave_p.step(currentStep);
      wave_s.step(currentStep);
      wave_n.step(currentStep);
      updateGrid();
    }
  }
  void updateGrid() {
    auto stepBlockIndex = selectedWave.stepBlockIndex(currentStep);
    // first comes blocks
    for (auto i = 0; i <= 0xf; i++) {
      auto coord = getCoord(i);
      auto color =
          stepBlockIndex == i ? TICK_BLOCK_COLOR : STANDARD_BLOCK_COLOR;
      display.fillRect(coord.x, coord.y, BLOCK_SIZE, BLOCK_SIZE, color);
    }
    // then comes selection
    auto selectCoord = getCoord(selectedBlockIndex);
    display.drawRect(selectCoord.x, selectCoord.y, BLOCK_SIZE, BLOCK_SIZE,
                     ST7735_BLACK);

    // then the labels
    selectedWave.updateDisplay(&display, mode);
  }
  void handleButtons(uint32_t held, uint32_t released) {
    bool ab_mode = held & PAD_A && held & PAD_B;
    bool a_mode = !ab_mode && held & PAD_A;
    bool b_mode = !ab_mode && held & PAD_B;
    bool none_mode = !ab_mode && !a_mode && !b_mode;
    switch (mode) {
    case menu_mode::live:
      if (b_mode) {
        auto sound = &selectedWave.sounds[selectedBlockIndex];
        if (released & PAD_UP) {
          sound->noteUp();
        }
        if (released & PAD_DOWN) {
          sound->noteDown();
        }
        if (released & PAD_LEFT) {
          sound->octaveDown();
        }
        if (released & PAD_RIGHT) {
          sound->octaveUp();
        }
        lastNote = sound->note;
        lastOctave = sound->octave;
      } else if (none_mode) {
        if (released & PAD_UP && selectedBlockIndex > 0x3) {
          selectedBlockIndex -= 4;
          return;
        }
        if (released & PAD_DOWN && selectedBlockIndex < 0xc) {
          selectedBlockIndex += 4;
          return;
        }
        if (released & PAD_LEFT && selectedBlockIndex > 0x0) {
          selectedBlockIndex -= 1;
          return;
        }
        if (released & PAD_RIGHT && selectedBlockIndex < 0xf) {
          selectedBlockIndex += 1;
          return;
        }

        if (released == PAD_B) {
          selectedWave.sounds[selectedBlockIndex].activate(lastNote, lastOctave);
        } else if (released == PAD_A) {
          auto sound = &selectedWave.sounds[selectedBlockIndex];
          if (sound->active) {
            clipboard = sound->cut();
            hasClipboard = true;
          } else if (hasClipboard) {
            selectedWave.sounds[selectedBlockIndex] = clipboard;
          }
        }
      }
      return;
    }
  }
};

BleepBloopMachine machine;

Adafruit_NeoPixel neopixels(NEOPIXEL_LENGTH, NEOPIXEL_PIN, NEO_GRB);

AudioConnection weq_patch(synth_q, env_q);
AudioConnection wep_patch(synth_p, env_p);
AudioConnection wes_patch(synth_s, env_s);
AudioConnection wen_patch(synth_n, env_n);

/* there are only monophonic sounds on this one so far :)
 * but this will need updated for:
 * - chords
 * - panning
 * - each instrument should perhaps have its own mixer?

AudioMixer4 q_mixer;
AudioMixer4 p_mixer;
AudioMixer4 s_mixer;
AudioMixer4 n_mixer;
*/

AudioMixer4 mixer;

AudioConnection emq_patch(env_q, 0, mixer, 0);
AudioConnection emp_patch(env_p, 0, mixer, 1);
AudioConnection ems_patch(env_s, 0, mixer, 2);
AudioConnection emn_patch(env_n, 0, mixer, 3);

AudioOutputAnalogStereo headphones;
AudioConnection left_ear_patch(mixer, 0, headphones, 0);
AudioConnection right_ear_patch(mixer, 0, headphones, 1);

uint16_t readLightSensor() { return analogRead(LIGHT_SENSOR); }

float readBatterySensor() {
  return ((float)analogRead(BATTERY_SENSOR) / 1023.0) * 2.0 * 3.3;
}

Controls controls;

void setup() {
  Serial.begin(115200);
  AudioMemory(18);
  controls.begin();
  display.initR(INITR_BLACKTAB);
  display.setRotation(1);
  pinMode(DISPLAY_BACKLIGHT, OUTPUT);
  digitalWrite(DISPLAY_BACKLIGHT, HIGH);
  display.fillScreen(ST77XX_WHITE);
  neopixels.begin();
  neopixels.setBrightness(180);
}

void loop() {
  uint32_t buttons = controls.readButtons();

  uint32_t justReleased = controls.justReleased();

  machine.handleButtons(buttons, justReleased);
  machine.step();
}
