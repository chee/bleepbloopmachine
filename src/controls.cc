#include "controls.h"

void Controls::begin() {
  pinMode(BUTTON_CLOCK, OUTPUT);
  digitalWrite(BUTTON_CLOCK, HIGH);
  pinMode(BUTTON_LATCH, OUTPUT);
  digitalWrite(BUTTON_LATCH, HIGH);
  pinMode(BUTTON_DATA, INPUT);
}

int16_t Controls::readJoystickX(uint8_t sampling) {
  float reading = 0;

  if (JOYSTICK_X >= 0) {
    for (int i = 0; i < sampling; i++) {
      reading += analogRead(JOYSTICK_X);
    }
    reading /= sampling;

    reading -= JOYSTICK_X_CENTER;
  }

  return reading;
}

int16_t Controls::readJoystickY(uint8_t sampling) {
  float reading = 0;

  if (JOYSTICK_Y >= 0) {
    for (int i = 0; i < sampling; i++) {
      reading += analogRead(JOYSTICK_Y);
    }
    reading /= sampling;

    reading -= JOYSTICK_Y_CENTER;
  }

  return reading;
}

uint32_t Controls::readButtons(void) {
  uint32_t buttons = 0;

  uint8_t shift_buttons = 0;

  digitalWrite(BUTTON_LATCH, LOW);
  // delayMicroseconds(1);
  digitalWrite(BUTTON_LATCH, HIGH);
  // delayMicroseconds(1);

  for (int i = 0; i < 8; i++) {
    shift_buttons <<= 1;
    shift_buttons |= digitalRead(BUTTON_DATA);
    digitalWrite(BUTTON_CLOCK, HIGH);
    // delayMicroseconds(1);
    digitalWrite(BUTTON_CLOCK, LOW);
    // delayMicroseconds(1);
  }

  if (shift_buttons & BUTTON_SHIFTMASK_B) {
    buttons |= PAD_B;
  }
  if (shift_buttons & BUTTON_SHIFTMASK_A) {
    buttons |= PAD_A;
  }
  if (shift_buttons & BUTTON_SHIFTMASK_SELECT) {
    buttons |= PAD_SELECT;
  }
  if (shift_buttons & BUTTON_SHIFTMASK_START) {
    buttons |= PAD_START;
  }

  int16_t x = readJoystickX();
  if (x > 350)
    buttons |= PAD_RIGHT;
  else if (x < -350)
    buttons |= PAD_LEFT;
  int16_t y = readJoystickY();
  if (y > 350)
    buttons |= PAD_DOWN;
  else if (y < -350)
    buttons |= PAD_UP;

  // TODO add encoder in here when it's pinned

  last_buttons = curr_buttons;
  curr_buttons = buttons;
  justpressed_buttons = (last_buttons ^ curr_buttons) & curr_buttons;
  justreleased_buttons = (last_buttons ^ curr_buttons) & last_buttons;

  return buttons;
}

uint32_t Controls::justPressed() { return justpressed_buttons; }

uint32_t Controls::justReleased() { return justreleased_buttons; }

Menu1Control &operator++(Menu1Control &s) {
  return s = Menu1Control{wrapping_add((int)s, 3)};
}

Menu1Control &operator--(Menu1Control &s) {
  return s = Menu1Control{wrapping_sub((int)s, 3)};
}


Menu2Control &operator++(Menu2Control &s) {
  return s = Menu2Control{wrapping_add((int)s, 3)};
}

Menu2Control &operator--(Menu2Control &s) {
  return s = Menu2Control{wrapping_sub((int)s, 3)};
}
