#include "SensorTask.h"
#include "global_funcs.h"
#include "FS.h"
#include <cctype>
#include <cstring>
#include <cstdlib>
#include <ArduinoJson.h>
#include "TimeKeeper.h"
#include "WaterController.h"

SensorDirection currSensorDirection;
long resistances[NUM_PROBES];
SoilMoisture moistures;
ConfParams mainConfParams;
IrrigData irrigData;
static bool requestLearn;
static AsyncLearnFlowStatus learnFlowStatus;

bool fsOpen = false;

static const char UNINPLEMENTED_CALL[] PROGMEM = "WARNING: Unimplemented call at ";
static const char PARAMS_JSON_FILE[] PROGMEM = "/conf/params.json";

static const char TS_FMT_HHMM[] PROGMEM = "%02d%02d";

static time_t lastLogWrite = 0;

void sortResistances() {
  long tmp;
  int i,j;
  for(i = 0; i < NUM_PROBES-1; i++) {
    for(j = i + 1; j < NUM_PROBES; j++) {
      if ( resistances[j] < resistances[i] ) {
        tmp = resistances[i];
        resistances[i] = resistances[j];
        resistances[j] = tmp;
      }
    }
  }
}

time_t SensorTask::toConfTimet(time_t aTime_t) {
  char timeStr[5];
  timeStr[4] = '\0';
  
  snprintf_P(timeStr, 5, TS_FMT_HHMM, TimeKeeper::tkHour(aTime_t), TimeKeeper::tkMinute(aTime_t));
  return confParamTime2Timet(timeStr);
  
}

bool SensorTask::fulfillMinIrrigInterval(time_t aTime) {
  bool result = true;
  if (irrigData.lastIrrigEnd != 0) {
    if (irrigData.lastIrrigEnd < aTime) {
      result = (aTime - irrigData.lastIrrigEnd) > (60*mainConfParams.irrMIntervMins);
    }
  }
  return result;
}

unsigned int SensorTask::irrigTodayRemainingSecs() {
  unsigned int remainSeconds = mainConfParams.irrMaxTimeDaySeconds;
  time_t nowTime = TimeKeeper::tkNow();
  if (irrigData.isIrrigating) {
    if (irrigData.irrigSince != 0) {
      unsigned int irrigSeconds = nowTime - irrigData.irrigSince;
      if (irrigSeconds >= remainSeconds) {
        remainSeconds = 0;
      } else {
        remainSeconds -= irrigSeconds;
      }
    }
  } 
  if (irrigData.lastIrrigEnd != 0) {
    if (TimeKeeper::isSameDate(nowTime, irrigData.lastIrrigEnd)) {
        if (irrigData.irrigTodaySecs >= remainSeconds) {
          remainSeconds = 0;
        } else {
          remainSeconds -= irrigData.irrigTodaySecs;
        }
    }
  }
  return remainSeconds;
}

bool SensorTask::isInHHMMConfInterval(time_t aTimeInConfTimet, time_t initHHMM, time_t endHHMM) {
  if (!mainConfParams.isEmptyInterval(initHHMM, endHHMM) 
      && mainConfParams.isValidOrEmptyInterval(initHHMM, endHHMM)) {
        
    if(aTimeInConfTimet >= initHHMM && aTimeInConfTimet <= endHHMM)
      return true;
  }

  return false; 
}

bool SensorTask::isInNoIrrigTime(time_t aTime) {
  time_t timeInConfTimet = SensorTask::toConfTimet(aTime);

  return isInHHMMConfInterval(timeInConfTimet, mainConfParams.noIrrTime0Init, mainConfParams.noIrrTime0End)
      || isInHHMMConfInterval(timeInConfTimet, mainConfParams.noIrrTime1Init, mainConfParams.noIrrTime1End)
      || isInHHMMConfInterval(timeInConfTimet, mainConfParams.noIrrTime2Init, mainConfParams.noIrrTime2End)
      || isInHHMMConfInterval(timeInConfTimet, mainConfParams.noIrrTime3Init, mainConfParams.noIrrTime3End);
}

