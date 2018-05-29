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

#include <ArduinoJson.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <pgmspace.h>
#include "ServerTask.h"
#include "global_funcs.h"
#include <NTPClient.h>
#include "TimeKeeper.h"
#include "SensorTask.h"
#include "DirStream.h"
#include <memory>
#include "FS.h"
#include <algorithm>

ESP8266WebServer server(80);

static const char WWW_DEFAULT_USERNAME[] PROGMEM = "admin";
static const char WWW_REALM[] PROGMEM = "Intelligent Irrigator: Login Required";
static const char WWW_AUTH_FAIL[] PROGMEM = "Authentication Failed";
static const char WWW_MIME_TEXTHTML[] PROGMEM = "text/html";
static const char WWW_MIME_JSON[] PROGMEM = "application/json";
static const char WWW_ROOT_CONNECTION_MSG[] PROGMEM = "<h1>You are connected to the Intelligent Irrigator v0.01. Please use the application to control it instead.</h1>";

static const char WIFI_DEFAULT_SSID_FMTSTR[] PROGMEM = "IIRR-%02X%02X";
static const char WIFI_DEFAULT_PASS_FMTSTR[] PROGMEM = "%02X%02X%02X%02X%02X%02X";

static const char JSON_STATUS[] PROGMEM = "status";
static const char JSON_F_WIFISCANNETS[] PROGMEM = "/v100/wifiScanNets";
static const char JSON_F_WIFISCANCOMPLETE[] PROGMEM = "/v100/wifiScanComplete";
static const char JSON_F_GETWIFINETWITHID[] PROGMEM = "/v100/getWifiNetWithId";
static const char JSON_F_WIFICONNECT[] PROGMEM = "/v100/wifiConnect";
static const char JSON_F_WIFICONNECTSTATUS[] PROGMEM = "/v100/wifiConnectStatus";
static const char JSON_F_LEARNWATERFLOW[] PROGMEM = "/v100/learnWaterFlow";
static const char JSON_F_LEARNWATERFSTATUS[] PROGMEM = "/v100/learnWaterFStatus"; 
static const char JSON_F_RESETWATERFSTATUS[] PROGMEM = "/v100/resetWaterFStatus";

static const char JSON_F_MAINCONFPARAMS[] PROGMEM = "/v100/getMainConfParams";
static const char JSON_F_UPDATEMAINCONFPARAMS[] PROGMEM = "/v100/updateMainConfParams";
static const char JSON_F_GETLOGDIRCONTENTS[] PROGMEM = "/v100/getLogDirContents";
static const char JSON_F_GETCSVFILE[] PROGMEM = "/v100/getCSVFile";
static const char JSON_F_GETSOILMOISTURE[] PROGMEM = "/v100/getSoilMoisture";
static const char JSON_F_GETIRRIGDATA[] PROGMEM = "/v100/getIrrigData";
static const char JSON_F_GETMYUTCTIME[] PROGMEM = "/v100/getMyUTCTime";
static const char JSON_F_UPDATEUTCTIME[] PROGMEM = "/v100/updateMyUTCTime";
static const char HTTP_MIME_CSV[] PROGMEM = "text/csv";

void streamCSV(Stream &csvStream, size_t contentLength = CONTENT_LENGTH_UNKNOWN) {
  String csvMIME = String(FPSTR(HTTP_MIME_CSV));
  server.setContentLength(contentLength);
  server.send(HTTP_OK, csvMIME, "");
  const size_t bufSize = 64;
  char buf[bufSize];
  size_t bytesSent = 0;
  size_t contentLimit;
  while (csvStream.available()) {
    if (contentLength != CONTENT_LENGTH_UNKNOWN) {
      contentLimit = std::min(bufSize - 1, contentLength - bytesSent);
    } else {
      contentLimit = bufSize - 1;
    }
    size_t bytesRead = csvStream.readBytes(buf, contentLimit);
    if (bytesRead > 0) {
      buf[bytesRead] = '\0';
      server.sendContent(String(buf));
      bytesSent += bytesRead;
    }
    if (contentLength != CONTENT_LENGTH_UNKNOWN && bytesSent >= contentLength)
      break;
  }
}

