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