//timeStr is HHMM
//output time_t is this time at Jan 1st 2017 (0 secs)
time_t SensorTask::confParamTime2Timet(const char* timeStr) {
  time_t result = 0;
  if ( (strlen(timeStr) != 4)) return 0;
  if ( timeStr[0] < '0' || timeStr[0] > '2') return 0;
  if ( timeStr[0] == '2') {
    if (timeStr[1] < '0' || timeStr[1] > '3') return 0;
  }
  if (timeStr[1] < '0' || timeStr[1] > '9') return 0;
  if (timeStr[2] > '5' || timeStr[2] < '0') return 0;
  if (timeStr[3] < '0' || timeStr[3] > '9') return 0;

  tmElements_t timeElements;
  timeElements.Second = 0;
  char buf[3];
  buf[2] = '\0';
  strncpy(buf, timeStr, 2);
  timeElements.Hour = strtol(buf, NULL, 10);

  strncpy(buf, timeStr+2, 2);
  timeElements.Minute = strtol(buf, NULL, 10);

  timeElements.Wday = 0;
  timeElements.Year = 2017 - 1970;
  timeElements.Month = 1;
  timeElements.Day = 1;

    // tm.Second  Seconds   0 to 59
    // tm.Minute  Minutes   0 to 59
    // tm.Hour    Hours     0 to 23
    // tm.Wday    Week Day  0 to 6  (not needed for mktime)
    // tm.Day     Day       1 to 31
    // tm.Month   Month     1 to 12
    // tm.Year    Year      0 to 99 (offset from 1970)    
  result = TimeKeeper::tkMakeTime(timeElements);    
  return result;
}

JsonObject& SensorTask::createJsonFromConfParams(DynamicJsonBuffer& jsonBuffer) {
  JsonObject& root = jsonBuffer.createObject();

  JsonArray& noirrtimes = root.createNestedArray("noirrtimes");
  char strBuf[5];
  strBuf[4] = '\0';
  
  snprintf_P(strBuf, 5, TS_FMT_HHMM, TimeKeeper::tkHour(mainConfParams.noIrrTime0Init), TimeKeeper::tkMinute(mainConfParams.noIrrTime0Init));
  noirrtimes.add(strBuf);
  snprintf_P(strBuf, 5, TS_FMT_HHMM, TimeKeeper::tkHour(mainConfParams.noIrrTime0End), TimeKeeper::tkMinute(mainConfParams.noIrrTime0End));
  noirrtimes.add(strBuf);

  snprintf_P(strBuf, 5, TS_FMT_HHMM, TimeKeeper::tkHour(mainConfParams.noIrrTime1Init), TimeKeeper::tkMinute(mainConfParams.noIrrTime1Init));
  noirrtimes.add(strBuf);
  snprintf_P(strBuf, 5, TS_FMT_HHMM, TimeKeeper::tkHour(mainConfParams.noIrrTime1End), TimeKeeper::tkMinute(mainConfParams.noIrrTime1End));
  noirrtimes.add(strBuf);
  
  snprintf_P(strBuf, 5, TS_FMT_HHMM, TimeKeeper::tkHour(mainConfParams.noIrrTime2Init), TimeKeeper::tkMinute(mainConfParams.noIrrTime2Init));
  noirrtimes.add(strBuf);
  snprintf_P(strBuf, 5, TS_FMT_HHMM, TimeKeeper::tkHour(mainConfParams.noIrrTime2End), TimeKeeper::tkMinute(mainConfParams.noIrrTime2End));
  noirrtimes.add(strBuf);
  
  snprintf_P(strBuf, 5, TS_FMT_HHMM, TimeKeeper::tkHour(mainConfParams.noIrrTime3Init), TimeKeeper::tkMinute(mainConfParams.noIrrTime3Init));
  noirrtimes.add(strBuf);
  snprintf_P(strBuf, 5, TS_FMT_HHMM, TimeKeeper::tkHour(mainConfParams.noIrrTime3End), TimeKeeper::tkMinute(mainConfParams.noIrrTime3End));
  noirrtimes.add(strBuf);

  root["irrslot"] = mainConfParams.irrSlotSeconds;
  root["irrmaxtimeday"] = mainConfParams.irrMaxTimeDaySeconds;
  root["irrminterv"] = mainConfParams.irrMIntervMins;
  root["critlevel"] = mainConfParams.critLevel;
  root["satlevel"] = mainConfParams.satLevel;
  root["normpulses"] = mainConfParams.normalPulsesPerSec;

  return root;  
}