int ServerTask::getAdminPassword(char outPassword[], int maxLen) {
  if (maxLen < 13) {
    return SERVERTASK_ERROR_SMALLARRAY;
  }
  uint8_t mac[6];
  
  WiFi.macAddress(mac);
  outPassword[12] = '\0';
  snprintf_P(outPassword, 13, WIFI_DEFAULT_PASS_FMTSTR, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  

  return SERVERTASK_OK;
}

void ServerTask::authenticateAndExecute(ServerTask *taskServer, std::function<void (ServerTask *srvP)> funcToCall) {
  char www_password[13];
  taskServer->getAdminPassword(www_password, 13);
  String admUserName = String(FPSTR(WWW_DEFAULT_USERNAME));
  if(!server.authenticate(admUserName.c_str(), www_password)) {
    taskServer->delay(3000);
    String realmName = String(FPSTR(WWW_REALM));
    String authFailMsg = String(FPSTR(WWW_AUTH_FAIL));
    return server.requestAuthentication(DIGEST_AUTH, realmName.c_str(), authFailMsg.c_str());  
  } else {
    return funcToCall(taskServer);
  }
}


void ServerTask::handleRoot(ServerTask *taskServer) {
    String htmlMime = String(FPSTR(WWW_MIME_TEXTHTML));
    String rootMsg = String(FPSTR(WWW_ROOT_CONNECTION_MSG));
	  return server.send(HTTP_OK, htmlMime.c_str(), rootMsg.c_str());
}

void ServerTask::handleGetMyUTCTime(ServerTask *taskServer) {
  const size_t bufferSize = JSON_OBJECT_SIZE(6);
  DynamicJsonBuffer jsonBuffer(bufferSize);
  
  JsonObject& root = jsonBuffer.createObject();
  time_t nowTime = TimeKeeper::tkNow();
  tmElements_t timeStruct;
  TimeKeeper::tkBreakTime(nowTime, timeStruct);
  root["year"] = timeStruct.Year + 1970;
  root["month"] = timeStruct.Month;
  root["day"] = timeStruct.Day;
  root["hour"] = timeStruct.Hour;
  root["min"] = timeStruct.Minute;
  root["sec"] = timeStruct.Second;
  String jsonStr;
  root.printTo(jsonStr);
  String jsonMime = String(FPSTR(WWW_MIME_JSON));
  return server.send(HTTP_OK, jsonMime.c_str(), jsonStr);
}

void ServerTask::handleGetIrrigData(ServerTask *taskServer) {
  const size_t bufferSize = JSON_OBJECT_SIZE(7);
  DynamicJsonBuffer jsonBuffer(bufferSize);
  
  JsonObject& root = jsonBuffer.createObject();
  root["surfacestirr"] = irrigData.surfaceAtStartIrrig;
  root["middlestirr"] = irrigData.middleAtStartIrrig;
  root["deepstirr"] = irrigData.deepAtStartIrrig;
  root["isirrig"] = irrigData.isIrrigating ? 1 : 0;
  root["irrigsince"] = irrigData.irrigSince;
  root["lstirrigend"] = irrigData.lastIrrigEnd;
  root["irrigtdaysecs"] = irrigData.irrigTodaySecs;  
  String jsonStr;
  root.printTo(jsonStr);
  String jsonMime = String(FPSTR(WWW_MIME_JSON));
  return server.send(HTTP_OK, jsonMime.c_str(), jsonStr);
}

void ServerTask::handleGetSoilMoisture(ServerTask *taskServer) {
  const size_t bufferSize = JSON_OBJECT_SIZE(4);
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.createObject();
  root["surface"] = moistures.surface;
  root["middle"] = moistures.middle;
  root["deep"] = moistures.deep;
  root["ts"] = moistures.timeStamp;
  String jsonStr;
  root.printTo(jsonStr);
  String jsonMime = String(FPSTR(WWW_MIME_JSON));
  return server.send(HTTP_OK, jsonMime.c_str(), jsonStr);
}

void ServerTask::handleGetMainConfParams(ServerTask *taskServer) {
  const size_t bufferSize = JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(7);
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = SensorTask::createJsonFromConfParams(jsonBuffer);
  String jsonStr;
  root.printTo(jsonStr);
  String jsonMime = String(FPSTR(WWW_MIME_JSON));
  return server.send(HTTP_OK, jsonMime.c_str(), jsonStr);
}

void ServerTask::handleGetLogDirContents(ServerTask *taskServer) {
  if (!fsOpen) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_GETLOGDIRCONTENTS_FSNOTOPEN, HTTP_INTERNAL_ERROR);
  }
  String logDir = String(FPSTR(LOG_DIR));
  std::shared_ptr<Dir> logDirPtr(new Dir(SPIFFS.openDir(logDir)));
  DirStream dStream(logDirPtr);
  streamCSV(dStream);
}

