#ifndef OTASERVER_NET_INETADDRESS_H
#define OTASERVER_NET_INETADDRESS_H

#include <netinet/in.h>

#include <QString>

namespace otaserver {
namespace net {

class InetAddress {
 public:
  explicit InetAddress(quint16 port = 0);
  explicit InetAddress(sockaddr_in si) : sockin_(si) {}
  explicit InetAddress(const QString &ip, quint16 port = 0);

  QString ip() const noexcept;
  QString port_s() const noexcept;
  uint16_t port() const noexcept { return ::htons(sockin_.sin_port); }
  struct sockaddr *sockaddr() const noexcept {
    return (struct sockaddr *)&sockin_;
  }
  void set_sockaddr(const sockaddr_in &sockaddr) { sockin_ = sockaddr; }
  QString to_string() const { return ip() + ":" + port_s(); }

 private:
  sockaddr_in sockin_;
};
}  // namespace net
}  // namespace otaserver

#endif
