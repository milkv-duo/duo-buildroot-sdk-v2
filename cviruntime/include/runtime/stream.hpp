#ifndef RUNTIME_CVISTREAM_H
#define RUNTIME_CVISTREAM_H

#include <iostream>
#include <fstream>

namespace cvi {
namespace runtime {

class BaseStream {
public:
  BaseStream() {}
  virtual ~BaseStream() {}

  size_t length() {
    return _length;
  }

  virtual size_t read(uint8_t *buf, size_t offset, size_t size) = 0;

protected:
  size_t _length = 0;
};

class FileStream : public BaseStream {
public:
  FileStream(const std::string &file_name);
  ~FileStream();
  size_t read(uint8_t *buf, size_t offset, size_t size);
private:
  std::ifstream *_fstream;
};

class BufferStream : public BaseStream {
public:
  BufferStream(const int8_t *buf, size_t size);
  ~BufferStream() {}
  size_t read(uint8_t *buf, size_t offset, size_t size);

private:
  const int8_t *buffer;
};

class FdStream : public BaseStream {
public:
  FdStream(const int fd, const size_t ud_offset);
  ~FdStream() {};
  size_t read(uint8_t *buf, size_t offset, size_t size);
private:
  int file_descriptor;
  size_t user_define_offset = 0; //The file header's offset that user defined.
};

} // namespace runtime
} // namespace cvi

#endif
