#include "main.h"
#include <Arduino.h>

#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>
#define TFT_CS 44
#define TFT_RST 46
#define TFT_DC 45
#define TFT_BACKLIGHT 47

//#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"
#include "AudioStream.h"
#include "output_dacs.h"

#include <Adafruit_NeoPixel.h>
#include <Audio.h>

Adafruit_ST7735 display = Adafruit_ST7735(&SPI1, TFT_CS, TFT_DC, TFT_RST);

class Wave {};

class BleepBloopMachine {
  menu_mode mode = menu_mode::live;
  float bpm = 120.0;
  int current_tick = 0;
  int bar_of_ticks = 255;
  int selected_sound_index = 0;
  int selected_menu1_control = 0;
  int selected_menu2_control = 0;
  int selected_wave_index = 0;
  // Wave[] & waves[4];
  // TODO this needs to be whatever
  int last_keys;
};

// pygamer specific
Adafruit_NeoPixel neopixels(5, 8, NEO_GRB);

/* there are only monophonic noises on this one so far :)
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
AudioOutputAnalogStereo headphones;
AudioConnection left_ear_patch(mixer, 0, headphones, 0);
AudioConnection right_ear_patch(mixer, 0, headphones, 1);

void setup() {
  display.initR(INITR_BLACKTAB);
  display.setRotation(1);
  pinMode(TFT_BACKLIGHT, OUTPUT);
  digitalWrite(TFT_BACKLIGHT, HIGH);
  display.fillScreen(ST77XX_BLACK);
  display.fillScreen(ST77XX_YELLOW);
  display.drawCircle(0, 10, 20, ST77XX_BLACK);
  // arcada.arcadaBegin();
  // arcada.filesysBeginMSD();
  // arcada.displayBegin();
  // arcada.setBacklight(255);
  // arcada.drawBMP((char *)"icon.bmp", 0, 0);
  neopixels.begin();
  neopixels.setBrightness(180);
}

void loop() {
  // arcada.readButtons();
  // uint8_t buttons = arcada.pressed();
  // int16_t joyX = arcada.readJoystickX();
  // int16_t joyY = arcada.readJoystickY();
}
