
#include <QCoreApplication>
#include <QFile>

#include "otalib/ssl_socket_client.hpp"
#include "server/include/server.h"
using namespace otalib;
using namespace otaserver;

// OTAServer server;

// void signal_handler(int) {
//  server.quit();
//  print<GeneralInfoCtrl>(std::cout, "Server quit!");
//}

int main(int argc, char* argv[]) {
  QCoreApplication a(argc, argv);

  SSLSocketClient client("127.0.0.1", 5555);
  int err;
  client.connectServer(5, &err);
  client.setProgressCb([&](const char*, DataSize total, DataSize chunk) {
    print<GeneralDebugCtrl>(std::cout, total, chunk);
  });

  print<GeneralDebugCtrl>(std::cout, err, strerror(err));

  QByteArray bytes("hello 你好!");
  client.send(bytes);

  client.recv();
  print<GeneralInfoCtrl>(std::cout, "Read completely!");

  // read message from buffer
  std::string data(client.recver()->peek(), client.recver()->readable());
  print<GeneralInfoCtrl>(std::cout, data.size());

  QFile file("output.file");
  file.open(QFile::WriteOnly);
  file.write(data.data(), data.size());

  client.close();

  //  server.start();

  return a.exec();
}
