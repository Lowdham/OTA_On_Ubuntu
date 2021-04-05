#ifndef DIFF_H
#define DIFF_H

#include <QByteArray>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMap>
#include <QTextStream>
#include <memory>
#include <string>

#include "bsdiff/bsdiff.h"
#include "bsdiff/bspatch.h"
#include "delta_log.h"

namespace otalib::bs {

/* ----Info In The Log---- */
/*   [Action][File/Dir][FileName]    */
/* -------Example--------- */
/* [add/delete/delta/error][File/Dir]*/

// Generate update pack and rollback pack.
bool generateDeltaPack(QDir& oldfile, QDir& newfile, QDir& rollback_dest,
                       QDir& update_dest) noexcept;

// Apply the delta pack to update/rollback app.
bool applyDeltaPack(QDir& pack, QDir& target) noexcept;

}  // namespace otalib::bs

#endif  // DIFF_H
