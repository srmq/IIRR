/**
 * IIRR -- Intelligent Irrigator Based on ESP8266
    Copyright (C) 2016--2019  Sergio Queiroz <srmq@cin.ufpe.br>

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
#include "global_funcs.h"
#include "CloudTask.h"
#include "ServerTask.h"
#include "LineLimitedReadStream.h"
#include "HttpDateParser.h"
#include "FS.h"
#include "SensorTask.h"
#include <pgmspace.h>
#include <Arduino.h>
#include <cstring>
#include <cstdlib>
#include <cctype>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "MyHttpClient.h"


static const char CPARAMS_JSON_FILE[] PROGMEM = "/conf/cparams.json";

static const char DATALOG_SENDPARAMS_URL[] PROGMEM = "/v100/datalog/send-params";
static const char MSGLOG_SENDPARAMS_URL[] PROGMEM = "/v100/msglog/send-params";
static const char AUTH_HEADER[] PROGMEM = "WWW-Authenticate";
static const char DATALOG_SENDCSV_URL[] PROGMEM = "/v100/datalog/send";
static const char MSGLOG_SENDCSV_URL[] PROGMEM = "/v100/msglog/send";
static const char GEN204_URL[] PROGMEM = "/v100/generate_204"; 
static const char TEST_CONN1_URL[] PROGMEM = "http://clients3.google.com/generate_204";
static const char TEST_CONN2_URL[] PROGMEM = "http://g.cn/generate_204";
static const char USER_AGENT_STR[] PROGMEM = "Android";




bool CloudTask::confAvailable = false;

static const char FILEDATE_FMT_STR[] PROGMEM = "%04d%02d%02d"; //yyyymmdd


CloudConf::CloudConf() {
  memset(this->baseUrl, 0, sizeof(this->baseUrl));
  memset(this->certHash, 0, sizeof(this->certHash));
  memset(this->login, 0, sizeof(this->login));
  memset(this->pass, 0, sizeof(this->pass));
  this->enabled = 0;
}

CloudTask::CloudTask() : Task(), firstRun(true), lastCheck(TimeKeeper::tkNow()), 
    sentAllDataLogUntilToday(false), sentAllMsgLogUntilToday(false) {
  String confFileName = String(FPSTR(CPARAMS_JSON_FILE));
  if (fsOpen) {
    if(SPIFFS.exists(confFileName)) {
          CloudTask::confAvailable = true;
    }
  }
  //this->lastTSDataSent = TimeKeeper::firstValidTime();
  //this->lastTSMsgSent = this->lastTSDataSent;
}

static String exractParam(String& authReq, const String& param, const char delimit) {
  int _begin = authReq.indexOf(param);
  if (_begin == -1) {
    return "";
  }
  return authReq.substring(_begin + param.length(), authReq.indexOf(delimit, _begin + param.length()));
}

static String getCNonce(const int len) {
  static const char alphanum[] =
    "0123456789"
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz";
  String s = "";

  for (int i = 0; i < len; ++i) {
    s += alphanum[rand() % (sizeof(alphanum) - 1)];
  }

  return s;
}

String CloudTask::getDigestAuth(String& authReq, const String& username, const String& password, const String& uri, unsigned int counter, const String& method) {
  // extracting required parameters for RFC 2069 simpler Digest
  String realm = exractParam(authReq, "realm=\"", '"');
  String nonce = exractParam(authReq, "nonce=\"", '"');
  String cNonce = getCNonce(8);

  char nc[9];
  snprintf(nc, sizeof(nc), "%08x", counter);

  // parameters for the RFC 2617 newer Digest
  MD5Builder md5;
  md5.begin();
  md5.add(username + ":" + realm + ":" + password);  // md5 of the user:realm:user
  md5.calculate();
  String h1 = md5.toString();

  md5.begin();
  md5.add(method + ":" + uri);
  md5.calculate();
  String h2 = md5.toString();

  md5.begin();
  md5.add(h1 + ":" + nonce + ":" + String(nc) + ":" + cNonce + ":" + "auth" + ":" + h2);
  md5.calculate();
  String response = md5.toString();

  String authorization = "Digest username=\"" + username + "\", realm=\"" + realm + "\", nonce=\"" + nonce +
                         "\", uri=\"" + uri + "\", algorithm=\"MD5\", qop=auth, nc=" + String(nc) + ", cnonce=\"" + cNonce + "\", response=\"" + response + "\"";
  Serial.println(authorization);
  return authorization;
}

static String getFirstDigitSubstr(const String& str) {
  int digitStartAt = -1;
  for (int i = 0;  i < str.length(); i++) {
    if (isdigit(str.charAt(i))) {
      digitStartAt = i;
      break; 
    }
  }
  if(digitStartAt == -1) return String("");
  int subEnd = digitStartAt + 1;
  while (subEnd < str.length()) {
    if (isdigit(str.charAt(subEnd))) {
      subEnd++;  
    } else {
      break;
    }
  }
  return str.substring(digitStartAt, subEnd);
}

bool CloudTask::getDatesToOpen(time_t &msgDate, time_t &logDate, time_t lastRepTSLog, time_t lastRepTSMsg, bool checkLog, bool checkMsg) {
  tmElements_t outTm;
  TimeKeeper::tkBreakTime(lastRepTSLog, outTm);
  outTm.Second = 0;
  outTm.Minute = 0;
  outTm.Hour = 0;
  time_t lastDateLog = TimeKeeper::tkMakeTime(outTm);

  TimeKeeper::tkBreakTime(lastRepTSMsg, outTm);
  outTm.Second = 0;
  outTm.Minute = 0;
  outTm.Hour = 0;
  time_t lastDateMsg = TimeKeeper::tkMakeTime(outTm);
  if(!fsOpen) return false;
  

  String logDirStr = String(FPSTR(LOG_DIR));
  Dir logDir = SPIFFS.openDir(logDirStr);
  time_t initDateLog = 0;
  time_t initMsgLog = 0;

  bool finishedMsg = !checkMsg;
  bool finishedLog = !checkLog;
  while(logDir.next() && !(finishedMsg && finishedLog)) {
    String fileName = logDir.fileName();
    if (SensorTask::isLogFileName(fileName) && !finishedLog) {
      int year, month, day;
      if(SensorTask::getLogfileDMY(fileName, day, month, year)) {
        outTm.Day = day;
        outTm.Month = month;
        outTm.Year = (year - 1970);
        outTm.Second = 0;
        outTm.Minute = 0;
        outTm.Hour = 0;
        time_t logFileTS = TimeKeeper::tkMakeTime(outTm);
        if (logFileTS >= lastDateLog && TimeKeeper::isValidTS(logFileTS)) {
          if (initDateLog == 0) {
            initDateLog = logFileTS;  
          } else if(logFileTS < initDateLog) {
            initDateLog = logFileTS; 
          }
        }
        if ((initDateLog != 0) && (initDateLog == lastDateLog)) {
          finishedLog = true;
        }
      }
    } else if (SensorTask::isMsgFileName(fileName) && !finishedMsg) {
      int year, month, day;
      if(SensorTask::getMsgfileDMY(fileName, day, month, year)) {
        outTm.Day = day;
        outTm.Month = month;
        outTm.Year = (year - 1970);
        outTm.Second = 0;
        outTm.Minute = 0;
        outTm.Hour = 0;
        time_t msgFileTS = TimeKeeper::tkMakeTime(outTm);
        if (msgFileTS >= lastDateMsg && TimeKeeper::isValidTS(msgFileTS)) {
          if (initMsgLog == 0) {
            initMsgLog = msgFileTS;  
          } else if(msgFileTS < initMsgLog) {
            initMsgLog = msgFileTS; 
          }
        }
        if ((initMsgLog != 0) && (initMsgLog == lastDateMsg)) {
          finishedMsg = true; 
        }
      }      
    }
  }
  logDate = initDateLog;
  msgDate = initMsgLog;
  return true;
}

static const char SSCANF_TSFORMAT[] PROGMEM = "%4d%2d%2dT%2d%2d%2d%*s";

int CloudTask::sendDataLogFromDate(time_t logDate, CloudConf& conf, SendParams &datalogSendParams, std::shared_ptr<String> &outPayLoadPtr, int &outHttpCode) {
  if(TimeKeeper::isValidTS(logDate)) {
    File logFile = SensorTask::getLogFileWithDateForRead(logDate);
    if(logFile) {
      char line[81];
      size_t pos;

      bool foundToSend = false;
      while(logFile.available() > 0 && !foundToSend) {
        pos = logFile.position();
        memset(line, '\0', sizeof(line));
        logFile.readBytesUntil('\n', line, 80);

        int year, month, day, hour, min, secs;
        String tsFmtStr = String(FPSTR(SSCANF_TSFORMAT));
        if (sscanf(line, tsFmtStr.c_str(), &year, &month, &day, &hour, &min, &secs) == 6) {
          Serial.println(F("logfile successfuly read"));
          time_t ts = TimeKeeper::tkMakeTime(year, month, day, hour, min, secs);
          if (ts > datalogSendParams.lastTS) {
            logFile.seek(pos, SeekSet);
            foundToSend = true;
          } else {
            Serial.print(F("log line: \""));
            Serial.print(line);
            Serial.println(F("\" was not after lastTS"));
          }
        } else {
          Serial.print(F("Read line with invalid TS at sendDataLogFromDate with filename: "));
          Serial.println(logFile.name());
          Serial.println(F("Line was:"));
          Serial.println(line);
        }
      }
      if(foundToSend) {
        String logCSVEntryPoint = String(FPSTR(DATALOG_SENDCSV_URL));
        Serial.println(F("Found datalog do send, now calling payloadPOST"));
        outPayLoadPtr = payloadPOST(conf, datalogSendParams.maxLines, logFile, logCSVEntryPoint, outHttpCode);
        if (outHttpCode != HTTP_CODE_OK) {
          Serial.print(F("WARNING: Not HTTP_CODE_OK returned when calling payloadPOST at sendDataLogFromDate: "));
          Serial.println(outHttpCode);
        } else {
          //FIXME TODO decodificar JSON apontado por payLoadPtr 
        }
        
      } else {
        logFile.close();
        return CLOUDTASK_SENDDATALOGFROMDATE_NOTHINGTOSEND;
      }
      logFile.close();      
    } else {
      return CLOUDTASK_SENDDATALOGFROMDATE_NOFILEWITHDATE;
    }
  } else {
      return CLOUDTASK_SENDDATALOGFROMDATE_INVALIDTS;
  }
  return CLOUDTASK_OK;
}


int CloudTask::sendMsgsFromDate(time_t msgDate, CloudConf& conf, SendParams &msglogSendParams, std::shared_ptr<String> &outPayLoadPtr, int &outHttpCode) {
  if(TimeKeeper::isValidTS(msgDate)) {
    File msgFile = SensorTask::getMsgFileWithDateForRead(msgDate);
    if(msgFile) {
      char line[81];
      size_t pos;

      bool foundToSend = false;
      while(msgFile.available() > 0 && !foundToSend) {
        pos = msgFile.position();
        memset(line, '\0', sizeof(line));
        msgFile.readBytesUntil('\n', line, 80);

        int year, month, day, hour, min, secs;
        String tsFmtStr = String(FPSTR(SSCANF_TSFORMAT));
        if (sscanf(line, tsFmtStr.c_str(), &year, &month, &day, &hour, &min, &secs) == 6) {
          time_t ts = TimeKeeper::tkMakeTime(year, month, day, hour, min, secs);
          if (ts > msglogSendParams.lastTS) {
            msgFile.seek(pos, SeekSet);
            foundToSend = true;
          }
        } else {
          Serial.print(F("Read line with invalid TS at sendMsgsFromDate with filename: "));
          Serial.println(msgFile.name());
        }
      }
      if(foundToSend) {
        String msgCSVEntryPoint = String(FPSTR(MSGLOG_SENDCSV_URL));
        outPayLoadPtr = payloadPOST(conf, msglogSendParams.maxLines, msgFile, msgCSVEntryPoint, outHttpCode);
        if (outHttpCode != HTTP_CODE_OK) {
          Serial.print(F("WARNING: Not HTTP_CODE_OK returned when calling payloadPOST at sendMsgsFromDate: "));
          Serial.println(outHttpCode);
        } else {
          //FIXME TODO decodificar JSON apontado por payLoadPtr 
        }
        
      } else {
        msgFile.close();
        return CLOUDTASK_SENDMSGSFROMDATE_NOTHINGTOSEND;
      }
      msgFile.close();      
    } else {
      return CLOUDTASK_SENDMSGSFROMDATE_NOFILEWITHDATE;
    }
  } else {
      return CLOUDTASK_SENDMSGSFROMDATE_INVALIDTS;
  }
  return CLOUDTASK_OK;
}

int CloudTask::syncToCloud(CloudConf& conf) {
  if (!TimeKeeper::isValidTS(TimeKeeper::tkNow())) {
    return CLOUDTASK_SYNCTOCLOUD_INVALIDMYUTCTIME;
  }
  if(WiFi.status() != WL_CONNECTED) {
    return CLOUDTASK_SYNCTOCLOUD_NOTCONNECTED;
  }
  SendParams datalogSendParams;
  SendParams msglogSendParams;
  if(!CloudTask::getDatalogSendParams(conf, datalogSendParams)) {
    return CLOUDTASK_SYNCTOCLOUD_UNABLE_GET_DATALOGPARAMS;
  }
  yield();
  time_t newNow = TimeKeeper::tkNow();
  if (!CloudTask::getMsglogSendParams(conf, msglogSendParams)) {
    return CLOUDTASK_SYNCTOCLOUD_UNABLE_GET_MSGLOGPARAMS;
  }
  newNow = TimeKeeper::tkNow() - newNow;
  newNow = msglogSendParams.now + (int)(0.5*newNow);
  if (TimeKeeper::isValidTS(msglogSendParams.now)) {
    if (abs(newNow - TimeKeeper::tkNow()) > 120) {
      TimeKeeper::tkSetTime(newNow);
    }
  }
  yield();
  if (!fsOpen) {
    return CLOUDTASK_SYNCTOCLOUD_FSNOTOPEN;    
  }
  time_t msgDate = 0;
  time_t logDate = 0;
  
  if(!getDatesToOpen(msgDate, logDate, datalogSendParams.lastTS, msglogSendParams.lastTS)) {
    return CLOUDTASK_SYNCTOCLOUD_UNABLE_GET_DATESTOOPEN;
  }
  yield();

  //first sending messages
  if(TimeKeeper::isValidTS(msgDate)) {
    int httpCode;
    std::shared_ptr<String> payLoad;
    int retCode;
    do {
        retCode = CloudTask::sendMsgsFromDate(msgDate, conf, msglogSendParams, payLoad, httpCode);
        yield();
        if (retCode == CLOUDTASK_SENDMSGSFROMDATE_NOTHINGTOSEND) {
          if (TimeKeeper::isSameDate(msgDate, TimeKeeper::tkNow())) {
            this->sentAllMsgLogUntilToday = true;
            break;
          } else {
            bool gotNewMsgDate = CloudTask::getDatesToOpen(msgDate, logDate, 0, msgDate + (3600ul*24ul), false, true);
            if (!gotNewMsgDate || !TimeKeeper::isValidTS(msgDate)) {
              break;
            }
          }
      }
    } while (retCode == CLOUDTASK_SENDMSGSFROMDATE_NOTHINGTOSEND && msgDate <= TimeKeeper::tkNow());
    if (retCode != CLOUDTASK_OK && retCode != CLOUDTASK_SENDMSGSFROMDATE_NOTHINGTOSEND) {
      Serial.print(F("WARNING: unexpected retCode at syncToCloud when sending messages, was :"));
      Serial.println(retCode);
    }
  } else {
      Serial.println(F("WARNING: getDatesToOpen() did not return a valid msgDate at CloudTask::syncToCloud()"));
  }

  //now sending log
  if(TimeKeeper::isValidTS(logDate)) {
    int httpCode;
    std::shared_ptr<String> payLoad;
    int retCode;
    do {
      Serial.println(F("Now calling sendDataLogFromDate"));
      retCode = CloudTask::sendDataLogFromDate(logDate, conf, datalogSendParams, payLoad, httpCode);
      yield();
      if (retCode == CLOUDTASK_SENDDATALOGFROMDATE_NOTHINGTOSEND) {
        if (TimeKeeper::isSameDate(logDate, TimeKeeper::tkNow())) {
          this->sentAllDataLogUntilToday = true;
          break;
        } else {
          Serial.println(F("Nothing found to send, will try new date"));
          bool gotNewLogDate = CloudTask::getDatesToOpen(msgDate, logDate, logDate + (3600ul*24ul), 0 , true, false);
          if (!gotNewLogDate || !TimeKeeper::isValidTS(logDate)) {
            break;
          }
        }
      }
    } while (retCode == CLOUDTASK_SENDDATALOGFROMDATE_NOTHINGTOSEND && logDate <= TimeKeeper::tkNow());
    if (retCode != CLOUDTASK_OK && retCode != CLOUDTASK_SENDDATALOGFROMDATE_NOTHINGTOSEND) {
      Serial.print(F("WARNING: unexpected retCode at syncToCloud when sending datalog, was :"));
      Serial.println(retCode);
    }
  } else {
      Serial.println(F("WARNING: getDatesToOpen() did not return a valid logDate at CloudTask::syncToCloud()"));
  }
  
  
  return CLOUDTASK_OK;
}

std::shared_ptr<String> CloudTask::payloadPOST(CloudConf &conf, int maxLines, Stream& csvStream, const String& entryPoint, int& httpRetCode) {
  WiFiClient client;
  MyHttpClient http;
  httpRetCode = CloudTask::httpDigestAuthAndCSVPOST(maxLines, csvStream, conf, 
                entryPoint.c_str(), client, http);
  std::shared_ptr<String> payload = (httpRetCode != HTTP_CODE_OK) ? std::make_shared<String>("") : std::make_shared<String>(http.getString());
  http.end();
  return payload;
}

int CloudTask::httpDigestAuthAndCSVPOST(int maxLines, Stream& csvStream,
                CloudConf& conf, const char* urlEntry, WiFiClient& client, MyHttpClient& http) {

  if( !conf.isAllValid() ) {
    return CLOUDTASK_INVALID_CLOUDCONF;        
  }
  const short lenParamUrl = strlen(conf.baseUrl) + strlen(urlEntry)+2;
  char paramUrl[lenParamUrl];
  strcpy(paramUrl, conf.baseUrl);
  if(paramUrl[strlen(paramUrl)-1] == '/') 
    paramUrl[strlen(paramUrl)-1] = '\0';
  strcat(paramUrl, urlEntry);
  Serial.print(F("PARAM URL IS: "));
  Serial.println(paramUrl);
  Serial.print(F("[HTTP] begin...\n"));
  if (!http.begin(client, paramUrl))
    return CLOUDTASK_AUTHANDPOST_1STBEGIN_ERR;

  Serial.print(F("[HTTP] 1st POST...\n"));
  // start connection and send HTTP header
  String authHeader = String(FPSTR(AUTH_HEADER));
  const char *keys[] = {authHeader.c_str()};
  http.collectHeaders(keys, 1);
  http.setReuse(true);
  int httpCode = http.POST("");
  if (httpCode <= 0)
    return httpCode;
  Serial.println(F("PASSED 1st http.POST()"));

  String authReq = http.header(authHeader.c_str());
  Serial.println(authReq);
  if (!(authReq.length() > 0)) 
    return CLOUDTASK_AUTHANDPOST_NOAUTHHEADER;
  String authorization = getDigestAuth(authReq, String(conf.login), String(conf.pass), String(paramUrl), 1, "POST");
  http.end();
  
  if (!http.begin(client, paramUrl))
    return CLOUDTASK_AUTHANDPOST_2NDBEGIN_ERR;

  http.addHeader("Authorization", authorization);
  http.addHeader("Content-Type", "text/csv");
  LineLimitedReadStream limitedStream(csvStream, maxLines);
  Serial.print(F("[HTTP] will now try 2nd POST with auth info...\n"));
  http.setReuse(false);
  httpCode = http.sendRequest("POST", (Stream *)&limitedStream, 0);
  client.flush();
  return httpCode;  
}

bool CloudTask::isInternetConnected() {
  bool isConnected = false;
  String userAgent = String(FPSTR(USER_AGENT_STR));
  {
    String url = String(FPSTR(TEST_CONN1_URL));
    isConnected = CloudTask::canGet204(url, userAgent);
  }
  if (!isConnected) {
    String url = String(FPSTR(TEST_CONN2_URL));
    isConnected = CloudTask::canGet204(url, userAgent);
  }
  return isConnected;
}


bool CloudTask::canGet204(String &url, String &userAgent) {
  WiFiClient client;
  HTTPClient http;
  http.setReuse(false);
  Serial.print(F("canGet204 URL IS: "));
  Serial.println(url);
    
  if (!http.begin(client, url)) {
    Serial.print(F("[HTTP] could not reach 204 service. Error on http.begin().\n"));
    return false;
  }

  http.addHeader("User-Agent", userAgent);
  http.addHeader("Connection", "close");

  const int httpCode = http.GET();
  const int httpSize = http.getSize();

  http.end();
  bool isReachable = (httpCode == HTTP_CODE_NO_CONTENT) && (httpSize == 0);
  if (!isReachable) {
    Serial.print(F("[HTTP] could not reach gen204 service. Did not return 204 with Content-Length: 0.\n"));  
  }
  return isReachable;  
}


bool CloudTask::cloudServiceIsReachable() {
  bool isReachable = false;
  CloudConf conf;
  bool readOk = readCloudConf(conf);
  if (!readOk) { 
    Serial.print(F("Could not read cloud conf file at CloudTask::cloudServiceIsReachable"));
    return false; 
  }
  if (!conf.isAllValid()) {
    Serial.print(F("Cloud conf file is not valid at CloudTask::cloudServiceIsReachable"));
    return false;
  }
  if (!conf.enabled) {
    Serial.print(F("Cloud access is disabled at conf file (CloudTask::cloudServiceIsReachable)"));
    return false;
  }
  String gen204EntryPoint = String(FPSTR(GEN204_URL));
  const short lenParamUrl = strlen(conf.baseUrl) + strlen(gen204EntryPoint.c_str())+2;
  char paramUrl[lenParamUrl];
  strcpy(paramUrl, conf.baseUrl);
  if(paramUrl[strlen(paramUrl)-1] == '/') 
    paramUrl[strlen(paramUrl)-1] = '\0';
  strcat(paramUrl, gen204EntryPoint.c_str());
  {
    String urlString(paramUrl);
    String ua("IIRR");
    isReachable = CloudTask::canGet204(urlString, ua);
  }
      
  if (!isReachable) {
    Serial.print(F("[HTTP] could not reach our gen204 service. Did not return 204 with Content-Length: 0.\n"));  
  }
  return isReachable;
}

int CloudTask::httpDigestAuthAndGET(CloudConf& conf, const char* urlEntry, WiFiClient& client,
                         HTTPClient& http) {
  if( !conf.isAllValid() ) {
    return CLOUDTASK_INVALID_CLOUDCONF;        
  }
  const short lenParamUrl = strlen(conf.baseUrl) + strlen(urlEntry)+2;
  char paramUrl[lenParamUrl];
  strcpy(paramUrl, conf.baseUrl);
  if(paramUrl[strlen(paramUrl)-1] == '/') 
    paramUrl[strlen(paramUrl)-1] = '\0';
  strcat(paramUrl, urlEntry);
  Serial.print(F("PARAM URL IS: "));
  Serial.println(paramUrl);
  Serial.print(F("[HTTP] begin...\n"));
  if (!http.begin(client, paramUrl))
    return CLOUDTASK_AUTHANDGET_1STBEGIN_ERR;

  Serial.print(F("[HTTP] 1st GET...\n"));
  // start connection and send HTTP header
  String authHeader = String(FPSTR(AUTH_HEADER));
  const char *keys[] = {authHeader.c_str()};
  http.setReuse(true);
  http.collectHeaders(keys, 1);
  
  int httpCode = http.GET();
  Serial.println(F("PASSED 1st http.GET()"));
  if (httpCode <= 0)
    return httpCode;

  String authReq = http.header(authHeader.c_str());
  Serial.println(authReq);
  if (!(authReq.length() > 0)) 
    return CLOUDTASK_AUTHANDGET_NOAUTHHEADER;
  String authorization = getDigestAuth(authReq, String(conf.login), String(conf.pass), String(paramUrl), 1);
  http.end();

  if (!http.begin(client, paramUrl))
    return CLOUDTASK_AUTHANDGET_2NDBEGIN_ERR;

  http.addHeader("Authorization", authorization);
  http.setReuse(false);
  httpCode = http.GET();
  return httpCode;  
}

std::shared_ptr<String> CloudTask::payloadGET(CloudConf &conf, const String& entryPoint, int& httpRetCode) {
  WiFiClient client;
  HTTPClient http;
  httpRetCode = CloudTask::httpDigestAuthAndGET(conf, entryPoint.c_str(), client, http);
  std::shared_ptr<String> payload = (httpRetCode != HTTP_CODE_OK) ? std::make_shared<String>("") : std::make_shared<String>(http.getString());
  http.end();
  return payload;
}

bool CloudTask::decodeSendParams(const String& jsonString, SendParams& decodedSendParams) {
  const int capacity = JSON_OBJECT_SIZE(5) + 150;
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& root = jsonBuffer.parseObject(jsonString);
  bool ret = true;
  if (!root.success()) {
    return false;
  }
  decodedSendParams.status = root["status"]; 
  
  const char* last_ts = root["last-ts"];
  if (last_ts != NULL) {
    tmElements_t timeStruct;
    ret = HttpDateParser::parseHTTPDate(String(last_ts), timeStruct);
    if(ret) 
      decodedSendParams.lastTS = TimeKeeper::tkMakeTime(timeStruct);
  }
  decodedSendParams.maxLines = root["maxlines"];
  if (decodedSendParams.maxLines == 0) {
    return false; 
  }
  const char* now = root["now"];
  if (now == NULL) {
    return false;
  } else {
    tmElements_t timeStruct;
    ret = HttpDateParser::parseHTTPDate(String(now), timeStruct);
    if(ret) 
      decodedSendParams.now = TimeKeeper::tkMakeTime(timeStruct);
    
  }
  decodedSendParams.errno = root["errno"];

  return ret;
}

bool CloudTask::getEntryPointSendParams(CloudConf& conf, SendParams& sendParams, const String& entryPoint) {
  int retCode;
  int ret = false;
  std::shared_ptr<String> payload = CloudTask::payloadGET(conf, entryPoint, retCode);
  if (retCode != HTTP_CODE_OK) {
    Serial.print(F("WARNING: Not HTTP_CODE_OK returned when calling payloadGET at getEntryPointSendParams: "));
    Serial.println(retCode);
  } else {
    if(CloudTask::decodeSendParams(*payload, sendParams)) {
      Serial.print(F("Successfuly decoded sendParams, maxlines is "));
      Serial.println(sendParams.maxLines);
      ret = true;
    } else {
      Serial.println(F("WARNING: Could not decode json of sendParams"));
    }
  }
  return ret;  
}

bool CloudTask::getDatalogSendParams(CloudConf& conf, SendParams& sendParams) {
   String datalogEntryPoint = String(FPSTR(DATALOG_SENDPARAMS_URL));
   return CloudTask::getEntryPointSendParams(conf, sendParams, datalogEntryPoint);
}

bool CloudTask::getMsglogSendParams(CloudConf& conf, SendParams& sendParams) {
   String msglogEntryPoint = String(FPSTR(MSGLOG_SENDPARAMS_URL));
   return CloudTask::getEntryPointSendParams(conf, sendParams, msglogEntryPoint);
}


//Exemplo de quando pede send-params para datalog pela primeira vez: 
// {"status":0,"last-ts":null,"maxlines":50,"now":"Sun, 12 May 2019 14:37:24 GMT"}
// pegar strings como "const char*", null vai retornar null

#define SMALL_INTERVAL (30 + (rand()%60))

void CloudTask::loop()  {
  if (CloudTask::confAvailable) {
    if (ServerTask::initializationFinished()) {
      time_t nowTime = TimeKeeper::tkNow();
      const time_t diffTime = nowTime - lastCheck;
      const time_t interval = (!sentAllDataLogUntilToday || !sentAllMsgLogUntilToday) ? SMALL_INTERVAL : CLOUD_CHECK_SECS;
      if (diffTime > interval || firstRun) { //should see if we are connected
          if(WiFi.status() == WL_CONNECTED) {
            Serial.println(F("Will try cloud loop"));
            
            CloudConf conf;
            bool readOk = readCloudConf(conf);
            if (readOk && conf.isAllValid() && conf.enabled != 0) {
              const int retCode = syncToCloud(conf);
              if (retCode != CLOUDTASK_OK) {
                Serial.print(F("WARNING: syncToCloud() returned error status code: "));
                Serial.println(retCode);
              }
            } else {
              Serial.println(F("Could not read CloudConf or conf not enabled, will try again after interval"));
            } 
            firstRun = false;
          }
          lastCheck= nowTime;
          yield();
      } else {
          this->delay(max(500l, (((!sentAllDataLogUntilToday || !sentAllMsgLogUntilToday) ? SMALL_INTERVAL : CLOUD_CHECK_SECS) - diffTime)*1000));
      }
    } else yield();
  } else {
    this->delay(1000);
  }
}

bool CloudTask::updateJsonFromConfParams(CloudConf &conf) {
  if(!conf.isAllValid()) return false;

  String fileName = String(FPSTR(CPARAMS_JSON_FILE));
  File confFile = SPIFFS.open(fileName, "w");
  if (!confFile) return false;
  
  DynamicJsonBuffer jsonBuffer(CloudTask::jsonBufferCapacity);
  JsonObject& doc = jsonBuffer.createObject();
  String nBaseUrl = String(F("baseUrl"));
  String ncertHash = String(F("certHash"));
  String nLogin = String(F("login"));
  String nPass = String(F("pass"));
  String nEnabled = String(F("enabled"));
  
  
  doc[nBaseUrl.c_str()] = conf.baseUrl;
  doc[ncertHash.c_str()] = conf.certHash;
  doc[nLogin.c_str()] = conf.login;
  doc[nPass.c_str()] = conf.pass;
  doc[nEnabled.c_str()] = (conf.enabled == 0) ? 0 : 1;
  doc.printTo(confFile);
  confFile.flush();
  confFile.close();
  CloudTask::confAvailable = true;
  return true;
}

bool CloudTask::updateConfParamsFromJson(CloudConf &conf, JsonObject &root) {
  strncpy(conf.baseUrl, String(root.get<const char*>("baseUrl")).c_str(), 128);
  strncpy(conf.certHash, String(root.get<const char*>("certHash")).c_str(), 40);
  strncpy(conf.login, String(root.get<const char*>("login")).c_str(), 64);
  strncpy(conf.pass, String(root.get<const char*>("pass")).c_str(), 64);
  conf.enabled = root["enabled"];
  if (!conf.isAllValid()) return false;
  return true;
}

JsonObject* CloudTask::jsonForCloudConf(DynamicJsonBuffer& bufferToUse) {
    JsonObject* result = NULL;
    if (fsOpen) {
      String fileName = String(FPSTR(CPARAMS_JSON_FILE));
      File confFile = SPIFFS.open(fileName, "r");
      if (!confFile) return NULL;
      result = &bufferToUse.parseObject(confFile);
      confFile.close();
    }
    return result;
}

bool CloudTask::readCloudConf(CloudConf &conf) {
  const size_t bufferSize = JSON_OBJECT_SIZE(5) + 220;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject* root = jsonForCloudConf(jsonBuffer);
  if (root == NULL) return false;
  bool result = updateConfParamsFromJson(conf, *root);
  return result;
}
