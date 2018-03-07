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

  static inline unsigned long tkElapsedDays(time_t from, time_t to) {
    return elapsedDays(to-from);
  }

  static inline bool isSameDate(time_t tsA, time_t tsB)   {
    return (tkYear(tsA) == tkYear(tsB)) && (tkMonth(tsA) == tkMonth(tsB)) && (tkDay(tsA) == tkDay(tsB));
  }
};

#endif
