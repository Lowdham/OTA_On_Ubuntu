#include "diff.h"
namespace ota::diff {
namespace {

constexpr bool debug_mode = true;

static int plainWrite(bsdiff_stream* stream, const void* buffer, int size) {
  // Just write into the file
  if (static_cast<QFile*>(stream->opaque)
          ->write(static_cast<const char*>(buffer), size) == size)
    return 0;
  else
    return -1;
}

void copyDir(const QString& source, const QString& dest) {
#if defined(_WIN64) || defined(_WIN32)
  QString s = "\"" + source + "\"";
  QString d = "\"" + dest + "\"";
  QString cmd("xcopy " + s + " " + d + " /S");
  system(cmd.toStdString().c_str());
  return;
#endif

#ifdef _LINUX
  QString cmd("cp -r " + source + "/* " + dest);
  system(cmd.toStdString().c_str());
  return;
#endif
}

void copyDir(const QDir& source, const QDir& dest) {
  copyDir(source.path(), dest.path());
  return;
}

/* Copy the "newfile" into "tdir" and log "add" in alog, "delete" in dlog. */
bool doAddActionLog(QFile* newfile, const QDir& tdir, const QDir& nroot,
                    QTextStream& alog, QTextStream& dlog) noexcept {
  QFileInfo file(*newfile);
  QString udest_file = tdir.filePath(file.fileName());
  QString position = nroot.relativeFilePath(file.absoluteFilePath());
  // Try to remove the previous files.
  QFile::remove(udest_file);
  dlog << "[delete][file][" + position + "]\n";
  if constexpr (debug_mode) {
    qDebug() << "[DoAddActionLog][File]";
    qDebug() << "[Source: " + file.absoluteFilePath() + "]";
    qDebug() << "[Dest: " + tdir.absolutePath() + "]";
    qDebug() << "[Position: " + position + "]";
  }
  if (QFile::copy(file.filePath(), udest_file)) {
    alog << "[add][file][" << position << "]\n";
    if constexpr (debug_mode) {
      qDebug() << "[Success]";
    }
    return true;
  } else {  // Copy failed.
    // TODO process the error
    if constexpr (debug_mode) {
      qDebug() << "[Failed]";
    }
    return false;
  }
}

/* Copy the "newdir" into "tdir" and log "add" in alog, "delete" in dlog. */
bool doAddActionLog(QFileInfo* newdir, const QDir& tdir, const QDir& nroot,
                    QTextStream& alog, QTextStream& dlog) noexcept {
  QDir dir(newdir->filePath());
  copyDir(dir, tdir);
  QString position = nroot.relativeFilePath(dir.absolutePath());
  alog << "[add][dir][" << position << "]\n";
  dlog << "[delete][dir][" << position << "]\n";

  if constexpr (debug_mode) {
    qDebug() << "[DoAddActionLog][Directory]";
    qDebug() << "[Source: " + newdir->absolutePath() + "]";
    qDebug() << "[Dest: " + tdir.absolutePath() + "]";
    qDebug() << "[Postion: " + position + "]";
  }
  return true;
}

/* Apply the bsdiff algorithm to generate delta file and log. */
bool doChangeActionLog(QByteArray& buffer_old, QByteArray& buffer_new,
                       QDir& dest, const QString& name_delta,
                       const QString& pos, QTextStream& ulog) noexcept {
  // Generate the delta file to "dest" and log it in ulog.
  // Info in the log : [delta][filepath]
  QFile delta(dest.filePath(name_delta));
  if (delta.open(QFile::WriteOnly | QFile::Truncate)) {
    bsdiff_stream stream = {&delta, malloc, free, &plainWrite};

    if (buffer_old.isEmpty()) {
      // Error occur
      // TODO process
    }
    if (buffer_new.isEmpty()) {
      // Error occur
      // TODO process
    }

    //
    bool success = bsdiff(reinterpret_cast<const uint8_t*>(buffer_old.data()),
                          buffer_old.size(),
                          reinterpret_cast<const uint8_t*>(buffer_new.data()),
                          buffer_new.size(), &stream) == 0;

    delta.close();
    if (success) {
      ulog << "[delta][file][" << pos << "]\n";
      return true;
    } else {
      ulog << "[error][file][" << pos << "][bsdiff-error]\n";
      return false;
    }
  } else {
    ulog << "[error][file][" << pos << "][open-error]\n";
    return false;
  }
}

bool generateDeltaFile(QFile* oldfile, QFile* newfile, QDir& udest, QDir& rdest,
                       const QDir& oroot, const QDir& nroot, QTextStream& ulog,
                       QTextStream& rlog) noexcept {
  if (!oldfile && !newfile) {
    return true;
  } else if (oldfile && !newfile) {
    // Only oldfile exists, which will be removed in the new version.
    // Info in the update log   : [delete][filepath]
    // Info in the rollback log : [add][filepath]
    return doAddActionLog(oldfile, rdest, oroot, rlog, ulog);
  } else if (!oldfile && newfile) {
    // Only newfile exists, which will be added in the new version.
    // Info in the update log   : [add][filepath]
    // Info in the rollback log : [delete][filepath]
    return doAddActionLog(newfile, udest, nroot, ulog, rlog);
  } else {
    // File exists in both versions.
    // Read both file and call to generate delta
    QByteArray old_buffer = oldfile->readAll();
    QByteArray new_buffer = newfile->readAll();
    QFileInfo ofile(*oldfile);
    QFileInfo nfile(*newfile);
    QString opos = oroot.relativeFilePath(ofile.filePath());
    QString upos = nroot.relativeFilePath(nfile.filePath());
    bool update = doChangeActionLog(old_buffer, new_buffer, udest,
                                    ofile.fileName() + ".r", upos, ulog);
    bool rollback = doChangeActionLog(new_buffer, old_buffer, rdest,
                                      nfile.fileName() + ".r", opos, rlog);
    return (update && rollback);
  }
}

bool generateDeltaDir(QFileInfo* olddir, QFileInfo* newdir, QDir& udest,
                      QDir& rdest, const QDir& oroot, const QDir& nroot,
                      QTextStream& ulog, QTextStream& rlog) {
  if constexpr (debug_mode) {
    QString odir_info = !olddir ? "null" : olddir->filePath();
    QString ndir_info = !newdir ? "null" : newdir->filePath();

    qDebug() << "[GenerateDeltaDir]";
    qDebug() << "[Old][" + odir_info + "]";
    qDebug() << "[New][" + ndir_info + "]";
    qDebug() << "[UpdatePackDest][" + udest.absolutePath() + "]";
    qDebug() << "[RollbackPackDest][" + rdest.absolutePath() + "]";
  }

  // relativeFilePath
  if (!olddir && !newdir)
    return true;
  else if (olddir && !newdir) {
    // Only the old directory exists, which will be removed in the new version.
    return doAddActionLog(olddir, rdest, oroot, rlog, ulog);
  } else if (!olddir && newdir) {
    // Only the new directory exists, which will be added in the new version.
    return doAddActionLog(newdir, udest, nroot, ulog, rlog);
  } else if (olddir && newdir) {
    // Both dir exist. Compare two dir and generate delta file.
    // Insert the whole list_new into a map.
    // Traversal list_old and try finding new version info in the map, and
    // generate the delta in the pack, then remove the iter in the map.
    // Process the remaining files/dirs in the map and in the list_old.
    /*---------------Dirs process---------------------*/
    QDir dir_old(olddir->filePath());
    QDir dir_new(newdir->filePath());
    dir_old.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    dir_new.setFilter(QDir::Dirs | QDir::NoDotAndDotDot);
    QFileInfoList list_old = dir_old.entryInfoList();
    QFileInfoList list_new = dir_new.entryInfoList();
    QMap<QString, QFileInfo> ndirs;
    for (auto& info : list_new) ndirs.insert(info.fileName(), info);

    // Traversal list_old
    for (auto& oinfo : list_old) {
      auto ninfo = ndirs.find(oinfo.fileName());
      if (ninfo != ndirs.end()) {  // Dir in new version found.
        // Get subpath title.
        QString sp = ninfo.key();
        if (!udest.exists(sp)) udest.mkdir(sp);
        if (!rdest.exists(sp)) rdest.mkdir(sp);
        QDir usp(udest);
        QDir rsp(rdest);
        usp.cd(sp);
        rsp.cd(sp);
        generateDeltaDir(&oinfo, &(*ninfo), usp, rsp, oroot, nroot, ulog, rlog);
        ndirs.erase(ninfo);
        continue;
      } else {  // Dir not found, which means dir only exists in old version.
        QString sp = oinfo.fileName();
        if (!rdest.exists(sp)) rdest.mkdir(sp);
        QDir rsp(rdest);
        rsp.cd(sp);
        // udest and nroot is not used.
        generateDeltaDir(&oinfo, nullptr, udest, rsp, oroot, nroot, ulog, rlog);
        continue;
      }
    }
    // Process the remaining dir in the ndirs.
    for (auto& ninfo : ndirs) {
      QString sp = ninfo.fileName();
      if (!udest.exists(sp)) udest.mkdir(sp);
      QDir usp(udest);
      usp.cd(sp);
      // rdest and oroot is not used.
      generateDeltaDir(nullptr, &ninfo, usp, rdest, oroot, nroot, ulog, rlog);
    }
    /*---------------Files process---------------------*/
    dir_old.setFilter(QDir::Files);
    dir_new.setFilter(QDir::Files);
    list_old = dir_old.entryInfoList();
    list_new = dir_new.entryInfoList();

    QMap<QString, QFileInfo> map;
    for (auto& info : list_new) map.insert(info.fileName(), info);

    // Traversal list_old.
    for (auto& oinfo : list_old) {
      auto ninfo = map.find(oinfo.fileName());
      if (ninfo != map.end()) {
        // File in new version found.
        QFile oldfile(oinfo.absoluteFilePath());
        QFile newfile(ninfo->absoluteFilePath());
        if (oldfile.open(QFile::Truncate | QFile::ReadOnly) &&
            newfile.open(QFile::Truncate | QFile::ReadOnly)) {
          generateDeltaFile(&oldfile, &newfile, udest, rdest, oroot, nroot,
                            ulog, rlog);
          newfile.close();
          oldfile.close();
          map.erase(ninfo);
          continue;
        } else {  // Open failed.
          // TODO error process.
          continue;
        }
      } else {  // File in new version not found.
        QFile oldfile(oinfo.absoluteFilePath());
        if (oldfile.open(QFile::Truncate | QFile::ReadOnly)) {
          generateDeltaFile(&oldfile, nullptr, udest, rdest, oroot, nroot, ulog,
                            rlog);
          oldfile.close();
          continue;
        } else {  // Open failed.
                  // TODO error process.
          continue;
        }
      }
    }

    // Process the remaining new files.
    for (auto& ninfo : map) {
      QFile newfile(ninfo.absoluteFilePath());
      if (newfile.open(QFile::Truncate | QFile::ReadOnly)) {
        generateDeltaFile(nullptr, &newfile, udest, rdest, oroot, nroot, ulog,
                          rlog);
        newfile.close();
        continue;
      } else {  // Open failed.
        // TODO error process.
        continue;
      }
    }
  }
}

}  // namespace

bool BsDiff::generate(QDir& dir_old, QDir& dir_new, QDir& dest_rpack,
                      QDir& dest_upack) noexcept {
  //
  QFile ulogf(dest_upack.absoluteFilePath("upgrade_log"));
  QFile rlogf(dest_rpack.absoluteFilePath("rollback_log"));
  if (ulogf.open(QFile::WriteOnly | QFile::Truncate) &&
      rlogf.open(QFile::WriteOnly | QFile::Truncate)) {
    QFileInfo olddir(dir_old.absolutePath());
    QFileInfo newdir(dir_new.absolutePath());
    QTextStream ulog(&ulogf);
    QTextStream rlog(&rlogf);
    generateDeltaDir(&olddir, &newdir, dest_upack, dest_rpack, dir_old, dir_new,
                     ulog, rlog);
  } else {
    return false;
  }
  return true;
}

}  // namespace ota::diff
