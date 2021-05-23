#ifndef OTALIB_SSL_SOCKET_CLIENT_HPP
#define OTALIB_SSL_SOCKET_CLIENT_HPP

#include <arpa/inet.h>
#include <fcntl.h>
#include <openssl/ssl3.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#include <QByteArray>
#include <QString>

#include "buffer.hpp"
#include "otaerr.hpp"

namespace otalib {

using DataSize = unsigned long long;

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

  int send(const char* data, size_t size);
  int send(const std::string& data);
  int send(const QByteArray& data);

  int recv();

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
  Buffer sender_;
  Buffer recver_;
};
}  // namespace otalib

#endif
