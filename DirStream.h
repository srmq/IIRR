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

#ifndef _DIRSTREAM_H_
#define _DIRSTREAM_H_

#include "FS.h"
#include <memory>

class DirStream : public Stream {
public:  
  virtual int available() override;
  virtual int read() override;
  virtual int peek() override;
  virtual size_t readBytes(char *buffer, size_t length) override;
  virtual size_t write(uint8_t) override;
  virtual size_t write(const uint8_t *buffer, size_t size) override;
  virtual void flush() override;
  DirStream(std::shared_ptr<Dir> dirToStream);

private:
  std::shared_ptr<Dir> theDir;
  String currFile;
  int posInFile;
  bool avanceNextFile();

};

#endif
