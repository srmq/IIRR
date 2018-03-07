#ifndef _SENSOR_TASK_H_
#define _SENSOR_TASK_H_
#include "sensor_calibration.h"
#include <Scheduler.h>
#include "TimeKeeper.h"
#include "FS.h"
#include <ArduinoJson.h>
#include "ConfParams.h"
#include "WaterController.h"

enum MessageTypes {
  MSG_DEBUG,
  MSG_INFO,
  MSG_WARN,
  MSG_ERR
};

enum MessageCodes {
  MSG_INCONSIST_WATER_CURRSTATUS //this means that we were expecting a certain water 
                                 // current status, but another was found. First it
                                 // is logged the status found and after the one expected
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
  File getFSFileWithDate(time_t nowTime, PGM_P fmtStr, const int bufSize);

  bool doFSMaintenance(int keepDays);
  bool doFSMaintenance(int keepDays, const String& dirName, const String& commonName);

  ConfParams *readMainConfParams();

  static time_t confParamTime2Timet(const char* timeStr);

  static time_t toConfTimet(time_t aTime_t);

  static bool isInNoIrrigTime(time_t aTime);

  static bool isInHHMMConfInterval(time_t aTimeInConfTimet, time_t initHHMM, time_t endHHMM);  

  static unsigned int irrigTodayRemainingSecs();

  bool fulfillMinIrrigInterval(time_t aTime);

  TimeKeeper timeKeeper;

  WellPumpWaterController waterControl;

  inline static bool isValidMoisture(float percent) { return (percent >= 0) && (percent <= 100); }

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

};

#endif