bool SensorTask::updateConfParams(const ConfParams &newParams) {
  bool allOk = true;
  if (&newParams != &mainConfParams) {
    memcpy(&mainConfParams, &newParams, sizeof(ConfParams));    
  }
  String fileName = String(FPSTR(PARAMS_JSON_FILE));
  File confFile = SPIFFS.open(fileName, "w");
  if (!confFile) return false;
  const size_t bufferSize = JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(7);
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = createJsonFromConfParams(jsonBuffer);

  root.printTo(confFile);
  confFile.flush();
  confFile.close();
  
  return allOk;
}

void SensorTask::updateConfParamsFromJson(ConfParams& confStruct, JsonObject& jsonConfParamsRoot) {
  JsonArray& noirrtimes = jsonConfParamsRoot["noirrtimes"];
  confStruct.noIrrTime0Init = confParamTime2Timet(noirrtimes[0]);
  confStruct.noIrrTime0End = confParamTime2Timet(noirrtimes[1]);

  confStruct.noIrrTime1Init = confParamTime2Timet(noirrtimes[2]);
  confStruct.noIrrTime1End = confParamTime2Timet(noirrtimes[3]);

  confStruct.noIrrTime2Init = confParamTime2Timet(noirrtimes[4]);
  confStruct.noIrrTime2End = confParamTime2Timet(noirrtimes[5]);

  confStruct.noIrrTime3Init = confParamTime2Timet(noirrtimes[6]);
  confStruct.noIrrTime3End = confParamTime2Timet(noirrtimes[7]);

  confStruct.irrSlotSeconds = jsonConfParamsRoot["irrslot"];
  confStruct.irrMaxTimeDaySeconds = jsonConfParamsRoot["irrmaxtimeday"]; 
  confStruct.irrMIntervMins = jsonConfParamsRoot["irrminterv"];
  confStruct.critLevel = jsonConfParamsRoot["critlevel"]; 
  confStruct.satLevel = jsonConfParamsRoot["satlevel"];
  confStruct.normalPulsesPerSec = jsonConfParamsRoot["normpulses"];
  
}

static const char IRRIGDATA_JSON_FILE[] PROGMEM = "/var/irrig.json";

bool readFSIrrigData(IrrigData& data) {
  if (!fsOpen) return false;
  String fileName = String(FPSTR(IRRIGDATA_JSON_FILE));
  File irrigFile = SPIFFS.open(fileName, "r");
  if (!irrigFile) return false;
  const size_t bufferSize = JSON_OBJECT_SIZE(2) + 60;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(irrigFile);
  data.lastIrrigEnd = (time_t) root["lastIrrigEnd"];
  data.irrigTodaySecs = (unsigned long) root["irrigTodaySecs"]; 
  irrigFile.close();  
  return true;
}

bool updateFSIrrigData(IrrigData& data) {
  if (!fsOpen) return false;
  String fileName = String(FPSTR(IRRIGDATA_JSON_FILE));
  File irrigFile = SPIFFS.open(fileName, "w+");
  if (!irrigFile) return false;
  const size_t bufferSize = JSON_OBJECT_SIZE(2);
  DynamicJsonBuffer jsonBuffer(bufferSize);

  JsonObject& root = jsonBuffer.createObject();
  root["lastIrrigEnd"] = data.lastIrrigEnd;
  root["irrigTodaySecs"] = data.irrigTodaySecs;
  root.printTo(irrigFile);
  
  irrigFile.close();
  return true;
}

ConfParams *SensorTask::readMainConfParams() {
  ConfParams *result = NULL;
  if (fsOpen) {
      String fileName = String(FPSTR(PARAMS_JSON_FILE));
      File confFile = SPIFFS.open(fileName, "r");
      if (!confFile) return NULL;
      const size_t bufferSize = JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(7) + 170;
      DynamicJsonBuffer jsonBuffer(bufferSize);
      JsonObject& root = jsonBuffer.parseObject(confFile);
      updateConfParamsFromJson(mainConfParams, root);
      result = &mainConfParams;
      confFile.close(); 
  }
  return result;
}

