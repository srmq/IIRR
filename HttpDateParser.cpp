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
#include "HttpDateParser.h"

#include <pgmspace.h>

static const char DAY_SUN[] PROGMEM = "Sun";
static const char DAY_MON[] PROGMEM = "Mon";
static const char DAY_TUE[] PROGMEM = "Tue";
static const char DAY_WED[] PROGMEM = "Wed";
static const char DAY_THU[] PROGMEM = "Thu";
static const char DAY_FRI[] PROGMEM = "Fri";
static const char DAY_SAT[] PROGMEM = "Sat";

static const char MONTH_JAN[] PROGMEM = "Jan";
static const char MONTH_FEB[] PROGMEM = "Feb";
static const char MONTH_MAR[] PROGMEM = "Mar";
static const char MONTH_APR[] PROGMEM = "Apr";
static const char MONTH_MAY[] PROGMEM = "May";
static const char MONTH_JUN[] PROGMEM = "Jun";
static const char MONTH_JUL[] PROGMEM = "Jul";
static const char MONTH_AUG[] PROGMEM = "Aug";
static const char MONTH_SEP[] PROGMEM = "Sep";
static const char MONTH_OCT[] PROGMEM = "Oct";
static const char MONTH_NOV[] PROGMEM = "Nov";
static const char MONTH_DEC[] PROGMEM = "Dec";

/* 0-6 Sunday is 0 */
int HttpDateParser::parseDayOfWeek(const String& str) {
  if (str.length() < 3) return -1;
  String dayName = str.substring(0, 3);
  {
    String trySun = String(FPSTR(DAY_SUN));
    if (dayName.equals(trySun)) return 0;
  }
  {
    String tryMon = String(FPSTR(DAY_MON));
    if (dayName.equals(tryMon)) return 1;
  }
  {
    String tryTue = String(FPSTR(DAY_TUE));
    if (dayName.equals(tryTue)) return 2;
  }
  {
    String tryWed = String(FPSTR(DAY_WED));
    if (dayName.equals(tryWed)) return 3;
  }
  {
    String tryThu = String(FPSTR(DAY_THU));
    if (dayName.equals(tryThu)) return 4;
  }
  {
    String tryFri = String(FPSTR(DAY_FRI));
    if (dayName.equals(tryFri)) return 5;
  }
  {
    String trySat = String(FPSTR(DAY_SAT));
    if (dayName.equals(trySat)) return 6;
  }
  return -1;
}

/* 1-12 Jan is 1 etc. */
int HttpDateParser::parseMonth(const String& str) {
  if (str.length() < 3) return -1;
  String monthName = str.substring(0, 3);
  {
    String tryJan = String(FPSTR(MONTH_JAN));
    if (monthName.equals(tryJan)) return 1;
  }
  {
    String tryFeb = String(FPSTR(MONTH_FEB));
    if (monthName.equals(tryFeb)) return 2;
  }
  {
    String tryMar = String(FPSTR(MONTH_MAR));
    if (monthName.equals(tryMar)) return 3;
  }
  {
    String tryApr = String(FPSTR(MONTH_APR));
    if (monthName.equals(tryApr)) return 4;
  }
  {
    String tryMay = String(FPSTR(MONTH_MAY));
    if (monthName.equals(tryMay)) return 5;
  }
  {
    String tryJun = String(FPSTR(MONTH_JUN));
    if (monthName.equals(tryJun)) return 6;
  }
  {
    String tryJul = String(FPSTR(MONTH_JUL));
    if (monthName.equals(tryJul)) return 7;
  }
  {
    String tryAug = String(FPSTR(MONTH_AUG));
    if (monthName.equals(tryAug)) return 8;
  }
  {
    String trySep = String(FPSTR(MONTH_SEP));
    if (monthName.equals(trySep)) return 9;
  }
  {
    String tryOct = String(FPSTR(MONTH_OCT));
    if (monthName.equals(tryOct)) return 10;
  }
  {
    String tryNov = String(FPSTR(MONTH_NOV));
    if (monthName.equals(tryNov)) return 11;
  }
  {
    String tryDec = String(FPSTR(MONTH_DEC));
    if (monthName.equals(tryDec)) return 12;
  }
  return -1;
}

