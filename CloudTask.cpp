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
#include "FS.h"


static const char CPARAMS_JSON_FILE[] PROGMEM = "/conf/cparams.json";

bool CloudTask::confAvailable = false;

CloudTask::CloudTask() : Task(), firstRun(true), lastCheck(TimeKeeper::tkNow()) {
  String confFileName = String(FPSTR(CPARAMS_JSON_FILE));
  if (fsOpen) {
    if(SPIFFS.exists(confFileName)) {
          CloudTask::confAvailable = true;
    }
  }
}

void CloudTask::loop()  {
  if (CloudTask::confAvailable) {
    if (ServerTask::initializationFinished()) {
      time_t nowTime = TimeKeeper::tkNow();
      const time_t diffTime = nowTime - lastCheck;
      if (diffTime > CLOUD_CHECK_SECS || firstRun) { //should see if we are connected
          if(WiFi.status() == WL_CONNECTED) {
            CloudConf conf;
            bool readOk = readCloudConf(conf); 
            if (readOk) {
              //FIXME do magic
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
  
  doc["baseUrl"] = conf.baseUrl;
  doc["certHash"] = conf.certHash;
  doc["login"] = conf.login;
  doc["pass"] = conf.pass;
  doc["enabled"] = (conf.enabled == 0) ? 0 : 1;
  doc.printTo(confFile);
  confFile.flush();
  confFile.close();
  CloudTask::confAvailable = true;
  return true;
}

bool CloudTask::updateConfParamsFromJson(CloudConf &conf, JsonObject &root) {
  strncpy(conf.baseUrl, root["baseUrl"], 128);
  strncpy(conf.certHash, root["certHash"], 40);
  strncpy(conf.login, root["login"], 64);
  strncpy(conf.pass, root["pass"], 64);
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
