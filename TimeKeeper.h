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

#ifndef _TIME_KEEPER_H_
#define _TIME_KEEPER_H_

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include <TimeLib.h>
#include <Time.h>


class TimeKeeper {
private:
  WiFiUDP ntpUDP;
  NTPClient ntpTimeClient;

public:
  TimeKeeper();
  static inline int tkYear() {
    return year();
  }
  static inline int tkYear(time_t ts) {
    return year(ts);
  }
  static inline int tkMonth() {
    return month();
  }
  static inline int tkMonth(time_t ts) {
    return month(ts);
  }

  static inline int tkDay() {
    return day();    
  }
  
  static inline int tkDay(time_t ts) {
    return day(ts);    
  }
  
  static inline int tkHour() {
    return hour();
  }

  static inline int tkHour(time_t ts) {
    return hour(ts);
  }

  static inline int tkMinute() {
    return minute();
  }
  static inline int tkMinute(time_t ts) {
    return minute(ts);
  }
  
  static inline int tkSecond() {
    return second();
  }

  static inline int tkSecond(time_t ts) {
    return second(ts);
  }

  
  static inline time_t tkNow() {
    return now();
  }

  inline void setDelayFunction(NTPClient::DelayHandlerFunction delayHandler) {
    this->ntpTimeClient.setDelayFunction(delayHandler);
  }
  
  time_t syncTime();
  static inline bool isValidTS(time_t ts) {
    return tkYear(ts) > 2016;
  }

  static inline time_t tkMakeTime(const tmElements_t& tm) {
    return makeTime(tm);
  }

  static inline void tkBreakTime(time_t aTime, tmElements_t& outTm) {
    breakTime(aTime, outTm);
  }

  static inline void tkSetTime(int aYear, int aMonth, int aDay, int aHour, int aMin, int aSec) {
    setTime(aHour, aMin, aSec, aDay, aMonth, aYear);
  }

  static inline time_t tkMakeTime(int aYear, int aMonth, int aDay, int aHour, int aMin, int aSec) {
    tmElements_t timeStruct;
    timeStruct.Second = aSec;
    timeStruct.Minute = aMin;
    timeStruct.Hour = aHour;
    timeStruct.Day = aDay;
    timeStruct.Month = aMonth;
    timeStruct.Year = aYear - 1970;
    return tkMakeTime(timeStruct);
  }

  static inline void tkSetTime(time_t aTime) {
    setTime(aTime);
  }


  static inline bool isSameDate(time_t tsA, time_t tsB)   {
    return (tkYear(tsA) == tkYear(tsB)) && (tkMonth(tsA) == tkMonth(tsB)) && (tkDay(tsA) == tkDay(tsB));
  }

  static time_t firstValidTime();
};

#endif
