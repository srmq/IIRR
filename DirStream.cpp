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

#include "DirStream.h"

bool DirStream::avanceNextFile() {
  if (!theDir) {
    posInFile = -1;
    return false;
  }
  bool retVal;
  if ((retVal = theDir->next())) {
    currFile = String(',');
    currFile.concat('\"');
    currFile.concat(theDir->fileName());
    currFile.concat('\"');
    posInFile = 0;
  } else {
    posInFile = -1;
  }
  return retVal;
}

int DirStream::available() {
  if (posInFile >= 0) {
    return currFile.length() - posInFile;
  } else {
    return 0;
  }
}

int DirStream::read() {
  if (posInFile < 0) {
    return -1;
  }
  const int retVal = currFile.charAt(posInFile);
  posInFile++;
  if (posInFile == currFile.length()) {
      avanceNextFile();
  }
  return retVal;
}
  
int DirStream::peek() {
  if (posInFile < 0) {
    return -1;
  }
  return currFile.charAt(posInFile);
}

size_t DirStream::readBytes(char *buffer, size_t length) {
  size_t bytesWritten = 0;
  for (size_t i = 0; i < length; i++) {
    const int readVal = read();
    if (readVal != -1) {
      buffer[i] = readVal;
      bytesWritten++;
    } else
      break;
  }
  return bytesWritten;
}
  
size_t DirStream::write(uint8_t) {
  return 0;
}
  
size_t DirStream::write(const uint8_t *buffer, size_t size) {
  return 0;
}
  
void DirStream::flush() {
  
}

DirStream::DirStream(std::shared_ptr<Dir> dirToStream) : Stream(), theDir(dirToStream) {
  posInFile = -1;
  if (theDir) {
    if (theDir->next()) {
      currFile = String('\"');
      currFile.concat(theDir->fileName());
      currFile.concat('\"');
      posInFile = 0;
    }    
  }
}

