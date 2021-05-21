#include "server/include/server.h"

using namespace otaserver;

using std::placeholders::_1;

OTAServer::OTAServer(QObject* parent) : QObject(parent) {
  server_ = new TcpServer;
  server_->set_new_conn_cb(std::bind(&OTAServer::newConnection, this, _1));
  server_->set_close_conn_cb(std::bind(&OTAServer::closeConnection, this, _1));
  server_->set_msg_cb(std::bind(&OTAServer::readMessage, this, _1));

  server_->set_idle_timeout(5000);
  // server generate public.crt and private.pem

  // openssl genrsa -out private.pem 2048

  // openssl req -new -key private.pem -out server.csr -subj
  // "/C=CN/ST=myprovince/L=mycity/O=myorganization/OU=mygroup/CN=myServer

  // openssl x509 -req -days 99999 -in server.csr -signkey private.pem -out
  // public.crt
  server_->set_certificate("ssl/public.crt", "ssl/private.pem");
}

OTAServer::~OTAServer() {
  if (server_) delete server_;
  server_ = nullptr;
}

void OTAServer::quit() { server_->quit(); }

void OTAServer::start(quint16 port) {
  auto address = InetAddress(port);
  print<GeneralInfoCtrl>(std::cout, "Server listen:", address.to_string());
  server_->start(address);
}

void OTAServer::newConnection(TcpConnectionPtr conn) {
  print<GeneralSuccessCtrl>(std::cout, conn->peer_address().to_string());
}

void OTAServer::closeConnection(TcpConnectionPtr conn) {
  print<GeneralWarnCtrl>(std::cout, conn->peer_address().to_string());
}

///
/// \brief OTAServer::readMessage
/// \param conn
/// \return
/// return true => Read data completely. Start to send all data at once
/// return false => The received data is incomplete and needs to be read again
///
bool OTAServer::readMessage(TcpConnectionPtr conn) {
  // receive all data
  std::string data(conn->recver()->peek(), conn->recver()->readable());
  conn->recver()->retired_all();

  print<GeneralInfoCtrl>(std::cout, data);

  // send message to client
  conn->sender()->append(data.data(), data.size());
  return true;
}
