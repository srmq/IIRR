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
#include "MyHttpClient.h"
#include <cstring>
#include <cstdio>

/**
 * sendRequest
 * @param type const char *     "GET", "POST", ....
 * @param stream Stream *       data stream for the message body
 * @param size size_t           size for the message body if 0 not Content-Length is send
 * @return -1 if no info or > 0 when Content-Length is set by server
 */
int MyHttpClient::sendRequest(const char * type, Stream * stream, size_t size) {
  Serial.println(F("Called MyHttpClient.sendRequest()"));

  if(!stream) {
      return returnError(HTTPC_ERROR_NO_STREAM);
  }

  // connect to server
  if(!connect()) {
      return returnError(HTTPC_ERROR_CONNECTION_REFUSED);
  }

  if(size > 0) {
      addHeader("Content-Length", String(size));
  } else {
      addHeader("Transfer-encoding", "chunked");
  }

  // send Header
  if(!sendHeader(type)) {
      return returnError(HTTPC_ERROR_SEND_HEADER_FAILED);
  }
  _client->setTimeout(30000);
  if(size > 0) {
    _client->write(*stream);
  } else {
    uint8_t buf[81];
    uint8_t linelen[6];
    int bytesRead;
    do {
      memset(buf, 0, sizeof(buf));
      bytesRead = stream->readBytesUntil('\n', buf, 80);
      if(bytesRead > 0) {
        snprintf((char *)linelen, 6, "%x\r\n", (bytesRead+1));
        _client->write(linelen, strlen((char*)linelen));
        _client->write(buf, bytesRead);
        _client->write((uint8_t)'\n');
      } else {
        _client->write('0');
        _client->write((uint8_t)'\r');
        _client->write((uint8_t)'\n');
      }
      _client->write((uint8_t)'\r');
      _client->write((uint8_t)'\n');
    } while(bytesRead > 0);
  }
  
  // handle Server Response (Header)
  return returnError(handleHeaderResponse());
}