void ServerTask::handleGetCSVFile(ServerTask *taskServer) {
  if (!fsOpen) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_GETCSVFILE_FSNOTOPEN, HTTP_INTERNAL_ERROR);
  }
  static const char FILEPARAM_STR[] PROGMEM = "f";
  String fileParamStr = String(FPSTR(FILEPARAM_STR));
  if(!server.hasArg(fileParamStr)) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_GETCSVFILE_NOPARAM_FILE, HTTP_BAD_REQUEST);
  }
  const String fileName = server.arg(fileParamStr);
  File file = SPIFFS.open(fileName, "r");
  if (!file) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_GETCSVFILE_FILENOTFOUND, HTTP_NOT_FOUND);
  }
  return streamCSV(file, file.size());
}

void ServerTask::handleWifiConnectStatus(ServerTask *taskServer) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  String jsonStatusName = String(FPSTR(JSON_STATUS));
  root[jsonStatusName.c_str()] = (int)WiFi.status();
  String jsonStr;
  root.printTo(jsonStr);

  String jsonMime = String(FPSTR(WWW_MIME_JSON));
  return server.send(HTTP_OK, jsonMime.c_str(), jsonStr);
}

void ServerTask::handleLearnWaterFStatus(ServerTask *taskServer) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  String jsonStatusName = String(FPSTR(JSON_STATUS));
  root[jsonStatusName.c_str()] = (int)SensorTask::getLastLearnFlowStatus();
  String jsonStr;
  root.printTo(jsonStr);

  String jsonMime = String(FPSTR(WWW_MIME_JSON));
  return server.send(HTTP_OK, jsonMime.c_str(), jsonStr);
}


void ServerTask::handleWifiScanComplete(ServerTask *taskServer) {
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  String jsonStatus = String(FPSTR(JSON_STATUS));
  root[jsonStatus.c_str()] = (int)WiFi.scanComplete();
  String getUrlName = String(F("getUrl"));
  String getUrl = String(FPSTR(JSON_F_GETWIFINETWITHID));
  root[getUrlName.c_str()] = getUrl.c_str();
  String jsonStr;
  root.printTo(jsonStr);

  String jsonMime = String(FPSTR(WWW_MIME_JSON));
  return server.send(HTTP_OK, jsonMime.c_str(), jsonStr);
}

void ServerTask::sendJsonWithStatusOnly(ServerTaskStatusCodes taskStatusCode, HTTPStatus httpStatus) {
    StaticJsonBuffer<200> jsonBuffer;
    JsonObject& root = jsonBuffer.createObject();
    String statusName = String(FPSTR(JSON_STATUS));
    root[statusName.c_str()] = (int)taskStatusCode;
    String jsonStr;
    root.printTo(jsonStr);
    Serial.println(jsonStr);
    String jsonMime = String(FPSTR(WWW_MIME_JSON));
    return server.send(httpStatus, jsonMime.c_str(), jsonStr);  
}

void ServerTask::updateUTCTime(ServerTask *taskServer) {
  const size_t bufferSize = JSON_OBJECT_SIZE(6) + 60;
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
  int aYear = root["year"]; 
  int aMonth = root["month"];
  int aDay = root["day"]; 
  int aHour = root["hour"];
  int aMin = root["min"];
  int aSec = root["sec"];
  if ((aYear > 2016) && 
      (aMonth > 0) && (aMonth < 13) && 
      (aDay > 0) && (aDay < 32) && 
      (aHour >= 0) && (aHour < 24) && 
      (aMin >= 0) && (aMin < 60) &&
      (aSec >= 0) && (aSec < 60)) {

    TimeKeeper::tkSetTime(aYear, aMonth, aDay, aHour, aMin, aSec);
    return sendJsonWithStatusOnly(SERVERTASK_OK, HTTP_OK);      
  } else {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_UPDATEUTCTIME_INVALIDPARAMS, HTTP_BAD_REQUEST);
  }
}

