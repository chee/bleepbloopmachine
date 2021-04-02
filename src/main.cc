#include "main.h"
#include "controls.h"
#include "display.h"
#include "block.h"
#include "track.h"
#include "weirdmath.h"

#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

#define SPEAKER_ENABLE 51

#define LIGHT_SENSOR A7
#define BATTERY_SENSOR A6

#define RIGHT_AUDIO_PIN A0
#define LEFT_AUDIO_PIN A1

#define SD_CS 4

#include "Adafruit_TinyUSB.h"
#include "AudioStream.h"
#include "output_dacs.h"

#include <Adafruit_NeoPixel.h>
#include <Audio.h>

AudioMixer4 mainmixer0L;
AudioMixer4 mainmixer0R;
AudioMixer4 mainmixer1L;
AudioMixer4 mainmixer1R;
AudioMixer4 mainmixerL;
AudioMixer4 mainmixerR;
AudioConnection p0l(mainmixer0L, 0, mainmixerL, 0);
AudioConnection p0r(mainmixer0R, 0, mainmixerR, 0);
AudioConnection p1l(mainmixer1L, 0, mainmixerL, 1);
AudioConnection p1r(mainmixer1R, 0, mainmixerR, 1);
AudioOutputAnalogStereo headphones;
AudioConnection left_ear_patch(mainmixerL, 0, headphones, 0);
AudioConnection right_ear_patch(mainmixerR, 0, headphones, 1);

Adafruit_NeoPixel neopixels(NEOPIXEL_LENGTH, NEOPIXEL_PIN, NEO_GRB);

Adafruit_ST7735 display =
    Adafruit_ST7735(&SPI1, DISPLAY_CS, DISPLAY_DC, DISPLAY_RST);

Point getCoord(int idx) {
  int x = idx % 4;
  int y = idx / 4;
  return Point {
    x : GRID_OFFSET_X + (x * (BLOCK_SIZE + GRID_GAP)),
    y : GRID_OFFSET_Y + (y * (BLOCK_SIZE + GRID_GAP))
  };
}


// TODO sizeof()/sizeof[0]
#define NUMBER_OF_TRACKS 5
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
  int selectedTrackIndex = 0;
  int selectedBlockIndex = 0;
  int lastSelectedBlockIndex = 0;
  int currentStep = 0;
  int ignoreRelease = 0;
  Track *tracks[NUMBER_OF_TRACKS];
  void begin() {
    // TODO refactor?, something like track.connect(l, r, idx)
    tracks[0] = new WaveformTrack('q', Waveform::square, mainmixer0L, mainmixer0R, 0);
    tracks[1] = new WaveformTrack('p', Waveform::pulse, mainmixer0L, mainmixer0R, 1);
    tracks[2] = new WaveformTrack('s', Waveform::sawtooth, mainmixer0L, mainmixer0R, 2);
    tracks[3] =
        new WaveformTrack('n', Waveform::sampleHold, mainmixer0L, mainmixer0R, 3);
    tracks[4] = new KitTrack('k', mainmixer1L, mainmixer1R, 0);
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
  Track *selectedTrack() { return tracks[selectedTrackIndex]; }
  void trackUp() {
    selectedTrackIndex = wrapping_sub(selectedTrackIndex, NUMBER_OF_TRACKS - 1);
    display.fillScreen(ST7735_WHITE);
  }
  void trackDown() {
    // TODO only blank out the location of the track char
    selectedTrackIndex = wrapping_add(selectedTrackIndex, NUMBER_OF_TRACKS - 1);
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
	uint16_t readLightSensor() { return analogRead(LIGHT_SENSOR); }
	float readBatterySensor() {
  	return ((float)analogRead(BATTERY_SENSOR) / 1023.0) * 2.0 * 3.3;
	}
  void updateDisplay() {
    auto stepBlockIndex = selectedTrack()->stepBlockIndex(currentStep);
    // first comes blocks
    for (auto i = 0; i <= 0xf; i++) {
      Point coord = getCoord(i);
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
        if (released & PAD_DOWN) {
          trackDown();
        }
        if (released & PAD_UP) {
          trackUp();
        }
        if (pressed == PAD_B) {
          menu(MenuMode::live);
        }
        if (mode == MenuMode::menu1) {
          if (released & PAD_RIGHT) {
            selectedMenu1Control = ++selectedMenu1Control;
          }
          if (released & PAD_LEFT) {
            selectedMenu1Control = --selectedMenu1Control;
          }
          if (pressed == PAD_START) {
            menu(MenuMode::menu2);
          }
        } else if (mode == MenuMode::menu2) {
          if (released & PAD_RIGHT) {
            selectedMenu2Control = ++selectedMenu2Control;
          }
          if (released & PAD_LEFT) {
            selectedMenu2Control = --selectedMenu2Control;
          }
          if (pressed == PAD_START) {
            menu(MenuMode::menu1);
          }
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

Controls controls;

void setup() {
  Serial.begin(115200);
  machine.begin();
	pinMode(SPEAKER_ENABLE, OUTPUT);
	digitalWrite(SPEAKER_ENABLE, HIGH);
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
	Serial.println(machine.readBatterySensor());
  machine.handleButtons(buttons, justReleased, joyX, joyX);
  machine.step();
}
