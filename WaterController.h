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
