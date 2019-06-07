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

#include "TimeKeeper.h"
#include <TimeLib.h>
#include <Time.h>
#include <functional>
#include <ESP8266WiFi.h>

#include "ESP8266WiFi.h"
#include "ESP8266HTTPClient.h"

#include "HttpDateParser.h"


TimeKeeper::TimeKeeper() : ntpUDP(), ntpTimeClient(ntpUDP, "pool.ntp.org", 0, 3600000) {
}

static const char NTPTIME_NOT_UPDATED_MSG[] PROGMEM = "WARNING: TimeKeeper - NTP time NOT updated, trying HTTP";
static const char HTTPTIME_NOT_UPDATED_MSG[] PROGMEM = "WARNING: TimeKeeper - HTTP fallback time update FAILED";
static const char HTTPTIME_UPDATE_URL[] PROGMEM = "http://www.ntp.org"; //FIXME should be configurable and more than one option

static bool tryHTTPUpdate(tmElements_t& timeStruct) {
  const char * headerKeys[] = {"Date"};
  const size_t numberOfHeaders = 1;
  bool result = false;
  if (WiFi.status() == WL_CONNECTED) { //Check WiFi connection status
    WiFiClient client;
    HTTPClient http;
    const String updateUrl = String(FPSTR(HTTPTIME_UPDATE_URL));
    http.begin(client, updateUrl); 
    http.collectHeaders(headerKeys, numberOfHeaders);
 
    int httpCode = http.GET();
 
    if (httpCode > 0) {
      String headerDate = http.header("Date");
      result = HttpDateParser::parseHTTPDate(headerDate, timeStruct);
    }
 
    http.end();
 
  }
  return result;
}

time_t TimeKeeper::syncTime() {
  bool timeUpdated = false;
  bool shouldUpdate = this->ntpTimeClient.shouldUpdate();
  bool ntpUpdated = false;
  bool httpUpdated = false;
  if (WiFi.status() == WL_CONNECTED && shouldUpdate) {
    timeUpdated = this->ntpTimeClient.update();
    ntpUpdated = timeUpdated;
  }
  time_t newTime;
  if (!timeUpdated && shouldUpdate) { 
    Serial.println(FPSTR(NTPTIME_NOT_UPDATED_MSG));
    tmElements_t timeStruct;
    httpUpdated = tryHTTPUpdate(timeStruct);
    if (!httpUpdated) {
      Serial.println(FPSTR(HTTPTIME_NOT_UPDATED_MSG));
      return 0;
    } else {
      newTime = makeTime(timeStruct);
    }
  }
  if (ntpUpdated || httpUpdated) {
    if (ntpUpdated) newTime = this->ntpTimeClient.getEpochTime();
    setTime(newTime);
  }
  return tkNow();
}

time_t TimeKeeper::firstValidTime() {
  tmElements_t timeStruct;
  timeStruct.Second = 0;
  timeStruct.Minute = 0;
  timeStruct.Hour = 0;
  timeStruct.Wday = 6;
  timeStruct.Day = 1;
  timeStruct.Month = 1;
  timeStruct.Year = (2016 - 1970);
 
  return TimeKeeper::tkMakeTime(timeStruct);
}
