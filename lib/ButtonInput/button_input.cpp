#include "button_input.h"

const ButtonInput::ButtonPin ButtonInput::kPins[ButtonInput::kButtonCount] = {
    {ButtonId::kUp, 32},
    {ButtonId::kDown, 33},
    {ButtonId::kLeft, 14},
    {ButtonId::kRight, 13},
    {ButtonId::kConfirm, 27},
};

volatile bool ButtonInput::pressedFlags[ButtonInput::kButtonCount] = {
    false, false, false, false, false};

void ButtonInput::begin(ButtonHandler handlerIn) {
  handler = handlerIn;

  for (uint8_t i = 0; i < kButtonCount; i++) {
    pinMode(kPins[i].pin, INPUT_PULLUP);
  }

  attachInterrupt(digitalPinToInterrupt(kPins[0].pin), isrUp, FALLING);
  attachInterrupt(digitalPinToInterrupt(kPins[1].pin), isrDown, FALLING);
  attachInterrupt(digitalPinToInterrupt(kPins[2].pin), isrLeft, FALLING);
  attachInterrupt(digitalPinToInterrupt(kPins[3].pin), isrRight, FALLING);
  attachInterrupt(digitalPinToInterrupt(kPins[4].pin), isrConfirm, FALLING);
}

void ButtonInput::poll() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < kButtonCount; i++) {
    if (!pressedFlags[i]) {
      continue;
    }
    pressedFlags[i] = false;

    if (now - lastTriggerMs[i] < kDebounceMs) {
      continue;
    }
    lastTriggerMs[i] = now;

    if (handler) {
      handler(kPins[i].id);
    }
  }
}

void IRAM_ATTR ButtonInput::isrUp() { pressedFlags[0] = true; }
void IRAM_ATTR ButtonInput::isrDown() { pressedFlags[1] = true; }
void IRAM_ATTR ButtonInput::isrLeft() { pressedFlags[2] = true; }
void IRAM_ATTR ButtonInput::isrRight() { pressedFlags[3] = true; }
void IRAM_ATTR ButtonInput::isrConfirm() { pressedFlags[4] = true; }
