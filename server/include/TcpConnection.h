#ifndef OTASERVER_NET_TCPCONNECTION_H
#define OTASERVER_NET_TCPCONNECTION_H

#include <functional>

#include "../include/ServerSocket.h"
#include "Buffer.h"

#define SUPPORT_SSL_LIB

#ifdef SUPPORT_SSL_LIB
#include "SSL.h"
#endif

namespace otaserver {
namespace net {

class TcpConnection;
using TcpConnectionPtr = TcpConnection *;
class TcpConnection {
 public:
#ifdef SUPPORT_SSL_LIB
  SSL *ssl() const { return ssl_; }
  void set_ssl(SSL *ssl) { ssl_ = ssl; }
#endif

  void initialize(int connfd);

  int fd() const noexcept { return connfd_; }
  InetAddress local_address() { return ServerSocket::sockname(connfd_); }
  InetAddress peer_address() { return ServerSocket::peername(connfd_); }

  int read(int *err);
  int write(int *err);

  bool disconnected() const noexcept { return disconnected_; }
  void disconnected(bool is) { disconnected_ = is; }

  decltype(auto) sender() { return &sender_; }
  decltype(auto) recver() { return &recver_; }

 private:
  int connfd_;

#ifdef SUPPORT_SSL_LIB
  SSL *ssl_;
#endif

  bool disconnected_;

  Buffer sender_;
  Buffer recver_;
};
}  // namespace net
}  // namespace otaserver

#endif
