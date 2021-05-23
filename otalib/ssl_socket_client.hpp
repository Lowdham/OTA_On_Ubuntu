#ifndef OTALIB_SSL_SOCKET_CLIENT_HPP
#define OTALIB_SSL_SOCKET_CLIENT_HPP

#include <arpa/inet.h>
#include <fcntl.h>
#include <openssl/ssl3.h>
#include <openssl/tls1.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QByteArray>
#include <QString>

#include "../server/include/Buffer.h"
#include "otaerr.hpp"

namespace otaserver::net {
class Buffer;
}

// 16K
constexpr static const size_t kMaxBuffer = 16 * 1024;
using DataSize = unsigned long long;
namespace otalib {

class SSLSocketClient {
 public:
  explicit SSLSocketClient(const QString& ip, quint16 port);

  using ProgressCallback = std::function<void(
      const char* data, DataSize totalSize, DataSize chunkSize)>;

  ~SSLSocketClient();
  bool connectServer(quint32 timeout_second, int* error);

  void setProgressCb(const ProgressCallback& cb) {
    progressCb_ = std::move(cb);
  }

  bool send(const char* data, size_t size);
  bool send(const std::string& data);
  bool send(const QByteArray& data);

  void recv();

  decltype(auto) sender() { return &sender_; }
  decltype(auto) recver() { return &recver_; }

  void close();

 private:
  void initSSLSocket();

 private:
  int sockfd_;
  SSL_CTX* ctx_;
  SSL* ssl_;
  bool closed_;
  struct sockaddr_in si_;

  ProgressCallback progressCb_;
  otaserver::net::Buffer sender_;
  otaserver::net::Buffer recver_;
};
}  // namespace otalib

#endif
