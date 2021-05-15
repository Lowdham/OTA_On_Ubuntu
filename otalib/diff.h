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
#include "logger/logger.h"
#include "otaerr.hpp"
#include "shell_cmd.hpp"

namespace otalib::bs {

/* ----Info In The Log---- */
/*   [Action][File/Dir][FileName]    */
/* -------Example--------- */
/* [add/delete/delta/error][File/Dir]*/

// Generate update pack and rollback pack.
bool generateDeltaPack(QDir& oldfile, QDir& newfile, QDir& rollback_dest,
                       QDir& update_dest);

// Apply the delta pack to update/rollback app.
bool applyDeltaPack(const QDir& pack, const QDir& target);

}  // namespace otalib::bs

#endif  // DIFF_H
