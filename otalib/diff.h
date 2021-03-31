#ifndef DIFF_H
#define DIFF_H

#include <memory>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QTextStream>

#include "bsdiff/bsdiff.h"
#include "bsdiff/bspatch.h"

namespace ota::diff {

/* ----Info In The Log---- */
/*   [Action][File/Dir][FileName]    */

class BsDiff {
 public:
  BsDiff() noexcept {}

  static bool generate(QDir &oldfile, QDir &newfile, QDir &rollback_dest,
                       QDir &update_dest) noexcept;
};

class bspatch {
  bspatch_stream stream_;

 public:
  bspatch() noexcept;

  void patch();
};

}  // namespace ota::diff

#endif  // DIFF_H