void ServerTask::handleUpdateMainConfParams(ServerTask *taskServer) {
  const size_t bufferSize = JSON_ARRAY_SIZE(8) + JSON_OBJECT_SIZE(7);
  DynamicJsonBuffer jsonBuffer(bufferSize);
  JsonObject& root = jsonBuffer.parseObject(server.arg("plain"));
  ConfParams newParams;
  SensorTask::updateConfParamsFromJson(newParams, root);
  if (!newParams.isAllValid()) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_UPDATEMAINCONFPARAMS_INVALIDPARAMS, HTTP_BAD_REQUEST);
  }
  if (!SensorTask::updateConfParams(newParams)) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_UPDATEMAINCONFPARAMS_ERRORWRITEJSON, HTTP_INTERNAL_ERROR);
  }
  return sendJsonWithStatusOnly(SERVERTASK_OK, HTTP_OK);
  
}

void ServerTask::handleWifiConnect(ServerTask *taskServer) {
  static const char SSID_STR[] PROGMEM = "ssid";
  static const char PASS_STR[] PROGMEM = "pass";
  if(!server.hasArg(String(FPSTR(SSID_STR)))) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_WIFICONNECT_NOPARAM_SSID, HTTP_BAD_REQUEST);
  }
  const String ssidStr = server.arg(String(FPSTR(SSID_STR)));
  if(ssidStr.length() < 1 || ssidStr.length() > 32) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_WIFICONNECT_INVALIDPARAM_SSID, HTTP_BAD_REQUEST);
  }
  const bool hasPass = server.hasArg(String(FPSTR(PASS_STR)));
  if (hasPass) {
    const String pass = server.arg(String(FPSTR(PASS_STR)));
    if(pass.length() < 8 || pass.length() > 64) {
      return sendJsonWithStatusOnly(SERVERTASK_HANDLE_WIFICONNECT_INVALIDPARAM_PASS, HTTP_BAD_REQUEST);
    }
    WiFi.begin(ssidStr.c_str(), pass.c_str());
  } else {
    WiFi.begin(ssidStr.c_str());
  }
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  String jsonStatus = String(FPSTR(JSON_STATUS));
  root[jsonStatus.c_str()] = (int)SERVERTASK_OK;
  String monitUrlName = String(F("monitUrl"));
  String monitUrl = String(FPSTR(JSON_F_WIFICONNECTSTATUS));
  root[monitUrlName.c_str()] = monitUrl.c_str();
  String jsonStr;
  root.printTo(jsonStr);
  String jsonMime = String(FPSTR(WWW_MIME_JSON));
  return server.send(HTTP_ACCEPTED, jsonMime.c_str(), jsonStr);
}

void ServerTask::handleGetWifiNetWithId(ServerTask *taskServer) {
  static const char ID_STR[] PROGMEM = "id";
  if (!server.hasArg(String(FPSTR(ID_STR)))) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_GETWIFINETWITHID_NOPARAM_ID, HTTP_BAD_REQUEST);
  }

  const String idStr = server.arg(String(FPSTR(ID_STR)));
  if (!isNumber(idStr) || idStr.length() > 3) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_GETWIFINETWITHID_NOTANUMBER_ID, HTTP_BAD_REQUEST); 
  }

  int idInt = (int) idStr.toInt();
  int n = WiFi.scanComplete();
  if (n < 0) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_GETWIFINETWITHID_STILLSCAN, HTTP_REQUEST_TIMEOUT);
  }
  if (n == 0) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_GETWIFINETWITHID_NONETSFOUND, HTTP_NOT_FOUND);
  }
  if (idInt > (n -1) || idInt < 0) {
    return sendJsonWithStatusOnly(SERVERTASK_HANDLE_GETWIFINETWITHID_IDOUTOFBOUNDS, HTTP_NOT_FOUND);
  }
  
  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  String statusName = String(FPSTR(JSON_STATUS));
  String ssidName = String(F("SSID"));
  String chName = String(F("Ch"));
  String powName = String(F("Pow"));
  String cryptName = String(F("Crypt"));
  Serial.println(statusName);
  Serial.println(ssidName);
  Serial.println(chName);
  Serial.println(powName);
  Serial.println(cryptName);
  root[statusName.c_str()] = (int)SERVERTASK_OK;
  String ssidRet = String(WiFi.SSID(idInt));
  root[ssidName.c_str()] = ssidRet.c_str();
  root[chName.c_str()] = WiFi.channel(idInt);
  root[powName.c_str()] = WiFi.RSSI(idInt);
  root[cryptName.c_str()] = (int)WiFi.encryptionType(idInt);
  
  
  String jsonStr;
  root.printTo(jsonStr);
  Serial.println(jsonStr);

  String jsonMime = String(FPSTR(WWW_MIME_JSON));
  return server.send(HTTP_OK, jsonMime.c_str(), jsonStr);    
}

