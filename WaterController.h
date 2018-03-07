#ifndef _WATERCONTROLLER_H
#define _WATERCONTROLLER_H

#include "TimeKeeper.h"

enum WaterStartStatus {
  WATER_STARTOK = 0,
  WATER_STARTNOACTION, //is already started
  WATER_STARTEMPTY, //will not start because empty state was selected, may need resetEmpty first
  WATER_STARTNOCONF //will not start because lacks configuration
};

enum WaterCurrSensorStatus {
  WATER_CURRFLOWING,
  WATER_CURRSTOP,
  WATER_CURREMPTY, //empty means water block was detected, may need manual reset
  WATER_CURRNOCONF //need to configure first to distinguish between flowing, empty and stop status
};

class WaterController {
public:
  virtual WaterStartStatus startWater(bool ignoreNoConf = false) = 0;
  virtual void stopWater() = 0;
  virtual WaterCurrSensorStatus currStatus() = 0;
  virtual void resetEmpty() = 0;
  virtual bool configureFlow() = 0; 

  virtual ~WaterController() = 0;
};

inline WaterController::~WaterController() {}

class WellPumpWaterController : public WaterController {
public:
  virtual WaterStartStatus startWater(bool ignoreNoConf = false) override;
  virtual void stopWater() override;
  virtual WaterCurrSensorStatus currStatus() override;
  virtual void resetEmpty() override;
  virtual bool configureFlow() override;
  virtual ~WellPumpWaterController();
  WellPumpWaterController();
  inline void setDelayFunction(NTPClient::DelayHandlerFunction delayFunction) { this->delayHandler = delayFunction; }

private:
  NTPClient::DelayHandlerFunction delayHandler = delay;
  bool pumpIsOn;
  bool noConfStatus;
  bool emptyTriggered;
  void turnOnSensor();
  void turnOffSensor();
  double avgSensorPulsesPerSec();
  const double acceptedMinFlowPerc = 0.3;
};


#endif
