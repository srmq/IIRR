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
#ifndef _WIFI_TASK_H_
#define _WIFI_TASK_H_
#include <Scheduler.h>
#include "TimeKeeper.h"
#include <ArduinoJson.h>
#include "ServerTask.h"
#include "FS.h"

#define WIFI_CHECK_SECONDS 180
#define MAX_WIFI_ELEMS 10

class WiFiTask : public Task {

public:
    WiFiTask();
    static bool addToTop(const String& newssid, const String& newpass);

protected:
  void loop();

private:
  time_t lastCheck;
  bool firstRun;
  void loopWiFis(String& fileName);
  int isAvailable(const char* ssid);

};
#endif
