#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QProcess>

#include "app/control.hpp"
#include "otalib/file_logger.h"
using namespace otalib;
using namespace otalib::app;
int main(int argc, char *argv[]) {
  QCoreApplication a(argc, argv);
  QCommandLineOption op_update("u");
  QCommandLineOption op_ver("v");
  QCommandLineOption op_app_ver_test("t");

  QCommandLineParser parser;
  parser.addOption(op_update);
  parser.addOption(op_ver);
  parser.addOption(op_app_ver_test);

  parser.process(a);

  bool op_update_set = parser.isSet(op_update);
  bool op_ver_set = parser.isSet(op_ver);
  bool op_app_ver_test_set = parser.isSet(op_app_ver_test);

  if (op_update_set) {
    printf("\n");
    QProcess update_process;
    if (!update_process.startDetached(kUpdateModule)) {
      print<GeneralFerrorCtrl>(::std::cerr, "Cannot execute update program.");
    }
    a.exit();
    return 0;
  }

  if (op_app_ver_test_set) {
    // Test
    print<GeneralInfoCtrl>(::std::cout, "This binary's version is 0.0.1");
  }

  if (op_ver_set) {
    // output version info and hash.
    Property app = ReadProperty();
    QString info = "\napp name: " + app.app_name_ + "\n";
    info += "app type: " + app.app_type_ + "\n";
    info += "app version: " + app.app_version_ + "\n";
    info += "app hash: " +
            QString::fromStdString(
                FileLogger::GetHashFromLogFile(kFileLogPath).to_string()) +
            "\n";
    print<GeneralInfoCtrl>(::std::cout, info);
  }
  a.exit();
  return 0;
}
