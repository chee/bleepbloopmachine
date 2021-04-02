#pragma once
#include <Arduino.h>
#include "weirdmath.h"

#define BUTTON_CLOCK 48
#define BUTTON_DATA 49
#define BUTTON_LATCH 50
#define BUTTON_SHIFTMASK_B 0x80
#define BUTTON_SHIFTMASK_A 0x40
#define BUTTON_SHIFTMASK_START 0x20
#define BUTTON_SHIFTMASK_SELECT 0x10

#define JOYSTICK_X A11
#define JOYSTICK_Y A10

#define JOYSTICK_X_CENTER 512
#define JOYSTICK_Y_CENTER 512

#define PAD_A 0x01
#define PAD_B 0x02
#define PAD_SELECT 0x04
#define PAD_START 0x08
#define PAD_UP 0x10
#define PAD_DOWN 0x20
#define PAD_LEFT 0x40
#define PAD_RIGHT 0x80

#define KEYLAYER_0 0
#define KEYLAYER_1 1
#define KEYLAYER_2 2
#define KEYLAYER_3 3
#define KEYLAYER_4 4
#define KEYLAYER_5 5

enum class Keymod { a, b, ab, select, start, none };

class Controls {
public:
  void begin();
  int16_t readJoystickX(uint8_t sampling = 3);
  int16_t readJoystickY(uint8_t sampling = 3);
  uint32_t readButtons();
  uint32_t justPressed();
  uint32_t justReleased();

private:
  uint32_t last_buttons;
  uint32_t curr_buttons;
  uint32_t justpressed_buttons;
  uint32_t justreleased_buttons;
};

enum class MenuMode { live, menu1, menu2 };

enum class Menu1Control { env, mod, filter, delay };

enum class Menu2Control { bpm, chain, wave, file };

Menu1Control &operator++(Menu1Control &s);

Menu1Control &operator--(Menu1Control &s);

Menu2Control &operator++(Menu2Control &s);

Menu2Control &operator--(Menu2Control &s);
