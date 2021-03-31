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

#define KEYLAYER_0 0
#define KEYLAYER_1 1
#define KEYLAYER_2 2
#define KEYLAYER_3 3
#define KEYLAYER_4 4
#define KEYLAYER_5 5

enum class Keymod { a, b, ab, select, start, none };

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

enum class Menu1Control { env, mod, filter, delay };

Menu1Control &operator++(Menu1Control &s) {
  return s = Menu1Control{wrapping_add((int)s, 3)};
}

Menu1Control &operator--(Menu1Control &s) {
  return s = Menu1Control{wrapping_sub((int)s, 3)};
}

enum class Menu2Control { bpm, chain, wave, file };

Menu2Control &operator++(Menu2Control &s) {
  return s = Menu2Control{wrapping_add((int)s, 3)};
}

Menu2Control &operator--(Menu2Control &s) {
  return s = Menu2Control{wrapping_sub((int)s, 3)};
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

class Block {};

class SoundBlock : public Block {
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
  float pulseWidth = 0.5;
  float frequency() { return freq[note][octave]; }
  void activate(int n, int o) {
    note = n;
    octave = o;
    active = true;
  }
  void activate() { active = true; }
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
  void pulseWidthUp() { pulseWidth = saturating_add(pulseWidth, 1.0, 0.1, 0); }
  void pulseWidthDown() {
    pulseWidth = saturating_sub(pulseWidth, 1.0, 0.1, 0);
  }
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
    s.pulseWidth = pulseWidth;
    return s;
  }
  SoundBlock() {}
};

SoundBlock clipboard;
bool hasClipboard = false;

AudioMixer4 mainmixerL;
AudioMixer4 mainmixerR;
AudioOutputAnalogStereo headphones;
AudioConnection left_ear_patch(mainmixerL, 0, headphones, 0);
AudioConnection right_ear_patch(mainmixerR, 0, headphones, 1);

class Track {
protected:
  int lastStepBlockIndex = 0;

public:
  int bpmDivider = 1;
  unsigned char name;
  Block blocks[16];
  int stepBlockIndex(int currentStep) { return currentStep / bpmDivider % 16; }
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
  virtual void updateDisplay(Adafruit_GFX *display, MenuMode mode,
                             Menu1Control menu1control,
                             Menu2Control menu2control) = 0;
  virtual void handle(Menu1Control control, Keymod modifier, uint32_t pressed,
                      int blockIndex) = 0;
  virtual void handle(Menu2Control control, Keymod modifier, uint32_t pressed,
                      int blockIndex) = 0;
  virtual void handle(Keymod modifier, uint32_t pressed, int blockIndex) = 0;
  virtual void step(int currentTick) = 0;
};

