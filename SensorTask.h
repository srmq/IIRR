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

#ifndef _SENSOR_TASK_H_
#define _SENSOR_TASK_H_
#include "sensor_calibration.h"
#include <Scheduler.h>
#include "TimeKeeper.h"
#include "FS.h"
#include <ArduinoJson.h>
#include "ConfParams.h"
#include "WaterController.h"
#include "global_funcs.h"

#define MIN_IRRIG_TS 300

enum StopIrrigReason {
  STOPIRRIG_SLOTEND,
  STOPIRRIG_SURFACESAT,
  STOPIRRIG_MIDDLESAT,
  STOPIRRIG_DEEPINCREASE,
  STOPIRRIG_MAXTIMEDAY,
  STOPIRRIG_WATEREMPTY
};

enum MessageTypes {
  MSG_DEBUG,
  MSG_INFO,
  MSG_WARN,
  MSG_ERR
};

enum MessageCodes {
  MSG_INCONSIST_WATER_CURRSTATUS, //this means that we were expecting a certain water 
                                 // current status, but another was found. First it
                                 // is logged the status found and after the one expected
  MSG_STOPPED_IRRIG, // this means that we stopped irrigating. It may be just informational.
  MSG_STARTED_IRRIG 
};

enum AsyncLearnFlowStatus {
  LFLOW_NOTREQUESTED,
  LFLOW_INPROGRESS,
  LFLOW_DONE,
  LFLOW_ERROR
};

class SensorTask : public Task {
private:
  float readMoisture(SensorType sType);

  // the loop function runs over and over again forever
  void loopSensorMode();
  
  void selectSensor(SensorInput sensor);
  
  SensorDirection switchSensorDirection();
  
  //bool isWithWater();
  
  void enableSensorVoltage(SensorType sType, SensorDirection sDir);
  
  int readReferenceVoltage(SensorType sType, SensorDirection sDir);
  
  int readAfterSensorVoltage(SensorType sType, SensorDirection sDir);

  int readVoltage(SensorType sType, SensorDirection sDirection, SensorInput sensorInput);

  static void multiTaskDelay(SensorTask *taskServer, unsigned long ms);

  File getCurrLogFile(time_t nowTime);
  File getCurrMsgFile(time_t nowTime);
  static File getFSFileWithDate(time_t nowTime, PGM_P fmtStr, const int bufSize);

  static bool doFSMaintenance(int keepDays);
  static bool doFSMaintenance(int keepDays, const String& dirName, const String& commonName);

  ConfParams *readMainConfParams();

  static time_t toConfTimet(time_t aTime_t);

  static time_t confParamTime2Timet(const char* timeStr);

  static bool isInNoIrrigTime(time_t aTime);

  static bool isInHHMMConfInterval(time_t aTimeInConfTimet, time_t initHHMM, time_t endHHMM);  

  static unsigned int irrigTodayRemainingSecs();

  bool fulfillMinIrrigInterval(time_t aTime);

  TimeKeeper timeKeeper;

  WellPumpWaterController waterControl;

  inline static bool isValidMoisture(float percent) { return (percent >= 0) && (percent <= 100); }

  bool stopIrrigationAndLog(time_t aTime, enum StopIrrigReason);
  bool startIrrigationAndLog(time_t aTime, const SoilMoisture& moist);
  static File getFSFileWithDateForRead(time_t aTime, PGM_P fmtStr, const int bufSize);

protected:
    void loop();

public:
    SensorTask();
    static JsonObject& createJsonFromConfParams(DynamicJsonBuffer& jsonBuffer);
    static void updateConfParamsFromJson(ConfParams& confStruct, JsonObject& jsonConfParamsRoot);
    static bool updateConfParams(const ConfParams &newParams);
    static void asyncLearnNormalFlow();
    static AsyncLearnFlowStatus getLastLearnFlowStatus();
    static void resetLearnFlowStatus();
    static File getLogFileWithDate(time_t theDate);
    static File getMsgFileWithDate(time_t theDate);
    static bool isLogFileName(const String& str);
    static bool isMsgFileName(const String& str);
    static bool getLogfileDMY(const String& logStr, int& day, int& month, int&year);
    static bool getMsgfileDMY(const String& msgStr, int& day, int& month, int&year);
    static File getMsgFileWithDateForRead(time_t theDate);
    static File getLogFileWithDateForRead(time_t theDate);
};



#endif
