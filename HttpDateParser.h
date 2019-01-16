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

#ifndef _HTTPDATEPARSER_H_
#define _HTTPDATEPARSER_H_

#include <Arduino.h>
#include <Time.h>
#include <TimeLib.h>
#include <cctype>
#include <cstdio>

enum HTTPDateFormat { PREFER, RFC850, CASCTIME };


class HttpDateParser {
public:
  static bool parseHTTPDate(const String& httpDate, tmElements_t& timeStruct);
  
private:
  static int parseDayOfWeek(const String& str);
  static int parseMonth(const String& str);
  static bool parseRFC850Time(const String& dateFromSndTok, tmElements_t& timeStruct);
  static bool parsePreferTime(const String& dateFromSndTok, tmElements_t& timeStruct);
  static bool parseAsctime(const String& dateFromSndTok, tmElements_t& timeStruct);
};
#endif