float SensorTask::readMoisture(SensorType sType) {
  float value = 1;
  int refVoltage, afterSensorVoltage;
  for (int i = 0; i < NUM_PROBES; i++) {
      resistances[i] = -1;
      refVoltage = readReferenceVoltage(sType, currSensorDirection);
      afterSensorVoltage = readAfterSensorVoltage(sType, currSensorDirection);
      #ifdef DEBUG_SENSOR_MODE
        Serial.print("Reference read was: ");
        Serial.println(refVoltage);
        Serial.print("After sensor voltage was: ");
        Serial.println(afterSensorVoltage);
      #endif
      if (afterSensorVoltage < MINAFTERVOLTAGE_OPENCIRCUIT) {
        value = AFTERVOLTAGE_OPEN;
      } else if ((refVoltage - afterSensorVoltage) < SHORTCIRCUIT_SENSOR_MINDIFF){
        value = SENSOR_SHORTCIRCUIT;
      } else {
        resistances[i] = long( double(REFERENCE_RESISTOR) * ( refVoltage - afterSensorVoltage ) / afterSensorVoltage + 0.5 );
        #ifdef DEBUG_SENSOR_MODE
          Serial.print("Resistance: ");
          Serial.println(resistances[i]);
        #endif 
        switchSensorDirection();
        this->delay(SIGNAL_DELAY);
      }
      yield();
  }
  sortResistances();
  const long medianResistance = resistances[NUM_PROBES/2];
  if (medianResistance > 0) {
    value = float(exp((log(MOISTURE_MULX)-log(medianResistance))/(MOISTURE_EXPFACT)));
    if (value > 1) value = 1;
  } else if (value > 0) {
    value = MOISTURE_READERROR;
  }
  
  return value;
}



static const char TS_FMT_STR[] PROGMEM = "%04d%02d%02dT%02d%02d%02d"; //yyyymmddThhmmss
static const char LOGF_FMT_STR[] PROGMEM = "/logs/sensor%04d%02d%02d.txt"; //yyyymmdd
static const char MSGF_FMT_STR[] PROGMEM = "/logs/msg%04d%02d%02d.txt"; //yyyymmdd

const char LOG_DIR[] PROGMEM = "/logs";
static const char SENSORLOG_COMMON_NAME[] PROGMEM = "/logs/sensor";
static const char MSGLOG_COMMON_NAME[] PROGMEM = "/logs/msg";
static const char STR_FMT_STR[] PROGMEM = "%s";

bool SensorTask::doFSMaintenance(int keepDays, const String& dirName, const String& commonName) {
  time_t nowTime = this->timeKeeper.tkNow();
  Dir logDir = SPIFFS.openDir(dirName);
  tmElements_t timeElements;
  timeElements.Second = 59;
  timeElements.Minute = 59;
  timeElements.Hour = 23;
  timeElements.Wday = 0;
    // tm.Second  Seconds   0 to 59
    // tm.Minute  Minutes   0 to 59
    // tm.Hour    Hours     0 to 23
    // tm.Wday    Week Day  0 to 6  (not needed for mktime)
    // tm.Day     Day       1 to 31
    // tm.Month   Month     1 to 12
    // tm.Year    Year      0 to 99 (offset from 1970)  
  bool allOk = true;
  while(logDir.next()) {
    String fileName = logDir.fileName();
    if (fileName.startsWith(commonName)) {
      String year = fileName.substring(commonName.length(), commonName.length()+4);
      String month = fileName.substring(commonName.length()+4, commonName.length()+4+2);
      String day = fileName.substring(commonName.length()+6, commonName.length()+6+2);
      int tmYear = (int)year.toInt();
      if (tmYear == 0) return false;
      tmYear -= 1970;
      int tmMonth = (int)month.toInt();
      if (tmMonth == 0) return false;
      int tmDay = (int)day.toInt();
      if (tmDay == 0) return false;
      timeElements.Year = tmYear;
      timeElements.Month = tmMonth;
      timeElements.Day = tmDay;
      time_t logFileTime = this->timeKeeper.tkMakeTime(timeElements);
      int elapsedDays = (int) this->timeKeeper.tkElapsedDays(logFileTime, nowTime);
      if (elapsedDays > keepDays) {
        allOk = allOk && SPIFFS.remove(fileName);
      }
    }
  }
  return allOk; 
}

bool SensorTask::doFSMaintenance(int keepDays) {
  return doFSMaintenance(keepDays, String(FPSTR(LOG_DIR)), String(FPSTR(SENSORLOG_COMMON_NAME))) 
      && doFSMaintenance(keepDays, String(FPSTR(LOG_DIR)), String(FPSTR(MSGLOG_COMMON_NAME)));
}

