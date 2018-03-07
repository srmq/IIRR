/*
  author: srmq@cin.ufpe.br
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