void ServerTask::handleWifiScanNets(ServerTask *taskServer) {
  taskServer->wifiScanNets();

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  String statusName = String(FPSTR(JSON_STATUS));
  root[statusName.c_str()] = 0;
  String monitName = String(F("monitUrl"));
  String monitUrl = String(FPSTR(JSON_F_WIFISCANCOMPLETE));
  root[monitName.c_str()] = monitUrl.c_str();
  String jsonStr;
  root.printTo(jsonStr);
  String jsonMime = String(FPSTR(WWW_MIME_JSON));
  return server.send(HTTP_ACCEPTED, jsonMime.c_str(), jsonStr);
}

void ServerTask::handleResetWaterFStatus(ServerTask *taskServer) {
  SensorTask::resetLearnFlowStatus();

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  String statusName = String(FPSTR(JSON_STATUS));
  root[statusName.c_str()] = 0;
  String monitName = String(F("monitUrl"));
  String monitUrl = String(FPSTR(JSON_F_LEARNWATERFSTATUS));
  root[monitName.c_str()] = monitUrl.c_str();
  String jsonStr;
  root.printTo(jsonStr);
  String jsonMime = String(FPSTR(WWW_MIME_JSON));
  return server.send(HTTP_ACCEPTED, jsonMime.c_str(), jsonStr);
}


void ServerTask::handleLearnWaterFlow(ServerTask *taskServer) {
  SensorTask::asyncLearnNormalFlow();

  StaticJsonBuffer<200> jsonBuffer;
  JsonObject& root = jsonBuffer.createObject();
  String statusName = String(FPSTR(JSON_STATUS));
  root[statusName.c_str()] = 0;
  String monitName = String(F("monitUrl"));
  String monitUrl = String(FPSTR(JSON_F_LEARNWATERFSTATUS));
  root[monitName.c_str()] = monitUrl.c_str();
  String jsonStr;
  root.printTo(jsonStr);
  String jsonMime = String(FPSTR(WWW_MIME_JSON));
  return server.send(HTTP_ACCEPTED, jsonMime.c_str(), jsonStr);
}


