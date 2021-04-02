#pragma once
#include "weirdmath.h"

class Block {
public:
  bool active = false;
  void activate() { active = true; }
  void deactivate() { active = false; }
};

class KitBlock : public Block {
public:
  int sample;
  void sampleUp() { sample = wrapping_add(sample, 4); }
  void sampleDown() { sample = wrapping_sub(sample, 4); }
};

#define MAX_ATTACK 800.0
#define MAX_DECAY 800.0

class SoundBlock : public Block {
public:
  enum Filter { low };
  int note = 0;
  int octave = 3;
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
  float frequency();
  void activate(int n, int o) {
    note = n;
    octave = o;
    active = true;
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
  void decayUp() { decay = saturating_add_curve(decay, MAX_DECAY, 10, 0); }
  void decayDown() { decay = saturating_sub_curve(decay, MAX_DECAY, 10, 0); }
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
  void attackUp() { attack = saturating_add_curve(attack, MAX_ATTACK, 10, 0); }
  void attackDown() { attack = saturating_sub_curve(attack, MAX_ATTACK, 10, 0); }
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
};
