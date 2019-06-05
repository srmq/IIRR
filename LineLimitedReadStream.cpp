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
#include "LineLimitedReadStream.h"
#include <algorithm>

LineLimitedReadStream::LineLimitedReadStream(Stream& internalStream, 
    const int maxLines) : Stream(), internalStream(internalStream), maxLines(maxLines), linesRead(0) {
       

}

int LineLimitedReadStream::available() {
  if (linesRead >= maxLines) {
    return 0;
  }
  const int linesToGo = maxLines - linesRead;
  const int numAvailable = min(linesToGo, internalStream.available());
  Serial.print(F("LineLimitedReadStream returning number of available bytes: "));
  Serial.println(numAvailable);
  return numAvailable;
}

int LineLimitedReadStream::read() {
  int retByte = -1;
  if(linesRead < maxLines) {
    retByte = internalStream.read();
    if (retByte == '\n') {
      linesRead++;
    }
  }
  return retByte;
}

int LineLimitedReadStream::peek() {
  if (linesRead >= maxLines) return -1;
  return this->internalStream.peek();
}

size_t LineLimitedReadStream::readBytes(char *buffer, size_t length) {
  size_t bytesRead = 0;
  while(bytesRead < length && internalStream.available() > 0 && linesRead < maxLines) {
    int bRead = internalStream.read();
    if (bRead == '\n') {
      linesRead++;
    }
    if(bRead >= 0) {
      buffer[bytesRead++] = (char)bRead;
    }
  }
  return bytesRead;
}
 
size_t LineLimitedReadStream::write(uint8_t) {
  return 0; //ignore, read only stream
}

size_t LineLimitedReadStream::write(const uint8_t *buffer, size_t size) {
  return 0;  //ignore, read only stream
}

void LineLimitedReadStream::flush() {
  
}