ServerTask::ServerTask() : Task() {
  uint8_t mac[6];
  char ssId[10] = { 0 };
  
  WiFi.macAddress(mac);
  sprintf_P(ssId, WIFI_DEFAULT_SSID_FMTSTR, mac[4], mac[5]);
  char pass[13];
  getAdminPassword(pass, 13);
  Serial.print(F("SSID: "));
  Serial.println(ssId);
  Serial.print(F("Default Pass: "));
  Serial.println(pass);

  WiFi.softAP(ssId, pass);
  IPAddress myIP = WiFi.softAPIP();
  Serial.print(F("Own AP IP address: "));
  Serial.println(myIP);
  
  WiFi.enableAP(true);
  /*
  if (WiFi.wifi_get_opmode() != WIFI_AP_STA) {
    WiFi.mode(WIFI_AP);
  }
  int i;
  for (i = 0; i < 10; ++i) {
    if (WiFi.status() != WL_CONNECTED) {
      this->delay(500);
    } else {
      break;
    }
  }
  if (i == 10) {
    Serial.println(F("INFO: Wifi not connected to 3rd party AP"));
    WiFi.mode(WIFI_AP);
  } else {
    WiFi.mode(WIFI_AP_STA);
    Serial.print(F("INFO: Wifi connected to 3rd party AP, my IP addr "));
    Serial.println(WiFi.localIP());
    
  }
  */

  
  static ESP8266WebServer::THandlerFunction myHandleRoot = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleRoot);
  server.on("/", myHandleRoot);

  static ESP8266WebServer::THandlerFunction myHandleWifiScan = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleWifiScanNets);
  server.on(String(FPSTR(JSON_F_WIFISCANNETS)), HTTP_POST, myHandleWifiScan);

  static ESP8266WebServer::THandlerFunction myHandleWifiScanComplete = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleWifiScanComplete);
  server.on(String(FPSTR(JSON_F_WIFISCANCOMPLETE)), HTTP_GET, myHandleWifiScanComplete);

  static ESP8266WebServer::THandlerFunction myHandleGetWifiNetWithId = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleGetWifiNetWithId);
  server.on(String(FPSTR(JSON_F_GETWIFINETWITHID)), HTTP_GET, myHandleGetWifiNetWithId);

  static ESP8266WebServer::THandlerFunction myHandleWifiConnect = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleWifiConnect);
  server.on(String(FPSTR(JSON_F_WIFICONNECT)), HTTP_POST, myHandleWifiConnect);
  
  static ESP8266WebServer::THandlerFunction myHandleWifiConnectStatus = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleWifiConnectStatus);
  server.on(String(FPSTR(JSON_F_WIFICONNECTSTATUS)), HTTP_GET, myHandleWifiConnectStatus);

  static ESP8266WebServer::THandlerFunction myHandleGetMainConfParams = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleGetMainConfParams);
  server.on(String(FPSTR(JSON_F_MAINCONFPARAMS)), HTTP_GET, myHandleGetMainConfParams);

  static ESP8266WebServer::THandlerFunction myHandleUpdateMainConfParams = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleUpdateMainConfParams);
  server.on(String(FPSTR(JSON_F_UPDATEMAINCONFPARAMS)), HTTP_POST, myHandleUpdateMainConfParams);

  static ESP8266WebServer::THandlerFunction myHandleGetLogDirContents = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleGetLogDirContents);
  server.on(String(FPSTR(JSON_F_GETLOGDIRCONTENTS)), HTTP_GET, myHandleGetLogDirContents);

  static ESP8266WebServer::THandlerFunction myHandleGetCSVFile = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleGetCSVFile);
  server.on(String(FPSTR(JSON_F_GETCSVFILE)), HTTP_GET, myHandleGetCSVFile);  

  static ESP8266WebServer::THandlerFunction myHandleLearnWaterFlow = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleLearnWaterFlow);
  server.on(String(FPSTR(JSON_F_LEARNWATERFLOW)), HTTP_POST, myHandleLearnWaterFlow);

  static ESP8266WebServer::THandlerFunction myHandleLearnWaterFStatus = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleLearnWaterFStatus);
  server.on(String(FPSTR(JSON_F_LEARNWATERFSTATUS)), HTTP_GET, myHandleLearnWaterFStatus);

  static ESP8266WebServer::THandlerFunction myHandleResetWaterFStatus = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleResetWaterFStatus);
  server.on(String(FPSTR(JSON_F_RESETWATERFSTATUS)), HTTP_POST, myHandleResetWaterFStatus);
  

  static ESP8266WebServer::THandlerFunction myHandleGetSoilMoisture = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleGetSoilMoisture);
  server.on(String(FPSTR(JSON_F_GETSOILMOISTURE)), HTTP_GET, myHandleGetSoilMoisture);

  static ESP8266WebServer::THandlerFunction myHandleGetIrrigData = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleGetIrrigData);
  server.on(String(FPSTR(JSON_F_GETIRRIGDATA)), HTTP_GET, myHandleGetIrrigData);


  static ESP8266WebServer::THandlerFunction myHandleGetMyUTCTime = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::handleGetMyUTCTime);
  server.on(String(FPSTR(JSON_F_GETMYUTCTIME)), HTTP_GET, myHandleGetMyUTCTime);

  static ESP8266WebServer::THandlerFunction myHandleUpdateUTCTime = std::bind(ServerTask::authenticateAndExecute, this, ServerTask::updateUTCTime);
  server.on(String(FPSTR(JSON_F_UPDATEUTCTIME)), HTTP_POST, myHandleUpdateUTCTime);


  server.begin();
  Serial.println(F("HTTP server started"));
}

void ServerTask::loopServerMode() {
  server.handleClient();
}

void ServerTask::loop()  {
  loopServerMode();
  yield();
}

void ServerTask::wifiScanNets() {
  WiFi.mode(WIFI_AP_STA);
  WiFi.disconnect();
  this->delay(100);
  WiFi.scanNetworks(true);
}


