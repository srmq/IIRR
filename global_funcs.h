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

#ifndef GLOBAL_FUNCS_H
#define GLOBAL_FUNCS_H

#include <WString.h>
#include <TimeLib.h>
#include <Time.h>


void disableMulAndDecod();
void enableMulAndDecod();

bool isNumber(const String& str);

typedef struct soil_moisture {
  float surface;
  float middle;
  float deep;
  //bool hasWater;
  time_t timeStamp;
} SoilMoisture;

class IrrigData {
public:
  float surfaceAtStartIrrig;
  float middleAtStartIrrig;
  float deepAtStartIrrig;
  bool isIrrigating;
  time_t irrigSince;
  //guardar lastIrrigEnd e irrigTodaySecs para o caso de faltar energia.
  time_t lastIrrigEnd;
  unsigned long irrigTodaySecs;
  
  IrrigData() : surfaceAtStartIrrig(0), middleAtStartIrrig(0), deepAtStartIrrig(0), isIrrigating(false), 
                irrigSince(0), lastIrrigEnd(0), irrigTodaySecs(0) {}
};

extern SoilMoisture moistures;

extern IrrigData irrigData;

extern const char LOG_DIR[] PROGMEM;
extern const char TS_FMT_STR[] PROGMEM; //yyyymmddThhmmss

extern bool fsOpen;

#endif
