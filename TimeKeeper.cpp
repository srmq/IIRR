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


TimeKeeper::TimeKeeper() : ntpUDP(), ntpTimeClient(ntpUDP, "pool.ntp.org", 0, 3600000) {
}

static const char NTPTIME_NOT_UPDATED_MSG[] PROGMEM = "WARNING: TimeKeeper - NTP time NOT updated";

time_t TimeKeeper::syncTime() {
  bool timeUpdated = false;
  bool shouldUpdate = this->ntpTimeClient.shouldUpdate();
  if (WiFi.status() == WL_CONNECTED && shouldUpdate) {
    timeUpdated = this->ntpTimeClient.update();
  }
  if (!timeUpdated && shouldUpdate) { 
    Serial.println(FPSTR(NTPTIME_NOT_UPDATED_MSG));
    return 0;
  }
  if (timeUpdated) {
    time_t ntpTime = this->ntpTimeClient.getEpochTime();
    setTime(ntpTime);
  }
  return tkNow();
}
