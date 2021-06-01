#include "delta_log.h"

namespace otalib {
namespace {

#define INFO_LOAD_ERROR_CHECK                        \
  if (list.isEmpty()) {                              \
    OTAError::S_delta_log_invalid_line xerror{line}; \
    throw OTAError{::std::move(xerror)};             \
  }

#define INFO_LOAD_UNEXPECTED_END                   \
                                                   \
  OTAError::S_delta_log_invalid_line xerror{line}; \
  throw OTAError{::std::move(xerror)};

// Receive the action block and shift.
DeltaInfo receiveLine(const QString& line) {
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
  else {
    INFO_LOAD_UNEXPECTED_END
  }

  // Receive the "Category" field.
  list.pop_front();
  INFO_LOAD_ERROR_CHECK
  const auto& category = list.front();
  if (category == "DIR")
    info.category = Category::DIR;
  else if (category == "FILE")
    info.category = Category::FILE;
  else {
    INFO_LOAD_UNEXPECTED_END
  }
  // Receive the "Position" field.
  list.pop_front();
  INFO_LOAD_ERROR_CHECK
  info.position = list.front();

  info.opaque = QString();
  if (info.action == Action::DELTA && info.category == Category::FILE) {
    list.pop_front();
    if (!list.isEmpty()) info.opaque = list.front();
  }
  return info;
}

}  // namespace

void writeDeltaLog(QTextStream& log, const DeltaInfo& info) {
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
    default: {
      OTAError::S_general xerror{
          QStringLiteral(
              "Meet invalid delta log info, writing delta log failed.") +
          STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }
  }

  // Stringlize the "Category" field.
  switch (info.category) {
    case Category::DIR:
      log << "DIR|";
      break;
    case Category::FILE:
      log << "FILE|";
      break;
      OTAError::S_general xerror{
          QStringLiteral(
              "Meet invalid delta log info, writing delta log failed.") +
          STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
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

DeltaInfoStream readDeltaLog(QTextStream& log) {
  DeltaInfoStream stream;
  QString line;
  while (log.readLineInto(&line)) {
    if (line.trimmed().isEmpty()) continue;
    stream.push_back(receiveLine(line));
  }
  return stream;
}

}  // namespace otalib
