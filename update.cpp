#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>

#include "app/control.hpp"
#include "otalib/file_logger.h"
using namespace otalib;
using namespace otalib::app;
int main(int argc, char* argv[]) {
  QCoreApplication a(argc, argv);
  UpdateModule um;
  try {
    um.TryUpdate();
  } catch (::std::exception& e) {
    print<GeneralFerrorCtrl>(::std::cerr, e.what());
    print<GeneralFerrorCtrl>(::std::cerr, "Update(Rollback) failed. Press[Enter] to quit.");
    a.exit();
    return -1;
  }
  print<GeneralSuccessCtrl>(::std::cout, "Update(Rollback) succeed.");
  a.exit();
  return 0;
}
