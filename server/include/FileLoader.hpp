#ifndef OTASERVER_FILELOADER_HPP
#define OTASERVER_FILELOADER_HPP

#include <stdio.h>

#include <string>

#include "Buffer.h"

namespace otaserver {
namespace net {

class FileLoader {
 public:
  explicit FileLoader(const std::string &filename) {
    fp_ = ::fopen(filename.c_str(), "rb");
  }

  ~FileLoader() {
    if (fp_) ::fclose(fp_);
    fp_ = nullptr;
  }

  void readAll(net::Buffer *buffer) {
    if (!fp_) return;
    ::fseek(fp_, 0, SEEK_END);
    auto size = ::ftell(fp_);
    ::rewind(fp_);
    buffer->ensure_writable(size);
    ::fread(buffer->begin_write(), sizeof(char), size, fp_);
    buffer->has_written(size);
  }

 private:
  FILE *fp_;
};
}  // namespace net

}  // namespace otaserver

#endif