File SensorTask::getFSFileWithDate(time_t nowTime, PGM_P fmtStr, const int bufSize) {
  File logFile;
  if (fsOpen) {
    char logFName[bufSize];
    memset(logFName, '\0', bufSize*sizeof(char)); 
    
    snprintf_P(logFName, bufSize, fmtStr, this->timeKeeper.tkYear(nowTime), this->timeKeeper.tkMonth(nowTime), this->timeKeeper.tkDay(nowTime));

    if(lastLogWrite == 0) {
      if (this->timeKeeper.isValidTS(nowTime)) {
        doFSMaintenance(FS_LOG_KEEP_DAYS);
        logFile = SPIFFS.open(logFName, "a+");
      } else {
        logFile = SPIFFS.open(logFName, "w+");
      }
    } else {
      if (this->timeKeeper.tkDay(nowTime) == this->timeKeeper.tkDay(lastLogWrite)) {
        logFile = SPIFFS.open(logFName, "a+");
      } else {
        doFSMaintenance(FS_LOG_KEEP_DAYS);
        logFile = SPIFFS.open(logFName, "w+");
      }
    }
  }
  return logFile; //test if this returned object is true, if false no valid file
                  //has been returned  
}

File SensorTask::getCurrLogFile(time_t nowTime) {
  return getFSFileWithDate(nowTime, LOGF_FMT_STR, 33);
}

File SensorTask::getCurrMsgFile(time_t nowTime) {
  return getFSFileWithDate(nowTime, MSGF_FMT_STR, 33);
}

// the loop function runs over and over again forever
void SensorTask::loopSensorMode() {
  if (requestLearn) {
    requestLearn = false;
    learnFlowStatus = LFLOW_INPROGRESS;
    const bool configureOk = this->waterControl.configureFlow();
    if (configureOk) {
      learnFlowStatus = LFLOW_DONE;
    } else {
      learnFlowStatus = LFLOW_ERROR;
    }
  }
  float moistureSurface = readMoisture(SURFACE);
  Serial.print(">>>> SURFACE Moisture: ");
  Serial.println(moistureSurface);
  yield();

  float moistureMiddle = readMoisture(MIDDLE);
  Serial.print(">>>> MIDDLE Moisture: ");
  Serial.println(moistureMiddle);
  yield();

  float moistureDeep = readMoisture(DEEP);
  Serial.print(">>>> DEEP Moisture: ");
  Serial.println(moistureDeep);
  moistures.surface = moistureSurface;
  moistures.middle = moistureMiddle;
  moistures.deep = moistureDeep;
  
  //moistures.hasWater = isWithWater();
  //FIXME
  //aqui verificar regra de irrigacao
  
  moistures.timeStamp = this->timeKeeper.tkNow();
  char tsStr[16];
  snprintf_P(tsStr, 16, TS_FMT_STR, this->timeKeeper.tkYear(moistures.timeStamp), this->timeKeeper.tkMonth(moistures.timeStamp), this->timeKeeper.tkDay(moistures.timeStamp), this->timeKeeper.tkHour(moistures.timeStamp), this->timeKeeper.tkMinute(moistures.timeStamp), this->timeKeeper.tkSecond(moistures.timeStamp));

  if ((lastLogWrite == 0)  || ((moistures.timeStamp - lastLogWrite) > FS_LOG_WRITE_INTERVAL)) {
    File logFile = getCurrLogFile(moistures.timeStamp);
    if (logFile) {
      logFile.print(tsStr);
      logFile.print(',');
      logFile.print(moistures.surface);
      logFile.print(',');
      logFile.print(moistures.middle);
      logFile.print(',');
      logFile.print(moistures.deep);
      logFile.print(',');
      logFile.println(irrigData.isIrrigating ? '1' : '0');
      logFile.flush();
      logFile.close();
      lastLogWrite = moistures.timeStamp;
    }
  }  

  /*
  if(moistures.hasWater) {
    Serial.println("Habemus agua! :-D");
  } else {
    Serial.println("No water! :-(");
  } FIXME
  */

  Serial.println("");
}

