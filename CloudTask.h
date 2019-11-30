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

#ifndef _CLOUD_TASK_H_
#define _CLOUD_TASK_H_
#include <Scheduler.h>
#include "TimeKeeper.h"
#include <ArduinoJson.h>
#include <cstring>
#include <pgmspace.h>
#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include "MyHttpClient.h"



#define CLOUD_CHECK_SECS 360

enum CloudTaskStatusCodes {
  CLOUDTASK_OK = 0,
  CLOUDTASK_INVALID_CLOUDCONF = -200,
  CLOUDTASK_AUTHANDGET_1STBEGIN_ERR = -201,
  CLOUDTASK_AUTHANDGET_2NDBEGIN_ERR = -202,
  CLOUDTASK_AUTHANDGET_NOAUTHHEADER = -203,
  CLOUDTASK_AUTHANDPOST_1STBEGIN_ERR = -204,
  CLOUDTASK_AUTHANDPOST_NOAUTHHEADER = -205,
  CLOUDTASK_AUTHANDPOST_2NDBEGIN_ERR = -206,
  CLOUDTASK_SYNCTOCLOUD_INVALIDMYUTCTIME = -207,
  CLOUDTASK_SYNCTOCLOUD_NOTCONNECTED = -208,
  CLOUDTASK_SYNCTOCLOUD_UNABLE_GET_DATALOGPARAMS = -209,
  CLOUDTASK_SYNCTOCLOUD_UNABLE_GET_MSGLOGPARAMS = -210,
  CLOUDTASK_SYNCTOCLOUD_FSNOTOPEN = -211,
  CLOUDTASK_SYNCTOCLOUD_UNABLE_GET_DATESTOOPEN = -212,
  CLOUDTASK_SENDMSGSFROMDATE_INVALIDTS = -213,
  CLOUDTASK_SENDMSGSFROMDATE_NOFILEWITHDATE = -214,
  CLOUDTASK_SENDDATALOGFROMDATE_NOTHINGTOSEND = -215,
  CLOUDTASK_SENDDATALOGFROMDATE_NOFILEWITHDATE = -216,
  CLOUDTASK_SENDDATALOGFROMDATE_INVALIDTS = -217,
  CLOUDTASK_SENDMSGSFROMDATE_NOTHINGTOSEND = 1
};

class SendParams {
public:
  int status;
  time_t lastTS;
  int maxLines;
  time_t now;
  int errno;
  SendParams() : status(0), lastTS(0), maxLines(0), now(0), errno(0) { }
};

class CloudConf {
public:
  char baseUrl[129];
  char certHash[41];
  char login[65];
  char pass[65];
  short enabled;
  CloudConf();

  inline bool isAllValid() {
      if (strlen(baseUrl) < 10) return false;
      if (strlen(baseUrl) > 128) return false;
      if (strlen(certHash) != 40) return false;
      if (strlen(login) < 2) return false;
      if (strlen(login) > 64) return false;
      if (strlen(pass) < 4) return false;
      if (strlen(pass) > 64) return false;
      {
        String httpStr = String(F("http://"));
        String httpsStr = String(F("https://"));
        if ((strstr(baseUrl, httpStr.c_str()) == NULL) && (strstr(baseUrl, httpsStr.c_str()) == NULL)) return false;
      }
      return true;
  }
};

class CloudTask : public Task {
public:
    CloudTask();
    bool isConfAvailable() { return CloudTask::confAvailable; }
    static const size_t jsonBufferCapacity = JSON_OBJECT_SIZE(5)  + 220;

    //obter cloud conf para enviar para cliente
    //jsonForCloudConf -> passar buffer com tamanho adequado

    //update cloud conf a partir de json
    //updateConfParamsFromJson -> pega o CloudCOnf e passa para updateJsonFromConfParams
    
    //read the cloud conf json file and populate conf with the data
    static bool readCloudConf(CloudConf &conf);

    //update CloudConf with json data
    static bool updateConfParamsFromJson(CloudConf &conf, JsonObject& root);

    //update json file from CloudConf 
    static bool updateJsonFromConfParams(CloudConf &conf);

    //jsonobject with cloudconf allocated inside bufferToUse, or NULL if no conf
    static JsonObject* jsonForCloudConf(DynamicJsonBuffer& bufferToUse);

    //caller is responsible for calling http.end()
    static int httpDigestAuthAndGET(CloudConf& conf, const char* urlEntry, WiFiClient& client,
                HTTPClient& http);

    static std::shared_ptr<String> payloadGET(CloudConf &conf, const String& entryPoint, int& httpRetCode);
    static std::shared_ptr<String> payloadPOST(CloudConf &conf, int maxLines, Stream& csvStream, 
        const String& entryPoint, int& httpRetCode);

    static int httpDigestAuthAndCSVPOST(int maxLines, Stream& csvStream, 
                CloudConf& conf, const char* urlEntry, WiFiClient& client, MyHttpClient& http);

    static bool decodeSendParams(const String& jsonString, SendParams& decodedSendParams);

    static bool getDatalogSendParams(CloudConf& conf, SendParams& sendParams);
    static bool getMsglogSendParams(CloudConf& conf, SendParams& sendParams); 
    static bool getDatesToOpen(time_t &msgDate, time_t &logDate, time_t lastTSLog, time_t lastTSMsg, bool checkLog = true, bool checkMsg = true);

    static bool cloudServiceIsReachable();
    static bool isInternetConnected();

protected:
    void loop();

private:
    bool firstRun;
    time_t lastCheck;
    static bool confAvailable;
    bool sentAllDataLogUntilToday;
    bool sentAllMsgLogUntilToday;

    //time_t lastTSDataSent;
    //time_t lastTSMsgSent;

    static bool getEntryPointSendParams(CloudConf& conf, SendParams& sendParams, const String& entryPoint);
    static int sendMsgsFromDate(time_t msgDate, CloudConf& conf, SendParams &msglogSendParams, std::shared_ptr<String> &outPayLoadPtr, int &outHttpCode);
    static int sendDataLogFromDate(time_t logDate, CloudConf& conf, SendParams &datalogSendParams, std::shared_ptr<String> &outPayLoadPtr, int &outHttpCode);
    static String getDigestAuth(String& authReq, const String& username, const String& password, const String& uri, unsigned int counter, const String& method = "GET");
    int syncToCloud(CloudConf& conf);
    static bool canGet204(String &url, String &userAgent);
    
};

#endif
