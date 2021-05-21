#ifndef OTASERVER_SERVER_H
#define OTASERVER_SERVER_H

#include <QObject>

#include "TcpServer.h"
#include "otalib/logger/logger.h"

using namespace otaserver::net;
using namespace otalib;

namespace otaserver {
class OTAServer : public QObject {
  Q_OBJECT
 public:
  explicit OTAServer(QObject* parent = nullptr);
  ~OTAServer();

  void start(quint16 port = 5555);
  void quit();

 private:
  void newConnection(TcpConnectionPtr);
  void closeConnection(TcpConnectionPtr);
  bool readMessage(TcpConnectionPtr);

 private:
  TcpServer* server_;
};
}  // namespace otaserver

#endif