static const char SELECT_SENSOR_NAME[] PROGMEM =  "SensorTask::selectSensor()";
void SensorTask::selectSensor(SensorInput sensor) {
  switch(sensor) {
    case SURFACE_LEFT:
       digitalWrite(MULA_PIN, LOW);
       digitalWrite(MULB_PIN, LOW);
       digitalWrite(MULC_PIN, LOW);
       break;
    case SURFACE_RIGHT:
       digitalWrite(MULA_PIN, HIGH);
       digitalWrite(MULB_PIN, LOW);
       digitalWrite(MULC_PIN, LOW);
       break;
    case MIDDLE_LEFT:
       digitalWrite(MULA_PIN, LOW);
       digitalWrite(MULB_PIN, HIGH);
       digitalWrite(MULC_PIN, LOW);
       break;
    case MIDDLE_RIGHT:
       digitalWrite(MULA_PIN, HIGH);
       digitalWrite(MULB_PIN, HIGH);
       digitalWrite(MULC_PIN, LOW);
       break;
    case DEEP_LEFT:
       digitalWrite(MULA_PIN, LOW);
       digitalWrite(MULB_PIN, LOW);
       digitalWrite(MULC_PIN, HIGH);
       break;
    case DEEP_RIGHT:
       digitalWrite(MULA_PIN, HIGH);
       digitalWrite(MULB_PIN, LOW);
       digitalWrite(MULC_PIN, HIGH);
       break;    
    default:
      Serial.print(FPSTR(UNINPLEMENTED_CALL));
      Serial.println(FPSTR(SELECT_SENSOR_NAME));
      break;
  }
}
  
SensorDirection SensorTask::switchSensorDirection() {
  switch(currSensorDirection) {
    case LEFT:
      currSensorDirection = RIGHT;
      break;
    case RIGHT:
      currSensorDirection = LEFT;
      break;
  }
  return currSensorDirection;
}
  
/*
bool SensorTask::isWithWater() {
  short votes = 0;
  digitalWrite(DECOD_A0_PIN, LOW);
  digitalWrite(DECOD_A1_PIN, HIGH);
  digitalWrite(DECOD_A2_PIN, HIGH);
  enableMulAndDecod();
  for (int i = 0; i < NUM_PROBES; i++) {
    this->delay(SIGNAL_DELAY);
    const int levelRead = digitalRead(NOWATER_SWITCH_PIN);
    if (levelRead == HIGH) {
      votes++;
    } else {
      votes--;
    }
  }
  disableMulAndDecod();
  return (votes > 0);
}
*/

static const char ENABLE_SENSOR_VOLTAGE_NAME[] PROGMEM = "SensorTask::enableSensorVoltage() ";
static const char SENSORTYPE_NAME[] PROGMEM = "SensorType ";
static const char SENSORDIRECTION_NAME[] PROGMEM = "SensorDirection ";

void SensorTask::enableSensorVoltage(SensorType sType, SensorDirection sDir) {
  switch(sType) {
    case SURFACE:
      switch(sDir) {
        case LEFT:
          digitalWrite(DECOD_A0_PIN, LOW);
          digitalWrite(DECOD_A1_PIN, LOW);
          digitalWrite(DECOD_A2_PIN, LOW);
          break;
        case RIGHT:
          digitalWrite(DECOD_A0_PIN, HIGH);
          digitalWrite(DECOD_A1_PIN, LOW);
          digitalWrite(DECOD_A2_PIN, LOW);
          break;  
        default:
          Serial.print(FPSTR(UNINPLEMENTED_CALL));
          Serial.print(FPSTR(ENABLE_SENSOR_VOLTAGE_NAME));
          Serial.println(FPSTR(SENSORDIRECTION_NAME));
          break;
      }
      break;
    case MIDDLE:
      switch(sDir) {
        case LEFT:
          digitalWrite(DECOD_A0_PIN, LOW);
          digitalWrite(DECOD_A1_PIN, HIGH);
          digitalWrite(DECOD_A2_PIN, LOW);
          break;
        case RIGHT:
          digitalWrite(DECOD_A0_PIN, HIGH);
          digitalWrite(DECOD_A1_PIN, HIGH);
          digitalWrite(DECOD_A2_PIN, LOW);
          break;  
        default:
          Serial.print(FPSTR(UNINPLEMENTED_CALL));
          Serial.print(FPSTR(ENABLE_SENSOR_VOLTAGE_NAME));
          Serial.println(FPSTR(SENSORDIRECTION_NAME));
          break;
      }
      break;
    case DEEP:
      switch(sDir) {
        case LEFT:
          digitalWrite(DECOD_A0_PIN, LOW);
          digitalWrite(DECOD_A1_PIN, LOW);
          digitalWrite(DECOD_A2_PIN, HIGH);
          break;
        case RIGHT:
          digitalWrite(DECOD_A0_PIN, HIGH);
          digitalWrite(DECOD_A1_PIN, LOW);
          digitalWrite(DECOD_A2_PIN, HIGH);
          break;  
        default:
          Serial.print(FPSTR(UNINPLEMENTED_CALL));
          Serial.print(FPSTR(ENABLE_SENSOR_VOLTAGE_NAME));
          Serial.println(FPSTR(SENSORDIRECTION_NAME));
          break;
      }
      break;
      
    default:
      Serial.print(FPSTR(UNINPLEMENTED_CALL));
      Serial.print(FPSTR(ENABLE_SENSOR_VOLTAGE_NAME));
      Serial.println(FPSTR(SENSORTYPE_NAME));
      break;
  }
}

