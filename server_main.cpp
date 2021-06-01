#include "server/include/server.h"

#include <QCoreApplication>
using namespace otaserver;

OTAServer server;
void signal_handler(int) {
  server.quit();
  print<GeneralInfoCtrl>(std::cout, "OTAServer quit!");
}

int main(int argc, char* argv[]) {
  QCoreApplication a(argc, argv);
  server.start();
  return a.exec();
}
