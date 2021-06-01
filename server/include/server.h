#ifndef OTASERVER_SERVER_H
#define OTASERVER_SERVER_H

#include <QFile>
#include <QObject>

#include "../../otalib/pack_apply.hpp"
#include "../../otalib/update_strategy.hpp"
#include "DirectoryWatcher.h"
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

  void initializeEnv();
  void start(quint16 port = 5555);
  void quit();

  void dumpAppClientInfo(const QString& client,
                         const ClientInfo<GeneralVersion>& cinfo);

 private:
  void newConnection(TcpConnectionPtr);
  void closeConnection(TcpConnectionPtr);
  bool readMessage(TcpConnectionPtr);
  void directoryChanged(const QString& version);

  void initDirectoryListener();
  void initVersionMap();

  bool mkDir(const QString& dir);

 private:
  TcpServer* server_;
  VersionMap<GeneralVersion> gVcm_;
  DirectoryWatcher watcher_;
  FILE* indxFp_;
};
}  // namespace otaserver

#endif