static const char READ_REFERENCE_VOLTAGE_NAME[] PROGMEM = "SensorTask::readReferenceVoltage() ";  

int SensorTask::readVoltage(SensorType sType, SensorDirection sDirection, SensorInput sensorInput) {
  int readVal;
  enableSensorVoltage(sType, sDirection);
  selectSensor(sensorInput);
  enableMulAndDecod();
  this->delay(SIGNAL_DELAY);
  readVal = analogRead(ANALOG_PIN);
  disableMulAndDecod();
  this->delay(SIGNAL_DELAY);        
  return readVal;
}

int SensorTask::readReferenceVoltage(SensorType sType, SensorDirection sDir) {
  int readVal = -1;
  switch(sType) {
    case SURFACE:
      switch(sDir) {
        case LEFT:
          readVal = readVoltage(SURFACE, LEFT, SURFACE_LEFT);
          break;
        case RIGHT:
          readVal = readVoltage(SURFACE, RIGHT, SURFACE_RIGHT);
          break;
      }
      break;
    case MIDDLE:
      switch(sDir) {
        case LEFT:
          readVal = readVoltage(MIDDLE, LEFT, MIDDLE_LEFT);
          break;
        case RIGHT:
          readVal = readVoltage(MIDDLE, RIGHT, MIDDLE_RIGHT);
          break;
      }
      break;
    case DEEP:
      switch(sDir) {
        case LEFT:
          readVal = readVoltage(DEEP, LEFT, DEEP_LEFT);
          break;
        case RIGHT:
          readVal = readVoltage(DEEP, RIGHT, DEEP_RIGHT);
          break;
      }
      break;      
    default:
      Serial.print(FPSTR(UNINPLEMENTED_CALL));
      Serial.println(FPSTR(READ_REFERENCE_VOLTAGE_NAME));
      break; 
  }
  return readVal;
}

static const char READ_AFTERSENSOR_VOLTAGE_NAME[] PROGMEM = "SensorTask::readAfterSensorVoltage() ";    
int SensorTask::readAfterSensorVoltage(SensorType sType, SensorDirection sDir) {
  int readVal = -1;
  switch(sType) {
    case SURFACE:
      switch(sDir) {
        case LEFT:
          readVal = readVoltage(SURFACE, LEFT, SURFACE_RIGHT); //should enable reference at left but read at right
          break;
        case RIGHT:
          readVal = readVoltage(SURFACE, RIGHT, SURFACE_LEFT);
          break;
      }
      break;
    case MIDDLE:
      switch(sDir) {
        case LEFT:
          readVal = readVoltage(MIDDLE, LEFT, MIDDLE_RIGHT);
          break;
        case RIGHT:
          readVal = readVoltage(MIDDLE, RIGHT, MIDDLE_LEFT);
          break;
      }
      break;
    case DEEP:
      switch(sDir) {
        case LEFT:
          readVal = readVoltage(DEEP, LEFT, DEEP_RIGHT);
          break;
        case RIGHT:
          readVal = readVoltage(DEEP, RIGHT, DEEP_LEFT);
          break;
      }
      break;
    default:
      Serial.print(FPSTR(UNINPLEMENTED_CALL));
      Serial.println(FPSTR(READ_AFTERSENSOR_VOLTAGE_NAME));
      break; 
  }
  return readVal;
}