class Wave : public Track {
  AudioConnection *patches[7];

public:
  Waveform form;
  AudioSynthWaveform *synthL = new AudioSynthWaveform();
  AudioSynthWaveform *synthR = new AudioSynthWaveform();
  AudioEffectEnvelope *envL = new AudioEffectEnvelope();
  AudioEffectEnvelope *envR = new AudioEffectEnvelope();
  AudioFilterBiquad *filterL = new AudioFilterBiquad();
  AudioFilterBiquad *filterR = new AudioFilterBiquad();
  AudioMixer4 *mixerL = new AudioMixer4();
  AudioMixer4 *mixerR = new AudioMixer4();
  SoundBlock *blocks = new SoundBlock[16];
  Wave(unsigned char n, Waveform f, int mixerIndex) {
    form = f;
    name = n;
    patches[0] = new AudioConnection(*synthL, *envL);
    patches[1] = new AudioConnection(*envL, *filterL);
    patches[2] = new AudioConnection(*filterL, 0, *mixerL, 0);
    patches[3] = new AudioConnection(*synthR, *envR);
    patches[4] = new AudioConnection(*envR, *filterR);
    patches[5] = new AudioConnection(*filterR, 0, *mixerR, 0);
    patches[6] = new AudioConnection(*mixerL, 0, mainmixerL, mixerIndex);
    patches[7] = new AudioConnection(*mixerR, 0, mainmixerR, mixerIndex);
    filterL->setLowpass(0, 12000);
    filterR->setLowpass(0, 12000);
    synthL->begin((int)f);
    synthR->begin((int)f);
    synthL->amplitude(0.4);
    synthR->amplitude(0.4);
    envL->sustain(1.0);
    envR->sustain(1.0);
    if (f == Waveform::pulse) {
      synthL->pulseWidth(0.5);
      synthR->pulseWidth(0.5);
    }
  }
  ~Wave() {
    // TODO  delete pointers
    delete[] patches;
  }
  void play(int blockIndex) {
    auto sound = &blocks[blockIndex];
    if (!sound->active) {
      return;
    }
    synthL->frequency(sound->frequency());
    synthR->frequency(sound->frequency());
    envL->attack(sound->attack);
    envL->decay(sound->decay);
    envL->delay(sound->delay);
    envL->sustain(sound->sustain);
    envR->attack(sound->attack);
    envR->decay(sound->decay);
    envR->delay(sound->delay);
    envR->sustain(sound->sustain);
    mixerL->gain(0, sound->amp / 2);
    mixerR->gain(0, sound->amp / 2);
    if (sound->filterType == SoundBlock::Filter::low) {
      filterL->setLowpass(sound->filterStage, sound->filterFreq,
                          sound->filterQ);
      filterR->setLowpass(sound->filterStage, sound->filterFreq,
                          sound->filterQ);
    }
    if (sound->pan < 0) {
      mixerL->gain(0, (sound->amp / 2) + (abs(sound->pan) / 2));
      mixerR->gain(0, (sound->amp / 2) - (abs(sound->pan) / 2));
    } else if (sound->pan > 0) {
      mixerR->gain(0, (sound->amp / 2) + (abs(sound->pan) / 2));
      mixerL->gain(0, (sound->amp / 2) - (abs(sound->pan) / 2));
    }
    if (form == Waveform::pulse) {
      synthL->pulseWidth(sound->pulseWidth);
      synthR->pulseWidth(sound->pulseWidth);
    }
    envL->noteOn();
    envR->noteOn();
  }
  void step(int currentStep) {
    auto idx = stepBlockIndex(currentStep);
    if (idx != lastStepBlockIndex) {
      lastStepBlockIndex = idx;
      play(idx);
    }
  }
  void updateDisplay(Adafruit_GFX *display, MenuMode mode,
                     Menu1Control menu1control, Menu2Control menu2control) {
    for (int i = 0; i <= 0xf; i++) {
      auto sound = &blocks[i];
      Point point = getCoord(i);
      if (!sound->active)
        continue;
      String note = note_names[sound->note];
      display->fillRect(point.x + (sound->octave * 2), point.y, 2, 5,
                        sound->octave * 360);
      display->drawChar(point.x + 2, point.y + 5, note[0], ST7735_BLACK,
                        ST7735_BLACK, 1);
      if (note.length() == 2) {
        display->drawChar(point.x + 9, point.y + 5, '#', ST7735_BLACK,
                          ST7735_BLACK, 1);
      }
      if (mode == MenuMode::live) {
        // only note name and octave
      } else if (mode == MenuMode::menu1) {
        if (menu1control == Menu1Control::env) {
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
  void handle(Menu1Control control, Keymod modifier, uint32_t pressed,
              int blockIndex) {
    auto sound = &blocks[blockIndex];
    if (control == Menu1Control::env) {
      if (modifier == Keymod::a) {
        if (pressed & PAD_RIGHT) {
          sound->decayUp();
        }
        if (pressed & PAD_LEFT) {
          sound->decayDown();
        }
        if (pressed & PAD_UP) {
          sound->sustainUp();
        }
        if (pressed & PAD_DOWN) {
          sound->sustainDown();
        }
      }
      if (modifier == Keymod::b) {
        if (pressed & PAD_LEFT) {
          sound->attackDown();
        }
        if (pressed & PAD_RIGHT) {
          sound->attackUp();
        }
        if (pressed & PAD_UP) {
          sound->sustainUp();
        }
        if (pressed & PAD_DOWN) {
          sound->sustainDown();
        }
      }
      if (modifier == Keymod::ab) {
        if (pressed & PAD_LEFT) {
          sound->panLeft();
        }
        if (pressed & PAD_RIGHT) {
          sound->panRight();
        }
        if (pressed & PAD_UP) {
          sound->ampUp();
        }
        if (pressed & PAD_DOWN) {
          sound->ampDown();
        }
      }
    } else if (control == Menu1Control::filter) {
      if (modifier == Keymod::a) {
      } else if (modifier == Keymod::b) {
        if (pressed & PAD_LEFT) {
          sound->filterDown();
        }
        if (pressed & PAD_RIGHT) {
          sound->filterUp();
        }
        if (pressed & PAD_UP) {
          sound->filterQUp();
        }
        if (pressed & PAD_DOWN) {
          sound->filterQDown();
        }
      }
    } else if (control == Menu1Control::mod) {
      if (modifier == Keymod::a) {
      } else if (modifier == Keymod::b) {
        if (pressed & PAD_UP) {
          if (form == Waveform::pulse) {
            sound->pulseWidthUp();
          }
        }
        if (pressed & PAD_DOWN) {
          if (form == Waveform::pulse) {
            sound->pulseWidthDown();
          }
        }
      }
    } else if (control == Menu1Control::delay) {
      if (modifier == Keymod::a) {
      } else if (modifier == Keymod::b) {
        if (pressed & PAD_UP) {
          sound->delayUp();
        }
        if (pressed & PAD_DOWN) {
          sound->delayDown();
        }
      }
    }
  }
  void handle(Menu2Control control, Keymod modifier, uint32_t pressed,
              int blockIndex) {}
  int lastNote = 0;
  int lastOctave = 3;
  void handle(Keymod modifier, uint32_t pressed, int blockIndex) {
    auto sound = &blocks[blockIndex];
    if (modifier == Keymod::b) {
      if (pressed & PAD_UP) {
        sound->noteUp();
      }
      if (pressed & PAD_DOWN) {
        sound->noteDown();
      }
      if (pressed & PAD_LEFT) {
        sound->octaveDown();
      }
      if (pressed & PAD_RIGHT) {
        sound->octaveUp();
      }
      lastNote = sound->note;
      lastOctave = sound->octave;
    } else if (modifier == Keymod::none) {
      if (pressed == PAD_B) {
        sound->activate(lastNote, lastOctave);
      } else if (pressed == PAD_A) {
        if (sound->active) {
          clipboard = sound->cut();
          hasClipboard = true;
        } else if (hasClipboard) {
          blocks[blockIndex] = clipboard;
        }
      }
    }
  }
};

Adafruit_NeoPixel neopixels(NEOPIXEL_LENGTH, NEOPIXEL_PIN, NEO_GRB);

#define NUMBER_OF_TRACKS 4
class BleepBloopMachine {
public:
  MenuMode mode = MenuMode::live;
  float bpm = 120.0;
  long lastMicros = micros();
  // TODO ask abe how to write this without brackets
  long stepLength() { return ((1 / bpm) / 4) * 60000000; }
  int selectedSoundIndex = 0;
  Menu1Control selectedMenu1Control{0};
  Menu2Control selectedMenu2Control{0};
  int selectedWaveIndex = 0;
  int selectedBlockIndex = 0;
  int lastSelectedBlockIndex = 0;
  int currentStep = 0;

  SoundBlock clipboard;
  bool hasClipboard = false;
  int ignoreRelease = 0;
  Track *tracks[NUMBER_OF_TRACKS];
  void begin() {
    tracks[0] = new Wave('q', Waveform::square, 0);
    tracks[1] = new Wave('p', Waveform::pulse, 1);
    tracks[2] = new Wave('s', Waveform::sawtooth, 2);
    tracks[3] = new Wave('n', Waveform::sampleHold, 3);
  }
  void step() {
    auto m = micros();
    if (m - lastMicros >= stepLength()) {
      currentStep += 1;
      if (currentStep > 63) {
        currentStep = 0;
      }
      lastMicros = m;
      for (auto i = 0; i < NUMBER_OF_TRACKS; i++) {
        tracks[i]->step(currentStep);
      }
      updateDisplay();
      for (auto i = 0; i < 4; i++) {
        int bar = (int)(currentStep / 16);
        neopixels.setPixelColor(i, i == bar ? neopixels.Color(100, 20, 30)
                                            : neopixels.Color(0, 0, 200));
      }
      neopixels.show();
    }
  }
  Track *selectedTrack() { return tracks[selectedWaveIndex]; }
  void trackUp() {
    selectedWaveIndex = wrapping_sub(selectedWaveIndex, NUMBER_OF_TRACKS - 1);
    display.fillScreen(ST7735_WHITE);
  }
  void trackDown() {
    // TODO only blank out the location of the track char
    selectedWaveIndex = wrapping_add(selectedWaveIndex, NUMBER_OF_TRACKS - 1);
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
  void drawMenu(String choices, Menu1Control selectedControl) {
    drawMenu(choices, (int)selectedControl, 0xffee);
  }
  void drawMenu(String choices, Menu2Control selectedControl) {
    drawMenu(choices, (int)selectedControl, 0xeeff);
  }
  void updateDisplay() {
    auto stepBlockIndex = selectedTrack()->stepBlockIndex(currentStep);
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
    selectedTrack()->updateDisplay(&display, mode, selectedMenu1Control,
                                   selectedMenu2Control);
    String bpmString = String(bpm);
    display.drawChar(DISPLAY_WIDTH - 20, 10, bpmString[0], ST7735_BLACK,
                     ST7735_BLACK, 1);
    display.drawChar(DISPLAY_WIDTH - 15, 10, bpmString[1], ST7735_BLACK,
                     ST7735_BLACK, 1);
    display.drawChar(DISPLAY_WIDTH - 10, 10, bpmString[2], ST7735_BLACK,
                     ST7735_BLACK, 1);

    if (selectedTrack()->bpmDivider != 1) {
      display.drawChar(DISPLAY_WIDTH - 15, 20, '/', ST7735_BLACK, ST7735_BLACK,
                       1);
      display.drawChar(DISPLAY_WIDTH - 10, 20,
                       String(selectedTrack()->bpmDivider)[0], ST7735_BLACK,
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
    bool select_mode = !ab_mode && !a_mode && !b_mode && held & PAD_SELECT;
    bool none_mode = !ab_mode && !a_mode && !b_mode && !start_mode;
    Keymod modifier = ab_mode       ? Keymod::ab
                      : a_mode      ? Keymod::a
                      : b_mode      ? Keymod::b
                      : start_mode  ? Keymod::start
                      : select_mode ? Keymod::select
                                    : Keymod::none;

    if (b_mode && released) {
      ignoreRelease |= PAD_B;
    }

    if (a_mode && released) {
      ignoreRelease |= PAD_A;
    }

    if (ab_mode && released) {
      ignoreRelease |= PAD_B;
      ignoreRelease |= PAD_A;
    }

    if (start_mode && released) {
      ignoreRelease |= PAD_START;
    }

    if (select_mode && released) {
      ignoreRelease |= PAD_SELECT;
    }

    uint32_t pressed = released ^ ignoreRelease;

    // first up: facts of life
    if (mode == MenuMode::live) {
      if (none_mode) {
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
        if (pressed == PAD_START) {
          menu(MenuMode::menu1);
        }
      } else if (start_mode) {
        if (released == PAD_DOWN) {
          trackDown();
        } else if (released == PAD_UP) {
          trackUp();
        } else if (released == PAD_LEFT) {
          // chainLeft();
        } else if (released == PAD_RIGHT) {
          // chainRight();
        }
      }
    } else {
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
          trackDown();
        }
        if (released & PAD_UP) {
          trackUp();
        }
        if (pressed == PAD_START) {
          if (mode == MenuMode::menu1) {
            menu(MenuMode::menu2);
          } else {
            menu(MenuMode::menu1);
          }
        }
        if (pressed == PAD_B) {
          menu(MenuMode::live);
        }
      }
      if (mode == MenuMode::menu2) {
        if (selectedMenu2Control == Menu2Control::bpm) {
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
              selectedTrack()->bpmDividerDown();
              display.fillScreen(ST7735_WHITE);
            }
            if (released & PAD_UP) {
              selectedTrack()->bpmDividerUp();
              display.fillScreen(ST7735_WHITE);
            }
          }
        }
      }
    }

    if (mode == MenuMode::live) {
      selectedTrack()->handle(modifier, pressed, selectedBlockIndex);
    } else if (mode == MenuMode::menu1) {
      selectedTrack()->handle(selectedMenu1Control, modifier, pressed,
                              selectedBlockIndex);
    } else if (mode == MenuMode::menu2) {
      selectedTrack()->handle(selectedMenu2Control, modifier, pressed,
                              selectedBlockIndex);
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

    if (released == PAD_SELECT && ignoreRelease & PAD_SELECT) {
      ignoreRelease ^= PAD_SELECT;
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
  machine.begin();
  AudioMemory(18);
  controls.begin();
  display.initR(INITR_BLACKTAB);
  display.setRotation(1);
  pinMode(DISPLAY_BACKLIGHT, OUTPUT);
  digitalWrite(DISPLAY_BACKLIGHT, HIGH);
  display.fillScreen(ST77XX_WHITE);
  neopixels.begin();
  neopixels.setBrightness(4);
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
