#include "global_funcs.h"
#include "sensor_calibration.h"
#include <Arduino.h>

void disableMulAndDecod() {
  digitalWrite(MUL_DECOD_INHIB_PIN, HIGH);
}

void enableMulAndDecod() {
  digitalWrite(MUL_DECOD_INHIB_PIN, LOW);
}

bool isNumber(const String& str) {
  int i;
  for (i = 0; i < str.length(); ++i) {
    const unsigned char c = str[i];
    if (!std::isdigit(c)) {
      break;
    }
  }

  return (i == str.length());
}

