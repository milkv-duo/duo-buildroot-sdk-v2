#include <iostream>
#include <sstream>
#include <runtime/debug.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <runtime/stream.hpp>

namespace cvi {
namespace runtime {

FileStream::FileStream(const std::string& file_name) {
  _fstream = new std::ifstream(file_name, std::ifstream::binary);
  if (!_fstream->good()) {
    TPU_LOG_ERROR("Error, Failed to open %s\n", file_name.c_str());
    return;
  }
  _fstream->seekg(0, _fstream->end);
  _length = _fstream->tellg();
  _fstream->seekg(0, _fstream->beg);
}

FileStream::~FileStream() {
  if (_fstream)
    delete _fstream;
}

size_t FileStream::read(uint8_t *buf, size_t offset, size_t size) {
  TPU_ASSERT(offset + size <= _length, "model is incomplete or incorrect!");
  _fstream->seekg(offset);
  _fstream->read((char *)buf, size);
  return size;
}

BufferStream::BufferStream(const int8_t *buf, size_t size)
  : buffer(buf) {
  _length = size;
}

size_t BufferStream::read(uint8_t *buf, size_t offset, size_t size) {
  TPU_ASSERT(offset + size <= _length, "model is incomplete or incorrect!");
  memcpy(buf, buffer + offset, size);
  return size;
}

FdStream::FdStream(const int fd, const size_t ud_offset) {
  file_descriptor = fd;
  user_define_offset = ud_offset;
  _length = (size_t)lseek(fd, 0, SEEK_END) - ud_offset;
}

size_t FdStream::read(uint8_t *buf, size_t offset, size_t size) {
  TPU_ASSERT(offset + size <= _length, "model is incomplete or incorrect!");
  //when reading add user_define_offset to the offset
  lseek(file_descriptor, offset + user_define_offset, SEEK_SET);
  size_t sz = ::read(file_descriptor, buf, size);
  return sz;
}

} // namespace runtime
} // namespace cvi

