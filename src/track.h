#pragma once
#include "block.h"
#include "controls.h"

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <Audio.h>

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

class Track {
protected:
  int lastStepBlockIndex = 0;
  SoundBlock clipboard;
  bool hasClipboard = false;

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
  void step(int currentStep) {
    auto idx = stepBlockIndex(currentStep);
    if (idx != lastStepBlockIndex) {
      lastStepBlockIndex = idx;
      play(idx);
    }
  }
  virtual void play(int blockIndex) = 0;
};

class KitTrack : public Track {
  AudioPlayMemory *player = new AudioPlayMemory();
  AudioConnection *patches[2];

public:
  KitBlock *blocks = new KitBlock[16];
  // TODO update display should be the same every time, it should grab info it
  // needs to display from the blocks?
  void updateDisplay(Adafruit_GFX *display, MenuMode mode,
                     Menu1Control menu1control, Menu2Control menu2control) {}
  void handle(Menu1Control control, Keymod modifier, uint32_t pressed,
              int blockIndex);
  void handle(Menu2Control control, Keymod modifier, uint32_t pressed,
              int blockIndex);
  void handle(Keymod modifier, uint32_t pressed, int blockIndex);
  void play(int blockIndex);
  KitTrack(unsigned char n, AudioMixer4 &lMixer, AudioMixer4 &rMixer,
           int mixerIndex) {
    name = n;
    patches[0] = new AudioConnection(*player, 0, lMixer, mixerIndex);
    patches[1] = new AudioConnection(*player, 0, rMixer, mixerIndex);
  }
	~KitTrack() {
    // TODO  delete pointers
    delete[] patches;
  }
};

class WaveformTrack : public Track {
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
  WaveformTrack(unsigned char n, Waveform f, AudioMixer4 &lmixer,
                AudioMixer4 &rmixer, int mixerIndex) {
    form = f;
    name = n;
    patches[0] = new AudioConnection(*synthL, *envL);
    patches[1] = new AudioConnection(*envL, *filterL);
    patches[2] = new AudioConnection(*filterL, 0, *mixerL, 0);
    patches[3] = new AudioConnection(*synthR, *envR);
    patches[4] = new AudioConnection(*envR, *filterR);
    patches[5] = new AudioConnection(*filterR, 0, *mixerR, 0);
    patches[6] = new AudioConnection(*mixerL, 0, lmixer, mixerIndex);
    patches[7] = new AudioConnection(*mixerR, 0, rmixer, mixerIndex);
    filterL->setLowpass(0, 12000);
    filterR->setLowpass(0, 12000);
    synthL->begin((int)f);
    synthR->begin((int)f);
    synthL->amplitude(1.0);
    synthR->amplitude(1.0);
    envL->sustain(1.0);
    envR->sustain(1.0);
    if (f == Waveform::pulse) {
      synthL->pulseWidth(0.5);
      synthR->pulseWidth(0.5);
    }
  }
  ~WaveformTrack() {
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
  void updateDisplay(Adafruit_GFX *display, MenuMode mode,
                     Menu1Control menu1control, Menu2Control menu2control);
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
