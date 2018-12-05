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

#include "WaterController.h"
#include "ConfParams.h"
#include "sensor_calibration.h"
#include "SensorTask.h"
#include "global_funcs.h"

static unsigned long pulseCount;

static void handleInterrupt() {
  pulseCount++;
}

WaterStartStatus WellPumpWaterController::startWater(bool ignoreNoConf) {
 if(!ignoreNoConf && this->noConfStatus()) return WATER_STARTNOCONF;
 if(this->emptyTriggered) return WATER_STARTEMPTY;
 if(pumpIsOn) return WATER_STARTNOACTION;

 digitalWrite(PUMP_PIN, HIGH);
 this->pumpIsOn = true;
 return WATER_STARTOK;
}

void WellPumpWaterController::stopWater() {
 digitalWrite(PUMP_PIN, LOW);
 this->pumpIsOn = false;
}

WellPumpWaterController::~WellPumpWaterController() {
  if (this->currStatus() == WATER_CURRFLOWING)
    this->stopWater();
}

WaterCurrSensorStatus WellPumpWaterController::currStatus() {
  if (this->emptyTriggered) return WATER_CURREMPTY;
  if (this->noConfStatus()) return WATER_CURRNOCONF;
  double avgPulsesSec;
  turnOnSensor();
  avgSensorPulsesPerSec();
  avgPulsesSec = 0.5*avgSensorPulsesPerSec();
  avgPulsesSec += 0.5*avgSensorPulsesPerSec(); //we call three times, ignoring the first and taking the avg of the last two.
  turnOffSensor();
  bool noFlow = (avgPulsesSec <= (acceptedMinFlowPerc*mainConfParams.normalPulsesPerSec));
  if (!noFlow) {
    return WATER_CURRFLOWING;
  }
  
  //here we do not have flow
  if (this->pumpIsOn) {
    //trigger EMPTY
    stopWater();
    this->emptyTriggered = true;
    return WATER_CURREMPTY;
  } else {
    //pump is off, we dont have flow
    return WATER_CURRSTOP;
  }
}

void WellPumpWaterController::resetEmpty() {
  this->emptyTriggered = false;
}

bool WellPumpWaterController::configureFlow() {
  double avgPulsesSec;

  bool pumpWasOn = this->pumpIsOn;
  if (!pumpWasOn) {
    if (this->startWater(true) != WATER_STARTOK) {
      return false;
    }
  }
  delayHandler(30000);
  this->turnOnSensor();
  delayHandler(1000);
  avgSensorPulsesPerSec();
  avgPulsesSec = 0.5*avgSensorPulsesPerSec();
  avgPulsesSec += 0.5*avgSensorPulsesPerSec(); //we call three times, ignoring the first and taking the avg of the last two.
  this->turnOffSensor();
  if (!pumpWasOn) {
    this->stopWater();
  }
  mainConfParams.normalPulsesPerSec = (unsigned long)avgPulsesSec;
  return SensorTask::updateConfParams(mainConfParams);
}

void WellPumpWaterController::turnOnSensor() {
  digitalWrite(DECOD_A0_PIN, LOW);
  digitalWrite(DECOD_A1_PIN, HIGH);
  digitalWrite(DECOD_A2_PIN, HIGH);
  enableMulAndDecod();
}

void WellPumpWaterController::turnOffSensor() {
  disableMulAndDecod();
}


double WellPumpWaterController::avgSensorPulsesPerSec() {
  pulseCount = 0;
  //XXX  chamar codigo de ligar o sensor antes de chamar essa funcao
  
  attachInterrupt(digitalPinToInterrupt(FLOWSIGNAL_PIN), handleInterrupt, FALLING);
  unsigned long startTime = millis();
  do {
    delayHandler(50);
  } while ((millis() - startTime) < 1000);
  detachInterrupt(digitalPinToInterrupt(FLOWSIGNAL_PIN));
  double pulsesPerSec = (1000.0/(millis() - startTime))*pulseCount;
  return pulsesPerSec;
}

WellPumpWaterController::WellPumpWaterController() : WaterController(), 
    pumpIsOn(false), 
    emptyTriggered(false){ 
      
  digitalWrite(PUMP_PIN, LOW);
}
