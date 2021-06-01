#ifndef NETMODULE_HPP
#define NETMODULE_HPP

#include "../otalib/ssl_socket_client.hpp"
#include "properties.hpp"

namespace otalib::app {
//
using ::otalib::SSLSocketClient;
class NetModule {
  //
  SSLSocketClient socket_;
  int error_;

 public:
  NetModule(const QString& ip_addr = kIpAddr,
            const quint16& port = kPort) noexcept;

  bool Connect(quint32 timeout = kTimeOut);

  void Close();

  bool Send(const QByteArray& data);

  bool Recv();

  QByteArray GetData();

  inline bool Error() const noexcept { return error_ != 0; }
};

}  // namespace otalib::app

#endif  // NETMODULE_HPP
