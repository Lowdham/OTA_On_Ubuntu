#ifndef DELTA_LOG_H
#define DELTA_LOG_H

#include <QFile>
#include <QTextStream>
#include <cstdint>

namespace otalib {

enum class Action { ADD, DELETE, DELTA, ERROR, UNEXPECTED_ERROR };
enum class Category { FILE, DIR, UNEXPECTED_ERROR };
/* Info pattern
 *  Normal info: action|category|position|
 *  Error info : error |category|position|error-msg
 */
struct DeltaInfo {
  Action action;
  Category category;
  QString position;
  QString opaque;
};

using DeltaInfoStream = QVector<DeltaInfo>;

void writeDeltaLog(QTextStream& log, const DeltaInfo& info) noexcept;

DeltaInfoStream readDeltaLog(QTextStream& log) noexcept;

}  // namespace otalib

#endif  // DELTA_LOG_H
