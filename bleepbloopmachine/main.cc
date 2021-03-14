#include "main.h"
#include <Adafruit_BusIO.h>
#include "Adafruit_SPIFlash.h"
#include "Adafruit_TinyUSB.h"
#include "AudioStream.h"
#include "output_dacs.h"
#include <Adafruit_Arcada.h>
#include <Adafruit_NeoPixel.h>
#include <Arduino.h>
#include <Audio.h>

	 Adafruit_Arcada arcada;

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


Adafruit_FlashTransport_QSPI flashTransport;
Adafruit_SPIFlash flash(&flashTransport);
FatFileSystem fatfs;
FatFile root;
FatFile file;
Adafruit_USBD_MSC usb_msc;


void setup() {
  arcada.arcadaBegin();
  arcada.filesysBeginMSD();
  arcada.displayBegin();
  arcada.setBacklight(255);
  arcada.drawBMP((char *)"icon.bmp", 0, 0);
  neopixels.begin();
  neopixels.setBrightness(180);
}

void loop() {
  arcada.readButtons();
  uint8_t buttons = arcada.justPressedButtons();
  int16_t joyX = arcada.readJoystickX();
  int16_t joyY = arcada.readJoystickY();
}
