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

#ifndef _SERVER_TASK_H_
#define _SERVER_TASK_H_
#include "sensor_calibration.h"
#include <Scheduler.h>
#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include "TimeKeeper.h"


/*
 * 
 * enum wl_enc_type {  // Values map to 802.11 encryption suites... 
        ENC_TYPE_WEP  = 5,
        ENC_TYPE_TKIP = 2,
        ENC_TYPE_CCMP = 4,
        ... except these two, 7 and 8 are reserved in 802.11-2007 
        ENC_TYPE_NONE = 7,
        ENC_TYPE_AUTO = 8
};
 */

enum ServerTaskStatusCodes {
  SERVERTASK_OK = 0,
  SERVERTASK_ERROR_SMALLARRAY = -1,
  SERVERTASK_HANDLE_GETWIFINETWITHID_NOPARAM_ID = -2,
  SERVERTASK_HANDLE_GETWIFINETWITHID_NOTANUMBER_ID = -3,
  SERVERTASK_HANDLE_GETWIFINETWITHID_STILLSCAN = -4,
  SERVERTASK_HANDLE_GETWIFINETWITHID_NONETSFOUND = -5,
  SERVERTASK_HANDLE_GETWIFINETWITHID_IDOUTOFBOUNDS = -6,
  SERVERTASK_HANDLE_WIFICONNECT_NOPARAM_SSID = -7,
  SERVERTASK_HANDLE_WIFICONNECT_INVALIDPARAM_SSID = -8,
  SERVERTASK_HANDLE_WIFICONNECT_INVALIDPARAM_PASS = -9, 
  SERVERTASK_HANDLE_UPDATEMAINCONFPARAMS_INVALIDPARAMS = -10,
  SERVERTASK_HANDLE_UPDATEMAINCONFPARAMS_ERRORWRITEJSON = -11,
  SERVERTASK_HANDLE_GETLOGDIRCONTENTS_FSNOTOPEN = -12,
  SERVERTASK_HANDLE_GETCSVFILE_FSNOTOPEN = -13,
  SERVERTASK_HANDLE_GETCSVFILE_NOPARAM_FILE = -14,
  SERVERTASK_HANDLE_GETCSVFILE_FILENOTFOUND = -15,
  SERVERTASK_HANDLE_UPDATEUTCTIME_INVALIDPARAMS = -16
};

enum HTTPStatus {
  HTTP_OK = 200,
  HTTP_ACCEPTED = 202,
  HTTP_BAD_REQUEST = 400,
  HTTP_NOT_FOUND = 404,
  HTTP_REQUEST_TIMEOUT = 408,
  HTTP_INTERNAL_ERROR = 500
};

class ServerTask : public Task {
private:
  void loopServerMode();
  static void authenticateAndExecute(ServerTask *taskServer, std::function<void (ServerTask *srvP)> funcToCall);  
  static void handleRoot(ServerTask *taskServer);
  static void handleWifiScanNets(ServerTask *taskServer);
  static void handleWifiScanComplete(ServerTask *taskServer);
  static void handleGetWifiNetWithId(ServerTask *taskServer);
  static void handleWifiConnect(ServerTask *taskServer);
  static void handleWifiConnectStatus(ServerTask *taskServer);
  static void handleGetMainConfParams(ServerTask *taskServer);
  static void handleUpdateMainConfParams(ServerTask *taskServer);
  static void handleGetLogDirContents(ServerTask *taskServer);
  static void handleGetCSVFile(ServerTask *taskServer);
  static void handleLearnWaterFlow(ServerTask *taskServer);
  static void handleLearnWaterFStatus(ServerTask *taskServer);
  static void handleResetWaterFStatus(ServerTask *taskServer);
  static void handleGetSoilMoisture(ServerTask *taskServer);
  static void handleGetIrrigData(ServerTask *taskServer);
  static void handleGetMyUTCTime(ServerTask *taskServer);
  static void updateUTCTime(ServerTask *taskServer);
  
  static void sendJsonWithStatusOnly(ServerTaskStatusCodes taskStatusCode, HTTPStatus httpStatus);
  
  static int getAdminPassword(char outPassword[], int maxLen);

  static void onWifiDisconnected(const WiFiEventStationModeDisconnected& event);

  static void initializeAPMode();

  static bool hasInitialized;

protected:
  void loop();

public:
    ServerTask();
    void wifiScanNets();
    void updateTimeDate();
    TimeKeeper *getTimeKeeper();
    static inline bool initializationFinished() { return hasInitialized; }    

};

#endif
