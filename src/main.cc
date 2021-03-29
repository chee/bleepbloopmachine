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
#define DISPLAY_HEIGHT 128
#define DISPLAY_WIDTH 160

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

float wrapping_add(float cur, float max = 1.0, float inc = 0.1,
                   int min = -1.0) {
  auto nxt = cur + inc;
  return nxt <= max ? nxt : min;
}

float wrapping_sub(float cur, float max = 1.0, float inc = 0.1,
                   int min = -1.0) {
  auto nxt = cur - inc;
  return nxt >= min ? nxt : max;
}

float saturating_add(float cur, float max = 1.0, float inc = 0.1,
                     int min = -1.0) {
  auto nxt = cur + inc;
  return nxt <= max ? nxt : max;
}

float saturating_sub(float cur, float max = 1.0, float inc = 0.1,
                     int min = -1.0) {
  auto nxt = cur - inc;
  return nxt >= min ? nxt : min;
}

enum class MenuMode { live, menu1, menu2 };

enum class Menu1Selection { env, mod, filter, delay };

Menu1Selection &operator++(Menu1Selection &s) {
  return s = Menu1Selection{wrapping_add((int)s, 3)};
}

Menu1Selection &operator--(Menu1Selection &s) {
  return s = Menu1Selection{wrapping_sub((int)s, 3)};
}

enum class Menu2Selection { bpm, chain, wave, file };

Menu2Selection &operator++(Menu2Selection &s) {
  return s = Menu2Selection{wrapping_add((int)s, 3)};
}

