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
#include "WiFiTask.h"
#include "FS.h"
#include <cstring>

static const char WIFINETS_JSON_FILE[] PROGMEM = "/conf/wifinets.json";

WiFiTask::WiFiTask() : Task(), lastCheck(TimeKeeper::tkNow()), firstRun(true) {
  
}

void WiFiTask::loop()  {
  if (ServerTask::initializationFinished()) {
      time_t nowTime = TimeKeeper::tkNow();
      const time_t diffTime = nowTime - lastCheck;
      if (diffTime > WIFI_CHECK_SECONDS || firstRun) { //should see if we are connected
          firstRun = false;
          if(WiFi.status() != WL_CONNECTED) {
              yield();

              String fileName = String(FPSTR(WIFINETS_JSON_FILE));
              loopWiFis(fileName);
          }
          lastCheck= nowTime;
      } else {
          this->delay((WIFI_CHECK_SECONDS - diffTime)*1000);
      }
  } else yield();
}

int WiFiTask::isAvailable(const char* ssid) {
  String ssidStr = String(ssid);
  int nNets = WiFi.scanComplete();
  for (int i = 0; i < nNets; i++) {
    String ssidRet = String(WiFi.SSID(i));
    if (ssidStr.compareTo(ssidRet) == 0) {
        return i;
    }
  }
  return -1;
}

bool WiFiTask::addToTop(const String& newssid, const String& newpass) {
  String fileName = String(FPSTR(WIFINETS_JSON_FILE));
  File confFile = SPIFFS.open(fileName, "r");
  char ssids[10][33];
  memset(ssids, 0, 10*33*sizeof(char));
  char pass[10][65];
  memset(pass, 0, 10*65*sizeof(char));
  int ssidFoundAt = -1;
  int arraySize = 0;
  const size_t capacity = JSON_ARRAY_SIZE(MAX_WIFI_ELEMS) + JSON_OBJECT_SIZE(1) + MAX_WIFI_ELEMS*JSON_OBJECT_SIZE(2) + 121*MAX_WIFI_ELEMS;
  if (confFile) {
    DynamicJsonBuffer jsonBuffer(capacity);
    JsonObject& root = jsonBuffer.parseObject(confFile);
    confFile.close();
    JsonArray& wifinets = root["wifinets"];
    arraySize = wifinets.size();
    for (int i = 0; i < arraySize && i < 10; i++) {
      strncpy(ssids[i], wifinets[i]["ssid"], 32);
      strncpy(pass[i], wifinets[i]["pass"], 64);
      if (strncmp(ssids[i], newssid.c_str(), 32) == 0 && ssidFoundAt == -1) {
        ssidFoundAt = i;
      }
    }
  }
  DynamicJsonBuffer jsonBuffer(capacity);
  JsonObject& root = jsonBuffer.createObject();
  JsonArray& wifinets = root.createNestedArray("wifinets");
  JsonObject& firstEntry = jsonBuffer.createObject();
  firstEntry["ssid"] = newssid;
  firstEntry["pass"] = newpass;
  wifinets.add(firstEntry);
  int copied = 0;
  for (int i = 0; i < arraySize && copied < 10 && i < 10; i++) {
    if (i == ssidFoundAt) continue;
    JsonObject& entry = jsonBuffer.createObject();
    entry["ssid"] = ssids[i];
    entry["pass"] = pass[i];
    wifinets.add(entry);
    copied++;
  }
  confFile = SPIFFS.open(fileName, "w");
  if (!confFile) return false;
  root.printTo(confFile);
  confFile.flush();
  confFile.close();
  return true; 
}

void WiFiTask::loopWiFis(String& fileName) {
    File confFile = SPIFFS.open(fileName, "r");
    if (!confFile) return;
    const size_t capacity = JSON_ARRAY_SIZE(MAX_WIFI_ELEMS) + JSON_OBJECT_SIZE(1) + MAX_WIFI_ELEMS*JSON_OBJECT_SIZE(2) + 121*MAX_WIFI_ELEMS;
    DynamicJsonBuffer jsonBuffer(capacity);
    JsonObject& root = jsonBuffer.parseObject(confFile);
    confFile.close();
    JsonArray& wifinets = root["wifinets"];
    const size_t arraySize = wifinets.size();
    if (arraySize > 0) {
      WiFi.mode(WIFI_AP_STA);
      WiFi.disconnect();
      this->delay(100);
      WiFi.scanNetworks(true);
      this->delay(1000);
      int8_t scanResult = WiFi.scanComplete();
      for (int tryCount = 0; tryCount < 15; tryCount++) {
        if (scanResult >= 0) break;
        else {
          this->delay(1000); 
          scanResult = WiFi.scanComplete();
        }
      }
      if (scanResult > 0) {
        for (size_t i = 0; i < arraySize; i++) {
          const char *ssid = wifinets[i]["ssid"];
          const char *pass = wifinets[i]["pass"];
          int pos = isAvailable(ssid);
          if (pos >= 0) {
            if (strlen(pass) > 0) {
              WiFi.begin(ssid, pass);
            } else {
              WiFi.begin(ssid);
            }
            const time_t startMonit = TimeKeeper::tkNow();
            while ((WiFi.status() != WL_CONNECTED) && (TimeKeeper::tkNow() - startMonit) < 10) {
              this->delay(500);
            }
            if (WiFi.status() == WL_CONNECTED) break;
          }
        }
      }
      WiFi.scanDelete();
    }
}


