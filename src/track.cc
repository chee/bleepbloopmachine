#include "display.h"

#include "track.h"
#include "controls.h"

#include "kit/AudioSampleC.h"
#include "kit/AudioSampleH.h"
#include "kit/AudioSampleK.h"
#include "kit/AudioSampleO.h"
#include "kit/AudioSampleS.h"

String note_names[] = {"c",  "c#", "d",  "d#", "e",  "f",
                       "f#", "g",  "g#", "a",  "a#", "b"};

void KitTrack::handle(Menu1Control control, Keymod modifier, uint32_t pressed,
              int blockIndex) {}

void KitTrack::handle(Menu2Control control, Keymod modifier, uint32_t pressed,
              int blockIndex) {}

void KitTrack::handle(Keymod modifier, uint32_t pressed, int blockIndex) {
  auto block = &blocks[blockIndex];
  if (modifier == Keymod::b) {
    // if (pressed & PAD_UP) {
    //   block->sampleRateUp();
    // }
    // if (pressed & PAD_DOWN) {
    //   block->sampleRateDown();
    // }
    if (pressed & PAD_LEFT) {
      block->sampleDown();
    }
    if (pressed & PAD_RIGHT) {
      block->sampleUp();
    }
  } else if (modifier == Keymod::none) {
    if (pressed == PAD_B) {
      block->activate();
    } else if (pressed == PAD_A) {
      block->deactivate();
      // TODO local clipboard
      // if (block->active) {
      //   clipboard = sound->cut();
      //   hasClipboard = true;
      // } else if (hasClipboard) {
      //   blocks[blockIndex] = clipboard;
      // }
    }
  }
}

void KitTrack::play(int blockIndex) {
  auto block = &blocks[blockIndex];
  if (!block->active) {
    return;
  }
  switch (block->sample) {
  case 0:
    player->play(AudioSampleK);
    break;
  case 1:
    player->play(AudioSampleS);
    break;
  case 2:
    player->play(AudioSampleH);
    break;
  case 3:
    player->play(AudioSampleO);
    break;
  case 4:
    player->play(AudioSampleC);
    break;
  default:
    break;
  }
}

void WaveformTrack::updateDisplay(Adafruit_GFX *display, MenuMode mode,
                                  Menu1Control menu1control,
                                  Menu2Control menu2control) {
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
        display->drawLine(
            point.x + attackX, (point.y + BLOCK_SIZE) - BLOCK_SIZE * sound->amp,
            point.x + attackX + decayW, (point.y + BLOCK_SIZE), envColor);

        // draw sustain
        auto susY = BLOCK_SIZE * sound->sustain;
        display->fillRect(point.x + BLOCK_SIZE - 4,
                          point.y + (BLOCK_SIZE - susY), 4, susY, ST7735_CYAN);
      }
    } else if (mode == MenuMode::menu2) {
      display->drawPixel(point.x + 5, point.y + 5, ST7735_BLACK);
    }
  }
  display->drawChar(10, 10, name, ST77XX_BLACK, ST7735_BLACK, 1);
}
