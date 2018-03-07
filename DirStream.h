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
