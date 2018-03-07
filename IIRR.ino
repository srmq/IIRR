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
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <cmath>
#include "sensor_calibration.h"
#include "SensorTask.h"
#include "ServerTask.h"
#include <Scheduler.h>
#include <WiFiUdp.h>
#include <NTPClient.h>
#include "FS.h"
#include <TimeLib.h>
#include <Time.h>


void loop() {
  Serial.println(F("WARNING: loop() was called"));
}

void setup() {
  Serial.begin(9600);
  delay(1000);
  static SensorTask sensorTask;
  static ServerTask serverTask;

  Scheduler.start(&sensorTask);
  Scheduler.start(&serverTask);  
  Serial.println(F("SETUP"));
  Scheduler.begin();
}

//Medidas no jarro: seco, 0.22 nos tres
//Depois de colocar 1,5 litro (3,4 corresponderia a 1cm de agua
//para o vaso) os tres foram para cerca de 0,62 e escorreu um 
//pouco de agua
//Depois de uns 10 minutos os sensores estavam todos em 100%

