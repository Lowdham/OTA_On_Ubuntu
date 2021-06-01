#ifndef OTASERVER_FILELOADER_HPP
#define OTASERVER_FILELOADER_HPP
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <string>

#include "Buffer.h"

namespace otaserver {
namespace net {

class FileLoader {
 public:
  explicit FileLoader(const std::string &filename)
      : file_name_(filename), fp_(nullptr) {
    fp_ = ::fopen(file_name_.c_str(), "r+");
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

  //  void readAll(net::Buffer *buffer) {
  //    int fd = ::open(file_name_.c_str(), O_RDONLY);
  //    auto size = ::lseek(fd, 0, SEEK_END);
  //    char *mapped = (char *)mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
  //    ::close(fd);
  //    buffer->set_mapped(mapped, size);
  //  }

 private:
  std::string file_name_;
  FILE *fp_;
};
}  // namespace net

}  // namespace otaserver

#endif
