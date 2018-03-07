/**
 * IIRR -- Intelligent Irrigator Based on ESP8266
    Copyright (C) 2016--2018  Sergio Queiroz <srmq@cin.ufpe.br>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

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