void SensorTask::loop()  {
  loopSensorMode();
  unsigned long timeBeforeTest = millis();
  if (irrigData.isIrrigating) {
    while((millis() - timeBeforeTest) < SENSOR_READ_DELAY) {
      //verificando status da irrigacao
      WaterCurrSensorStatus statusWater = this->waterControl.currStatus();
      if(statusWater != WATER_CURRFLOWING) {
        //something wrong, log it
        time_t timeStamp = TimeKeeper::tkNow();
        char tsStr[16];
        snprintf_P(tsStr, 16, TS_FMT_STR, this->timeKeeper.tkYear(timeStamp), this->timeKeeper.tkMonth(timeStamp), this->timeKeeper.tkDay(timeStamp), this->timeKeeper.tkHour(timeStamp), this->timeKeeper.tkMinute(timeStamp), this->timeKeeper.tkSecond(timeStamp));
        File logFile = getCurrMsgFile(timeStamp);
        if (logFile) {
          logFile.print(tsStr);
          logFile.print(',');
          logFile.print(MSG_ERR);
          logFile.print(',');
          logFile.print(MSG_INCONSIST_WATER_CURRSTATUS);
          logFile.print(',');
          logFile.print(statusWater); //status was this
          logFile.print(',');
          logFile.println(WATER_CURRFLOWING); //but should be that
          logFile.flush();
          logFile.close();
        }
        
      }
      //LIGAR O RESULTADO DE SE TEM AGUA NO MULTIPLEXADOR DE ENTRADA E LIBERAR O PINO
      //QUE ESTA SENDO USADO PARA LIGAR A BOMBA
      //ALEM DISSO TEM  O D8 QUE SERA USADO PARA LIGAR O SENSOR
      //LIGAR O SENSOR SEPARADO DA BOMBA PERMITE VERIFICAR SE ELA DESLIGOU DE VERDADE
      //VERIFICAR COMO DEVE SER A LIGAÇÃO DO RELE DE ESTADO SOLIDO, TENSAO, CORRENTE ETC.
      
    }
  } else {
    this->delay(SENSOR_READ_DELAY);
  }
  this->timeKeeper.syncTime();
}

SensorTask::SensorTask() : Task() {
//  pinMode(NOWATER_SWITCH_PIN, INPUT);
  pinMode(FLOWSIGNAL_PIN, INPUT);
  pinMode(PUMP_PIN, OUTPUT);
  digitalWrite(PUMP_PIN, LOW);
  pinMode(DECOD_A0_PIN, OUTPUT);
  pinMode(DECOD_A1_PIN, OUTPUT);
  pinMode(DECOD_A2_PIN, OUTPUT);
  
  currSensorDirection = LEFT;
  pinMode(MUL_DECOD_INHIB_PIN, OUTPUT);
  disableMulAndDecod();
  pinMode(MULA_PIN, OUTPUT);
  pinMode(MULB_PIN, OUTPUT);
  pinMode(MULC_PIN, OUTPUT);
  fsOpen = SPIFFS.begin();
  if (!fsOpen) {
    Serial.println(F("WARNING: filesystem open failed!"));
  } else {
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    Serial.println(F("INFO: Filesystem"));
    Serial.print(F("Total bytes: "));
    Serial.println(fs_info.totalBytes);
    Serial.print(F("Used bytes: "));
    Serial.println(fs_info.usedBytes);
    Serial.print(F("Block size: "));
    Serial.println(fs_info.blockSize);
    Serial.print(F("Page size: "));
    Serial.println(fs_info.pageSize);
    Serial.print(F("Max open files: "));
    Serial.println(fs_info.maxOpenFiles);
    Serial.print(F("Max path length: "));
    Serial.println(fs_info.maxPathLength);

    ConfParams *confReadParams = readMainConfParams(); 
    if (confReadParams != NULL) {
      Serial.println(F("INFO: Main configuration parameters successfully read"));
    } else {
      Serial.println(F("ERROR: Unable to read main configuration parameters"));
    }

    if (!readFSIrrigData(irrigData)) {
      Serial.println(F("WARNING: Unable to read irrigData"));
    } else {
      Serial.println(F("INFO: irrigData successfully read"));
    }
  }
  requestLearn = false;
  learnFlowStatus = LFLOW_NOTREQUESTED;
  static NTPClient::DelayHandlerFunction delayHandler = std::bind(SensorTask::multiTaskDelay, this, std::placeholders::_1);
  this->timeKeeper.setDelayFunction(delayHandler);
  this->waterControl.setDelayFunction(delayHandler);
  
  Serial.println(F("SETUP SENSOR MODE"));
}

void SensorTask::multiTaskDelay(SensorTask *taskServer, unsigned long ms) {
  taskServer->delay(ms);
}

void SensorTask::asyncLearnNormalFlow() {
  requestLearn = true;
}

AsyncLearnFlowStatus SensorTask::getLastLearnFlowStatus() {
  return learnFlowStatus;
}

void SensorTask::resetLearnFlowStatus() {
  learnFlowStatus = LFLOW_NOTREQUESTED;
}

