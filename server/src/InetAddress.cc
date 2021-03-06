#include "../include/InetAddress.h"

#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>

using namespace otaserver::net;

InetAddress::InetAddress(uint16_t port) {
  ::memset(&sockin_, 0, sizeof(sockin_));
  sockin_.sin_port = ::htons(port);
  sockin_.sin_family = AF_INET;
  sockin_.sin_addr.s_addr = htonl(INADDR_ANY);
}

InetAddress::InetAddress(const QString &ip, quint16 port) {
  ::memset(&sockin_, 0, sizeof(sockin_));
  sockin_.sin_port = ::htons(port);
  sockin_.sin_family = AF_INET;
  ::inet_pton(AF_INET, ip.toStdString().c_str(), &sockin_.sin_addr);
}

QString InetAddress::port_s() const noexcept {
  return QString::number(::ntohs(sockin_.sin_port));
}

QString InetAddress::ip() const noexcept {
  char buf[64];
  ::inet_ntop(AF_INET, &sockin_.sin_addr, buf, sizeof(sockin_));
  return QString(buf);
}
