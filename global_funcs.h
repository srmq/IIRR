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

extern bool fsOpen;

#endif