Menu2Selection &operator--(Menu2Selection &s) {
  return s = Menu2Selection{wrapping_sub((int)s, 3)};
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

#define MAX_ATTACK 400.0
#define MAX_DECAY 400.0

class SoundBlock {
public:
  enum Filter { low };
  int note = 0;
  int octave = 3;
  bool active = false;
  float attack = 10.0;
  float decay = 40.0;
  float delay = 0.0;
  float pan = 0.0;
  float amp = 1.0;
  float sustain = 0.0;
  float filterType = Filter::low;
  float filterQ = 0.618;
  float filterFreq = 8000.0;
  int filterStage = 0;
  float pitchbend = 1.0;
  float pitchbendSpeed = 0.0025;
  float frequency() { return freq[note][octave]; }
  void activate(int n, int o) {
    note = n;
    octave = o;
    active = true;
  }
  void activate() { active = true; }
  void pitchbendUp() { pitchbend = saturating_sub(pitchbend, 10.0, 0.1, 0); }
  void pitchbendDown() {
    pitchbend = saturating_add(pitchbend, 10.0, 0.1, 0.0);
  }
  void pitchbendSpeedUp() {
    pitchbendSpeed = saturating_sub(pitchbendSpeed, 0.1, 0.0025, 0.0025);
  }
  void pitchbendSpeedDown() {
    pitchbendSpeed = saturating_add(pitchbendSpeed, 0.1, 0.0025, 0.0025);
  }
  void filterDown() {
    filterFreq = saturating_sub(filterFreq, 10000.0, 100.0, 40.0);
  }
  void filterUp() {
    filterFreq = saturating_add(filterFreq, 10000.0, 100.0, 40.0);
  }
  void filterQDown() {
    filterFreq = saturating_sub(filterFreq, 1.0, 0.0234, 0);
  }
  void filterQUp() { filterFreq = saturating_add(filterFreq, 1.0, 0.0234, 0); }
  void noteUp() { note = wrapping_add(note, 11); }
  void noteDown() { note = wrapping_sub(note, 11); }
  void octaveUp() { octave = wrapping_add(octave, 8); }
  void octaveDown() { octave = wrapping_sub(octave, 8); }
  void decayUp() { decay = saturating_add(decay, MAX_DECAY, 20.0, 0); }
  void decayDown() { decay = saturating_sub(decay, MAX_DECAY, 20.0, 0); }
  void delayUp() { delay = saturating_add(delay, 100.0, 10.0, 0); }
  void delayDown() { delay = saturating_sub(delay, 100.0, 10.0, 0); }
  void ampUp() { amp = saturating_add(amp, 1.0, 0.1, 0); }
  void ampDown() { amp = saturating_sub(amp, 1.0, 0.1, 0); }
  void panLeft() { pan = saturating_sub(pan, 1.0); }
  void panRight() { pan = saturating_add(pan, 1.0); }
  void attackUp() { attack = saturating_add(attack, MAX_ATTACK, 20.0, 0); }
  void attackDown() { attack = saturating_sub(attack, MAX_ATTACK, 20.0, 0); }
  void sustainUp() { sustain = saturating_add(sustain, 1.0, 0.137, 0); }
  void sustainDown() { sustain = saturating_sub(sustain, 1.0, 0.137, 0); }
  SoundBlock cut() {
    active = false;
    SoundBlock s;
    s.note = note;
    s.octave = octave;
    s.active = true;
    s.attack = attack;
    s.decay = decay;
    s.delay = delay;
    s.amp = amp;
    s.pan = pan;
    s.sustain = sustain;
    s.filterFreq = filterFreq;
    s.filterQ = filterQ;
    s.filterType = filterType;
    s.pitchbendSpeed = pitchbendSpeed;
    s.pitchbend = pitchbend;
    return s;
  }
  SoundBlock() {}
};

AudioMixer4 mainmixerL;
AudioMixer4 mainmixerR;
AudioOutputAnalogStereo headphones;
AudioConnection left_ear_patch(mainmixerL, 0, headphones, 0);
AudioConnection right_ear_patch(mainmixerR, 0, headphones, 1);

class Wave {
  enum PitchbendState { idle, inc, dec };
  PitchbendState pitchbendState = PitchbendState::idle;
  float pitchbendSpeed = 0.1;
  float pitchbend = 1.0;
  float prebendFreq = 0.0;
  void pitchbendStart() {
    if (pitchbend > 1.0) {
      pitchbendState = PitchbendState::dec;
    } else if (pitchbend < 1.0) {
      pitchbendState = PitchbendState::inc;
    } else {
      pitchbendState = PitchbendState::idle;
    }
    synthL.frequency(prebendFreq * pitchbend);
    synthR.frequency(prebendFreq * pitchbend);
  }
  AudioConnection *patches[7];

public:
  void pitchbendUpdate() {
    if (pitchbendState == PitchbendState::idle) {
      return;
    }
    if (pitchbendState == PitchbendState::dec) {
      pitchbend -= pitchbendSpeed;
      if (pitchbend <= 1.0) {
        pitchbendState = PitchbendState::idle;
      }
    }
    if (pitchbendState == PitchbendState::inc) {
      pitchbend += pitchbendSpeed;
      if (pitchbend >= 1.0) {
        pitchbendState = PitchbendState::idle;
      }
    }
    synthL.frequency(prebendFreq * pitchbend);
    synthR.frequency(prebendFreq * pitchbend);
  }
  Waveform form;
  unsigned char name;
  SoundBlock sounds[16];
  AudioSynthWaveform synthL = AudioSynthWaveform();
  AudioSynthWaveform synthR = AudioSynthWaveform();
  AudioEffectEnvelope envL = AudioEffectEnvelope();
  AudioEffectEnvelope envR = AudioEffectEnvelope();
  AudioFilterBiquad filterL = AudioFilterBiquad();
  AudioFilterBiquad filterR = AudioFilterBiquad();
  AudioMixer4 mixerL = AudioMixer4();
  AudioMixer4 mixerR = AudioMixer4();
  int bpmDivider = 1;
  int lastStepBlockIndex = 0;
  void bpmDividerUp() {
    switch (bpmDivider) {
    case 4:
      bpmDivider = 2;
      break;
    case 2:
      bpmDivider = 1;
      break;
    }
  }
  void bpmDividerDown() {
    switch (bpmDivider) {
    case 2:
      bpmDivider = 4;
      break;
    case 1:
      bpmDivider = 2;
      break;
    }
  }
  Wave(unsigned char n, Waveform f, int mixerIndex) : name{n}, form{f} {
    // memory leaks
    new AudioConnection(synthL, envL);
    new AudioConnection(envL, filterL);
    new AudioConnection(filterL, 0, mixerL, 0);
    new AudioConnection(synthR, envR);
    new AudioConnection(envR, filterR);
    new AudioConnection(filterR, 0, mixerR, 0);
    new AudioConnection(mixerL, 0, mainmixerL, mixerIndex);
    new AudioConnection(mixerR, 0, mainmixerR, mixerIndex);
    for (auto i = 0; i < 16; i++) {
      sounds[i] = SoundBlock();
    }
    filterL.setLowpass(0, 8000);
    filterR.setLowpass(0, 8000);
    synthL.begin((int)form);
    synthR.begin((int)form);
    synthL.amplitude(1.0);
    synthR.amplitude(1.0);
    envL.sustain(0.0);
    envR.sustain(0.0);
    if (form == Waveform::pulse) {
      synthL.pulseWidth(0.5);
      synthR.pulseWidth(0.5);
    }
  }
  int stepBlockIndex(int currentStep) { return currentStep / bpmDivider % 16; }
  void play(int blockIndex) {
    auto sound = &sounds[blockIndex];
    if (!sound->active) {
      return;
    }
    prebendFreq = sound->frequency();
    synthL.frequency(prebendFreq);
    synthR.frequency(prebendFreq);
    envL.attack(sound->attack);
    envL.decay(sound->decay);
    envL.delay(sound->delay);
    envL.sustain(sound->sustain);
    envR.attack(sound->attack);
    envR.decay(sound->decay);
    envR.delay(sound->delay);
    envR.sustain(sound->sustain);
    mixerL.gain(0, sound->amp / 2);
    mixerR.gain(0, sound->amp / 2);
    if (sound->filterType == SoundBlock::Filter::low) {
      filterL.setLowpass(sound->filterStage, sound->filterFreq, sound->filterQ);
      filterR.setLowpass(sound->filterStage, sound->filterFreq, sound->filterQ);
    }
    if (sound->pan < 0) {
      mixerL.gain(0, (sound->amp / 2) + (abs(sound->pan) / 2));
      mixerR.gain(0, (sound->amp / 2) - (abs(sound->pan) / 2));
    } else if (sound->pan > 0) {
      mixerR.gain(0, (sound->amp / 2) + (abs(sound->pan) / 2));
      mixerL.gain(0, (sound->amp / 2) - (abs(sound->pan) / 2));
    }
    pitchbendSpeed = sound->pitchbendSpeed;
    pitchbend = sound->pitchbend;
    pitchbendStart();
    envL.noteOn();
    envR.noteOn();
  }
  void step(int currentTick) {
    auto idx = stepBlockIndex(currentTick);
    if (idx != lastStepBlockIndex) {
      lastStepBlockIndex = idx;
      play(idx);
    }
  }
  void updateDisplay(Adafruit_GFX *display, MenuMode mode,
                     Menu1Selection menu1control, Menu2Selection menu2control) {
    for (int i = 0; i <= 0xf; i++) {
      auto sound = &sounds[i];
      Point point = getCoord(i);
      if (!sound->active)
        continue;
      String note = note_names[sound->note];
      display->drawChar(point.x + 2, point.y + 2, note[0], ST7735_BLACK,
                        ST7735_BLACK, 1);
      if (note.length() == 2) {
        display->drawChar(point.x + 9, point.y + 2, '#', ST7735_BLACK,
                          ST7735_BLACK, 1);
      }
      display->fillRect(point.x + (sound->octave * 2), point.y, 2, 2,
                        sound->octave * 100);
      if (mode == MenuMode::live) {
        // only note name and octave
      } else if (mode == MenuMode::menu1) {
        if (menu1control == Menu1Selection::env) {
          // draw panning
          auto panX = ((BLOCK_SIZE / 2) * sound->pan) + BLOCK_SIZE / 2;
          display->fillRect(point.x + panX, point.y + BLOCK_SIZE - 2, 2, 2,
                            ST7735_CYAN);

          // draw attack/decay/amp
          /// perhaps unexpected is that amplitude is represented by the height
          /// of the envelope, sustain being the bar on the right

          auto envColor = ST7735_GREEN;
          float attackX = ((float)BLOCK_SIZE / 2 / MAX_ATTACK) * sound->attack;
          float decayW = ((float)BLOCK_SIZE / 2 / MAX_DECAY) * sound->decay;

          /// attack/amp
          display->drawLine(point.x, point.y + BLOCK_SIZE, point.x + attackX,
                            (point.y + BLOCK_SIZE) - BLOCK_SIZE * sound->amp,
                            envColor);
          /// decay/amp
          display->drawLine(point.x + attackX,
                            (point.y + BLOCK_SIZE) - BLOCK_SIZE * sound->amp,
                            point.x + attackX + decayW, (point.y + BLOCK_SIZE),
                            envColor);

          // draw sustain
          auto susY = BLOCK_SIZE * sound->sustain;
          display->fillRect(point.x + BLOCK_SIZE - 4,
                            point.y + (BLOCK_SIZE - susY), 4, susY,
                            ST7735_CYAN);
        }
      } else if (mode == MenuMode::menu2) {
          display->drawPixel(point.x + 5, point.y + 5, ST7735_BLACK);
      }
    }
    display->drawChar(10, 10, name, ST77XX_BLACK, ST7735_BLACK, 1);
  }
};

Wave wave_q('q', Waveform::square, 0);
Wave wave_p('p', Waveform::pulse, 1);
Wave wave_s('s', Waveform::sawtooth, 2);
Wave wave_n('n', Waveform::sampleHold, 3);
Wave *waves[4] = {&wave_q, &wave_p, &wave_s, &wave_n};
Adafruit_NeoPixel neopixels(NEOPIXEL_LENGTH, NEOPIXEL_PIN, NEO_GRB);

class BleepBloopMachine {
public:
  MenuMode mode = MenuMode::live;
  float bpm = 120.0;
  long lastMicros = micros();
  // TODO ask abe how to write this without brackets
  long stepLength() { return ((1 / bpm) / 4) * 60000000; }
  int selectedSoundIndex = 0;
  Menu1Selection selectedMenu1Control{0};
  Menu2Selection selectedMenu2Control{0};
  int selectedWaveIndex = 0;
  int selectedBlockIndex = 0;
  int lastSelectedBlockIndex = 0;
  int currentStep = 0;
  int lastNote = 0;
  int lastOctave = 3;
  SoundBlock clipboard;
  bool hasClipboard = false;
  int ignoreRelease = 0;
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
      updateDisplay();
      for (auto i = 0; i < 4; i++) {
        int bar = (int)(currentStep / 16);
        neopixels.setPixelColor(i, i == bar ? neopixels.Color(100, 20, 30)
                                            : neopixels.Color(0, 0, 200));
      }
      neopixels.show();
    }
    wave_q.pitchbendUpdate();
    wave_p.pitchbendUpdate();
    wave_s.pitchbendUpdate();
    wave_n.pitchbendUpdate();
  }
  Wave *selectedWave() { return waves[selectedWaveIndex]; }
  void waveUp() {
    selectedWaveIndex = wrapping_sub(selectedWaveIndex, 3);
    display.fillScreen(ST7735_WHITE);
  }
  void waveDown() {
    selectedWaveIndex = wrapping_add(selectedWaveIndex, 3);
    display.fillScreen(ST7735_WHITE);
  }
  void drawMenu(String choices, int selectedControl, int background) {
    if (choices.length() != 4) {
      Serial.println("choices must be 4 chars");
      return;
    }
    auto y = 110;
    for (auto i = 0; i <= 3; i++) {
      auto color = i == selectedControl ? ST7735_BLACK : ST7735_WHITE;
      auto x = GRID_OFFSET_X + (i * (BLOCK_SIZE + GRID_GAP));
      display.fillRect(x, y, BLOCK_SIZE, BLOCK_SIZE, background);
      display.drawRect(x, y, BLOCK_SIZE, BLOCK_SIZE, color);
    }

    auto ty = y + (BLOCK_SIZE / 2) - 2;
    display.drawChar(8 + GRID_OFFSET_X, ty, choices[0], ST7735_BLACK,
                     ST7735_BLACK, 1);
    display.drawChar(8 + GRID_OFFSET_X + 1 * (BLOCK_SIZE + GRID_GAP), ty,
                     choices[1], ST7735_BLACK, ST7735_BLACK, 1);
    display.drawChar(8 + GRID_OFFSET_X + 2 * (BLOCK_SIZE + GRID_GAP), ty,
                     choices[2], ST7735_BLACK, ST7735_BLACK, 1);
    display.drawChar(8 + GRID_OFFSET_X + 3 * (BLOCK_SIZE + GRID_GAP), ty,
                     choices[3], ST7735_BLACK, ST7735_BLACK, 1);
  }
  void drawMenu(String choices, Menu1Selection selectedControl) {
    drawMenu(choices, (int)selectedControl, 0xffee);
  }
  void drawMenu(String choices, Menu2Selection selectedControl) {
    drawMenu(choices, (int)selectedControl, 0xeeff);
  }
  void updateDisplay() {
    auto stepBlockIndex = selectedWave()->stepBlockIndex(currentStep);
    // first comes blocks
    for (auto i = 0; i <= 0xf; i++) {
      auto coord = getCoord(i);
      auto color =
          stepBlockIndex == i ? TICK_BLOCK_COLOR : STANDARD_BLOCK_COLOR;
      display.fillRect(coord.x, coord.y, BLOCK_SIZE, BLOCK_SIZE, color);
    }
    // then comes selection
    auto selectCoord = getCoord(selectedBlockIndex);
    auto selectColor = mode == MenuMode::live ? ST7735_BLACK : TICK_BLOCK_COLOR;
    display.drawRect(selectCoord.x, selectCoord.y, BLOCK_SIZE, BLOCK_SIZE,
                     ST7735_BLACK);

    // then the labels
    selectedWave()->updateDisplay(&display, mode, selectedMenu1Control,
                                  selectedMenu2Control);
    String bpmString = String(bpm);
    display.drawChar(DISPLAY_WIDTH - 20, 10, bpmString[0], ST7735_BLACK,
                     ST7735_BLACK, 1);
    display.drawChar(DISPLAY_WIDTH - 15, 10, bpmString[1], ST7735_BLACK,
                     ST7735_BLACK, 1);
    display.drawChar(DISPLAY_WIDTH - 10, 10, bpmString[2], ST7735_BLACK,
                       ST7735_BLACK, 1);

    if (selectedWave()->bpmDivider != 1) {
      display.drawChar(DISPLAY_WIDTH - 15, 20, '/', ST7735_BLACK, ST7735_BLACK,
                       1);
      display.drawChar(DISPLAY_WIDTH - 10, 20,
                       String(selectedWave()->bpmDivider)[0], ST7735_BLACK,
                       ST7735_BLACK, 1);
    }

    // and the menus
    if (mode == MenuMode::menu1) {
      drawMenu("emfd", selectedMenu1Control);
    } else if (mode == MenuMode::menu2) {
      drawMenu("bwcf", selectedMenu2Control);
    }
  }
  void menu(MenuMode m) {
    mode = m;
    display.fillScreen(ST7735_WHITE);
  }
  void handleButtons(uint32_t held, uint32_t released, float joyX, float joyY) {
    bool ab_mode = held & PAD_A && held & PAD_B;
    bool a_mode = !ab_mode && held & PAD_A;
    bool b_mode = !ab_mode && held & PAD_B;
    bool start_mode = !ab_mode && !a_mode && !b_mode && held & PAD_START;
    bool none_mode = !ab_mode && !a_mode && !b_mode && !start_mode;
    auto sound = &selectedWave()->sounds[selectedBlockIndex];

    if (b_mode && released) {
      ignoreRelease |= PAD_B;
    }

    if (a_mode && released) {
      ignoreRelease |= PAD_A;
    }

    if (start_mode && released) {
      ignoreRelease |= PAD_START;
    }

    switch (mode) {
    case MenuMode::menu1:
      if (start_mode) {
        if (released & PAD_UP && selectedBlockIndex > 0x3) {
          selectedBlockIndex -= 4;
        }
        if (released & PAD_DOWN && selectedBlockIndex < 0xc) {
          selectedBlockIndex += 4;
        }
        if (released & PAD_LEFT && selectedBlockIndex > 0x0) {
          selectedBlockIndex -= 1;
        }
        if (released & PAD_RIGHT && selectedBlockIndex < 0xf) {
          selectedBlockIndex += 1;
        }
      } else if (none_mode) {
        if (released & PAD_RIGHT) {
          selectedMenu1Control = ++selectedMenu1Control;
        }
        if (released & PAD_LEFT) {
          selectedMenu1Control = --selectedMenu1Control;
        }
        if (released & PAD_DOWN) {
          waveDown();
        }
        if (released & PAD_UP) {
          waveUp();
        }
        if (released == PAD_START) {
          if (!(ignoreRelease & PAD_START)) {
            menu(MenuMode::menu2);
          }
        }
        if (released == PAD_B) {
          if (!(ignoreRelease & PAD_B)) {
            menu(MenuMode::live);
          }
        }
      }
      if (selectedMenu1Control == Menu1Selection::env) {
        if (b_mode) {
          if (released & PAD_LEFT) {
            sound->attackDown();
          }
          if (released & PAD_RIGHT) {
            sound->attackUp();
          }
          if (released & PAD_UP) {
            sound->sustainUp();
          }
          if (released & PAD_DOWN) {
            sound->sustainDown();
          }
        }
        if (a_mode) {
          if (released & PAD_RIGHT) {
            sound->decayUp();
          }
          if (released & PAD_LEFT) {
            sound->decayDown();
          }
          if (released & PAD_UP) {
            sound->sustainUp();
          }
          if (released & PAD_DOWN) {
            sound->sustainDown();
          }
        }
        if (ab_mode) {
          if (released & PAD_LEFT) {
            sound->panLeft();
          }
          if (released & PAD_RIGHT) {
            sound->panRight();
          }
          if (released & PAD_UP) {
            sound->ampUp();
          }
          if (released & PAD_DOWN) {
            sound->ampDown();
          }
        }
      } else if (selectedMenu1Control == Menu1Selection::filter) {
        if (b_mode) {
          if (released & PAD_LEFT) {
            sound->filterDown();
          }
          if (released & PAD_RIGHT) {
            sound->filterUp();
          }
          if (released & PAD_UP) {
            sound->filterQUp();
          }
          if (released & PAD_DOWN) {
            sound->filterQDown();
          }
        } else if (ab_mode) {
          for (auto i = 0; i <= 0xf; i++) {
            selectedWave()->sounds[i].filterFreq =
                8000.0 * ((joyX + 512) / 1024);
            selectedWave()->sounds[i].filterQ = (joyY + 512) / 1024;
          }
        }
      } else if (selectedMenu1Control == Menu1Selection::mod) {
        if (b_mode) {
          if (released & PAD_LEFT) {
            sound->pitchbendSpeedUp();
          }
          if (released & PAD_RIGHT) {
            sound->pitchbendSpeedDown();
          }
          if (released & PAD_UP) {
            sound->pitchbendUp();
          }
          if (released & PAD_DOWN) {
            sound->pitchbendDown();
          }
        }
      } else if (selectedMenu1Control == Menu1Selection::delay) {
        if (b_mode) {
          if (released & PAD_LEFT) {
            // sound->delayUp();
          }
          if (released & PAD_RIGHT) {
            //  sound->pitchbendSpeedUp();
          }
          if (released & PAD_UP) {
            sound->delayUp();
          }
          if (released & PAD_DOWN) {
            sound->delayDown();
          }
        }
      }
      break;
    case MenuMode::menu2:
      if (start_mode) {
        if (released & PAD_UP && selectedBlockIndex > 0x3) {
          selectedBlockIndex -= 4;
        }
        if (released & PAD_DOWN && selectedBlockIndex < 0xc) {
          selectedBlockIndex += 4;
        }
        if (released & PAD_LEFT && selectedBlockIndex > 0x0) {
          selectedBlockIndex -= 1;
        }
        if (released & PAD_RIGHT && selectedBlockIndex < 0xf) {
          selectedBlockIndex += 1;
        }
      } else if (none_mode) {
        if (released & PAD_DOWN) {
          waveDown();
        }
        if (released & PAD_UP) {
          waveUp();
        }
        if (released & PAD_RIGHT) {
          selectedMenu2Control = ++selectedMenu2Control;
        }
        if (released & PAD_LEFT) {
          selectedMenu2Control = --selectedMenu2Control;
        }
        if (released == PAD_START) {
          if (!(ignoreRelease & PAD_START)) {
            menu(MenuMode::menu1);
          }
        }
        if (released == PAD_B) {
          if (!(ignoreRelease & PAD_B)) {
            menu(MenuMode::live);
          }
        }
      }
      if (selectedMenu2Control == Menu2Selection::bpm) {
        if (b_mode) {
          if (released & PAD_LEFT) {
            bpm = saturating_sub(bpm, 240.0, 20.0, 40.0);
            display.fillScreen(ST7735_WHITE);
          }
          if (released & PAD_RIGHT) {
            bpm = saturating_add(bpm, 240.0, 20.0, 40.0);
            display.fillScreen(ST7735_WHITE);
          }
          if (released & PAD_DOWN) {
            selectedWave()->bpmDividerDown();
            display.fillScreen(ST7735_WHITE);
          }
          if (released & PAD_UP) {
            selectedWave()->bpmDividerUp();
            display.fillScreen(ST7735_WHITE);
          }
        }
      }
      break;
    case MenuMode::live:
      if (b_mode) {

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
        }
        if (released & PAD_DOWN && selectedBlockIndex < 0xc) {
          selectedBlockIndex += 4;
        }
        if (released & PAD_LEFT && selectedBlockIndex > 0x0) {
          selectedBlockIndex -= 1;
        }
        if (released & PAD_RIGHT && selectedBlockIndex < 0xf) {
          selectedBlockIndex += 1;
        }

        if (released == PAD_B) {
          sound->activate(lastNote, lastOctave);
        } else if (released == PAD_A) {
          if (sound->active) {
            // TODO this can be done by assigning it to a variable w/o using &
            clipboard = sound->cut();
            hasClipboard = true;
          } else if (hasClipboard) {
            selectedWave()->sounds[selectedBlockIndex] = clipboard;
          }
        }
        if (released == PAD_START) {
          if (!(ignoreRelease & PAD_START)) {
            menu(MenuMode::menu1);
          }
        }
      } else if (start_mode) {
        if (released == PAD_DOWN) {
          waveDown();
        } else if (released == PAD_UP) {
          waveUp();
        } else if (released == PAD_LEFT) {
          // chainLeft();
        } else if (released == PAD_RIGHT) {
          // chainRight();
        }
        break;
      }
    }
    if (released == PAD_B && ignoreRelease & PAD_B) {
      ignoreRelease ^= PAD_B;
    }
    if (released == PAD_A && ignoreRelease & PAD_A) {
      ignoreRelease ^= PAD_A;
    }
    if (released == PAD_START && ignoreRelease & PAD_START) {
      ignoreRelease ^= PAD_START;
    }
  }
  };

BleepBloopMachine machine;

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
  neopixels.setBrightness(10);
  neopixels.fill(ST7735_CYAN, 0, 4);
  neopixels.show();
}

void loop() {
  uint32_t buttons = controls.readButtons();
  uint32_t justReleased = controls.justReleased();
  float joyX = controls.readJoystickX();
  float joyY = controls.readJoystickY();

  machine.handleButtons(buttons, justReleased, joyX, joyX);
  machine.step();
}
