#include "delta_log.h"

namespace otalib {
namespace {

#define INFO_LOAD_ERROR_CHECK                                                 \
  if (list.isEmpty())                                                         \
    return DeltaInfo{Action::ERRORACT, Category::UNEXPECTED_ERROR, QString(), \
                     "Info-Load-Error"};

// Receive the action block and shift.
DeltaInfo receiveLine(const QString& line) noexcept {
  DeltaInfo info;
  QStringList list = line.split("|");

  // Receive the "Action" field.
  INFO_LOAD_ERROR_CHECK
  const auto& action = list.front();
  if (action == "ADD")
    info.action = Action::ADD;
  else if (action == "DELETE")
    info.action = Action::DELETEACT;
  else if (action == "DELTA")
    info.action = Action::DELTA;
  else if (action == "ERROR")
    info.action = Action::ERRORACT;
  else
    info.action = Action::UNEXPECTED_ERROR;

  // Receive the "Category" field.
  list.pop_front();
  INFO_LOAD_ERROR_CHECK
  const auto& category = list.front();
  if (category == "DIR")
    info.category = Category::DIR;
  else if (category == "FILE")
    info.category = Category::FILE;
  else
    info.category = Category::UNEXPECTED_ERROR;

  // Receive the "Position" field.
  list.pop_front();
  INFO_LOAD_ERROR_CHECK
  info.position = list.front();

  info.opaque = QString();
  if (info.action == Action::DELTA && info.category == Category::FILE) {
    list.pop_front();
    if (!list.isEmpty()) info.opaque = list.front();
  } else if (info.action == Action::ERRORACT) {
    list.pop_front();
    if (!list.isEmpty()) info.opaque = list.front();
  }
  return info;
}

}  // namespace

void writeDeltaLog(QTextStream& log, const DeltaInfo& info) noexcept {
  // Stringlize the "Action" field.
  switch (info.action) {
    case Action::ADD:
      log << "ADD|";
      break;
    case Action::DELETEACT:
      log << "DELETE|";
      break;
    case Action::DELTA:
      log << "DELTA|";
      break;
    case Action::ERRORACT:
      log << "ERROR|";
      break;
    default:
      log << "UNEXPECTED_ERROR|";
      break;
  }

  // Stringlize the "Category" field.
  switch (info.category) {
    case Category::DIR:
      log << "DIR|";
      break;
    case Category::FILE:
      log << "FILE|";
      break;
    default:
      log << "UNEXPECTED_ERROR|";
      break;
  }

  // Stringlize the "Position" field.
  if (!info.position.isEmpty()) {
    log << info.position;
    if (!info.opaque.isEmpty())
      log << "|" + info.opaque + "\n";
    else
      log << "\n";
  }
}

DeltaInfoStream readDeltaLog(QTextStream& log) noexcept {
  DeltaInfoStream stream;
  QString line;
  while (log.readLineInto(&line)) stream.push_back(receiveLine(line));
  return stream;
}

}  // namespace otalib
