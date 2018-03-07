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

#ifndef _SENSOR_CALIBRATION_H_
#define _SENSOR_CALIBRATION_H_

#include <TimeLib.h>
#include <Time.h>


#define ANALOG_PIN A0
#define DECOD_A0_PIN D6
#define DECOD_A1_PIN D5
#define DECOD_A2_PIN D0
#define MUL_DECOD_INHIB_PIN D4
#define MULA_PIN D1
#define MULB_PIN D2
#define MULC_PIN D3
//#define NOWATER_SWITCH_PIN D7
#define FLOWSIGNAL_PIN D7
#define PUMP_PIN D8

#define REFERENCE_RESISTOR 4700
#define NUM_PROBES 11
#define SIGNAL_DELAY 10
#define MINAFTERVOLTAGE_OPENCIRCUIT 10

#define AFTERVOLTAGE_OPEN -1
#define SHORTCIRCUIT_SENSOR_MINDIFF 2 
#define SENSOR_SHORTCIRCUIT -2
#define MOISTURE_READERROR -3
#define MOISTURE_MULX 593.288368205802
#define MOISTURE_EXPFACT 1.30431394603425

#define SENSOR_READ_DELAY 5000

#define FS_LOG_KEEP_DAYS 30
#define FS_LOG_WRITE_INTERVAL 300

//#define DEBUG_SENSOR_MODE

enum SensorInput {
  SURFACE_LEFT,
  SURFACE_RIGHT,
  MIDDLE_LEFT,
  MIDDLE_RIGHT,
  DEEP_LEFT,
  DEEP_RIGHT
};

enum SensorType {
  SURFACE,
  MIDDLE,
  DEEP
};

enum SensorDirection {
  LEFT,
  RIGHT
};


#endif
