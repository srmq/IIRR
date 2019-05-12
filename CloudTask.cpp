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
#include <pgmspace.h>
#include <Arduino.h>
#include <cstring>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>


static const char CPARAMS_JSON_FILE[] PROGMEM = "/conf/cparams.json";

static const char DATALOG_SENDPARAMS_URL[] PROGMEM = "/v100/datalog/send-params";
static const char MSGLOG_SENDPARAMS_URL[] PROGMEM = "/v100/msglog/send-params";
static const char AUTH_HEADER[] PROGMEM = "WWW-Authenticate";

bool CloudTask::confAvailable = false;

CloudConf::CloudConf() {
  memset(this->baseUrl, 0, sizeof(this->baseUrl));
  memset(this->certHash, 0, sizeof(this->certHash));
  memset(this->login, 0, sizeof(this->login));
  memset(this->pass, 0, sizeof(this->pass));
  this->enabled = 0;
}

CloudTask::CloudTask() : Task(), firstRun(true), lastCheck(TimeKeeper::tkNow()) {
  String confFileName = String(FPSTR(CPARAMS_JSON_FILE));
  if (fsOpen) {
    if(SPIFFS.exists(confFileName)) {
          CloudTask::confAvailable = true;
    }
  }
  this->lastTSDataSent = TimeKeeper::firstValidTime();
  this->lastTSMsgSent = this->lastTSDataSent;
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

static String getDigestAuth(String& authReq, const String& username, const String& password, const String& uri, unsigned int counter) {
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
  md5.add(String("GET:") + uri);
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

int CloudTask::httpDigestAuthAndCSVPOST(time_t onlyAfter, int maxLines, Stream& csvStream, DynamicJsonBuffer& returnObject,
                CloudConf& conf, const char* urlEntry, WiFiClient& client, HTTPClient& http) {

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
  
  int httpCode = http.POST("H");
    Serial.println(F("PASSED 1st http.POST()"));
  if (httpCode <= 0)
    return httpCode;

  String authReq = http.header(authHeader.c_str());
  Serial.println(authReq);
  if (!(authReq.length() > 0)) 
    return CLOUDTASK_AUTHANDPOST_NOAUTHHEADER;
  String authorization = getDigestAuth(authReq, String(conf.login), String(conf.pass), String(paramUrl), 1);
  http.end();

  if (!http.begin(client, paramUrl))
    return CLOUDTASK_AUTHANDPOST_2NDBEGIN_ERR;

  http.addHeader("Authorization", authorization);
  http.addHeader("Content-Type", "text/csv");
  LineLimitedReadStream limitedStream(csvStream, maxLines);
  httpCode = http.sendRequest("POST", (Stream *)&limitedStream, 0);
  return httpCode;  
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
void CloudTask::loop()  {
  if (CloudTask::confAvailable) {
    if (ServerTask::initializationFinished()) {
      time_t nowTime = TimeKeeper::tkNow();
      const time_t diffTime = nowTime - lastCheck;
      if (diffTime > CLOUD_CHECK_SECS || firstRun) { //should see if we are connected
          if(WiFi.status() == WL_CONNECTED) {
            Serial.println(F("Will try cloud loop"));
            
            CloudConf conf;
            bool readOk = readCloudConf(conf);
            if (readOk && conf.isAllValid() && conf.enabled != 0) {
              String datalogEntryPoint = String(FPSTR(DATALOG_SENDPARAMS_URL));

              int retCode;
              std::shared_ptr<String> payload = CloudTask::payloadGET(conf, datalogEntryPoint, retCode);
              if (retCode != HTTP_CODE_OK) {
                Serial.print(F("WARNING: Not HTTP_CODE_OK returned when calling httpDigestAuthAndGET: "));
                Serial.println(retCode);
              } else {
                SendParams sendParams;
                if(CloudTask::decodeSendParams(*payload, sendParams)) {
                  Serial.print(F("Successfuly decoded sendParams, maxlines is "));
                  Serial.println(sendParams.maxLines);
                } else {
                  Serial.println(F("WARNING: Could not decode json of sendParams"));
                }
              }
              Serial.println(*payload);
            } else {
              Serial.println(F("Could not read CloudConf or conf not enabled, will try again after interval"));
            } 
            firstRun = false;
          }
          lastCheck= nowTime;
          yield();
      } else {
          this->delay((CLOUD_CHECK_SECS - diffTime)*1000);
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
