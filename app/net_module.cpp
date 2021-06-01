#include "net_module.hpp"

namespace otalib::app {
//
NetModule::NetModule(const QString& ip_addr, const quint16& port) noexcept
    : socket_(ip_addr, port) {}

bool NetModule::Connect(quint32 timeout) {
  return socket_.connectServer(timeout, &error_);
}

void NetModule::Close() { socket_.close(); }

bool NetModule::Send(const QByteArray& data) { return 0 == socket_.send(data); }

bool NetModule::Recv() {
  error_ = socket_.recv();
  return error_ == SSL_ERROR_ZERO_RETURN;
}

QByteArray NetModule::GetData() {
  //
  QByteArray data(socket_.recver()->peek(), socket_.recver()->readable());
  socket_.recver()->retired_all();
  return data;
}

}  // namespace otalib::app
