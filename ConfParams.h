#ifndef _CONF_PARAMS_H_
#define _CONF_PARAMS_H_

class ConfParams {
public:  
  time_t noIrrTime0Init;
  time_t noIrrTime0End;
  time_t noIrrTime1Init;
  time_t noIrrTime1End;
  time_t noIrrTime2Init;
  time_t noIrrTime2End;
  time_t noIrrTime3Init;
  time_t noIrrTime3End;
  
  unsigned int irrSlotSeconds;
  unsigned int irrMIntervMins;
  unsigned int irrMaxTimeDaySeconds;
  float critLevel;
  float satLevel;
  unsigned long normalPulsesPerSec;

  ConfParams() : 
      noIrrTime0Init(0), 
      noIrrTime0End(0), 
      noIrrTime1Init(0), 
      noIrrTime1End(0), 
      noIrrTime2Init(0), 
      noIrrTime2End(0), 
      noIrrTime3Init(0), 
      noIrrTime3End(0),
      irrSlotSeconds(0),
      irrMIntervMins(0),
      irrMaxTimeDaySeconds(0),
      critLevel(0),
      satLevel(0),
      normalPulsesPerSec(0) { }

   inline bool isEmptyInterval(time_t initialTime, time_t endTime) {
     return (initialTime == 0) && (endTime == 0);
   }
   inline bool isValidOrEmptyInterval(time_t initialTime, time_t endTime) {
     return isEmptyInterval(initialTime, endTime) || (endTime > initialTime);
   }
   inline bool isAllValid() {
     return isValidOrEmptyInterval(noIrrTime0Init, noIrrTime0End) &&
        isValidOrEmptyInterval(noIrrTime1Init, noIrrTime1End) &&
        isValidOrEmptyInterval(noIrrTime2Init, noIrrTime2End) &&
        isValidOrEmptyInterval(noIrrTime3Init, noIrrTime3End) &&
        irrSlotSeconds > 0 && irrMIntervMins > 0 && irrMaxTimeDaySeconds > 0 && critLevel >= 0
        && critLevel < 100 && satLevel > critLevel && satLevel > 0 && satLevel <= 100;
   }
   
};

extern ConfParams mainConfParams;

#endif
