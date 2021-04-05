#include <QCoreApplication>
#include <fstream>
#include <iostream>

#include "../../otalib/diff.h"

#ifdef DIFFTEST
int main(int argc, char* argv[]) {
  QCoreApplication a(argc, argv);
  QDir v1("./complete_pack/v1");
  QDir v2("./complete_pack/v2");
  QDir rp("./delta_pack/rbp v2-v1");
  QDir up("./delta_pack/udp v1-v2");
  if (::otalib::bs::generateDeltaPack(v1, v2, rp, up)) std::cout << "success";
  return a.exec();
}
#endif
