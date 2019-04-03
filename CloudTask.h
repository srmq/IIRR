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

#define CLOUD_CHECK_SECS 240

typedef struct cloud_conf {
  char baseUrl[129];
  char certHash[41];
  char login[65];
  char pass[65];
  short enabled;
} CloudConf;

class CloudTask : public Task {
public:
    CloudTask();
    bool isConfAvailable() { return CloudTask::confAvailable; }

    //obter cloud conf para enviar para cliente
    //jsonForCloudConf -> passar buffer com tamanho adequado

    //update cloud conf a partir de json
    //updateConfParamsFromJson -> pega o CloudCOnf e passa para updateJsonFromConfParams
    
    //read the cloud conf json file and populate conf with the data
    static bool readCloudConf(CloudConf &conf);

    //update CloudConf with json data
    static bool updateConfParamsFromJson(CloudConf &conf, JsonObject *root);

    //update json file from CloudConf 
    static bool updateJsonFromConfParams(CloudConf &conf);

    //jsonobject with cloudconf allocated inside bufferToUse, or NULL if no conf
    static JsonObject* jsonForCloudConf(DynamicJsonBuffer& bufferToUse);



protected:
    void loop();

private:
    bool firstRun;
    time_t lastCheck;
    static bool confAvailable;
    
};

#endif
