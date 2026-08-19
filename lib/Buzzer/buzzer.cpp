#include "buzzer.h"

void Buzzer::begin() {
  pinMode(kPin, OUTPUT);
  digitalWrite(kPin, LOW);
}

void Buzzer::beep(uint16_t frequencyHz, uint16_t durationMs) {
  tone(kPin, frequencyHz, durationMs);
}