bool HttpDateParser::parseRFC850Time(const String& dateFromSndTok, tmElements_t& timeStruct) {
  char monthName[4];
  monthName[3] = '\0';
  int day, year, hour, min, second;
  if (sscanf(dateFromSndTok.c_str(), "%2d-%3s-%2d %2d:%2d:%2d", &day, monthName, &year, &hour, &min, &second) < 6) return false;
  year += 2000;
  int month = parseMonth(String(monthName));
  if (month < 1 || month > 12) return false;
  timeStruct.Second = (uint8_t) second;
  timeStruct.Minute = (uint8_t) min;
  timeStruct.Hour = (uint8_t) hour;
  timeStruct.Day = (uint8_t) day;
  timeStruct.Month = (uint8_t) month;
  timeStruct.Year = year - 1970;
  return true;
}

bool HttpDateParser::parsePreferTime(const String& dateFromSndTok, tmElements_t& timeStruct) {
  char monthName[4];
  monthName[3] = '\0';
  int day, year, hour, min, second;
  if (sscanf(dateFromSndTok.c_str(), "%2d %3s %4d %2d:%2d:%2d", &day, monthName, &year, &hour, &min, &second) < 6) return false;
  int month = parseMonth(String(monthName));
  if (month < 1 || month > 12) return false;
  timeStruct.Second = (uint8_t) second;
  timeStruct.Minute = (uint8_t) min;
  timeStruct.Hour = (uint8_t) hour;
  timeStruct.Day = (uint8_t) day;
  timeStruct.Month = (uint8_t) month;
  timeStruct.Year = year - 1970;
  return true;  
}

bool HttpDateParser::parseAsctime(const String& dateFromSndTok, tmElements_t& timeStruct) {
  int month = parseMonth(dateFromSndTok);
  if (month < 1 || month > 12) return false;
  timeStruct.Month = (uint8_t) month;
  int fstSp = dateFromSndTok.indexOf(' ');
  if (fstSp < 3) return false;
  const char* strAfterSndTok = dateFromSndTok.c_str()+fstSp;
  int day, hour, min, second, year;
  if (sscanf(strAfterSndTok, " %d %2d:%2d:%2d %d", &day, &hour, &min, &second, &year) < 5) return false;
  timeStruct.Day = day;
  timeStruct.Hour = hour;
  timeStruct.Minute = min;
  timeStruct.Second = second;
  timeStruct.Year = year - 1970;
  return true;
}

bool HttpDateParser::parseHTTPDate(const String& httpDate, tmElements_t& timeStruct) {
  if (httpDate.length() < 20) return false;
  int weekDay = parseDayOfWeek(httpDate);
  if (weekDay < 0) return false;
  timeStruct.Wday = ((uint8_t)weekDay) + 1; //sunday is 1 in TimeLib

  const int fstSp = httpDate.indexOf(' ');
  if (fstSp < 3) return false;
  if(fstSp > 12) return false;

  const int sndSp = httpDate.indexOf(' ', fstSp+1);
  if (sndSp < 0) return false;

  String sndToken = httpDate.substring(fstSp+1, sndSp);

  HTTPDateFormat dateFormat;

  if (!isdigit(sndToken.charAt(0))) {
    dateFormat = CASCTIME;
  } else if (sndToken.indexOf('-') > 0) {
    dateFormat = RFC850;
  } else dateFormat = PREFER;

  switch(dateFormat) {
    case CASCTIME:
      return parseAsctime(httpDate.substring(fstSp+1), timeStruct);
    case RFC850:
      return parseRFC850Time(httpDate.substring(fstSp+1), timeStruct);
    case PREFER:
      return parsePreferTime(httpDate.substring(fstSp+1), timeStruct);
    default:
      return false;
  }
}


