#include "diff.h"
namespace otalib::bs {
namespace {

constexpr bool bs_debug_mode = false;

static int plainWrite(bsdiff_stream* stream, const void* buffer, int size) {
  // Just write into the file.
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

#ifdef __linux__
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
                    QTextStream& alog, QTextStream& dlog) {
  QFileInfo file(*newfile);
  QString udest_file = tdir.filePath(file.fileName());
  QString position = nroot.relativeFilePath(file.absoluteFilePath());
  // Try to remove the previous files.
  if (QFile::exists(udest_file) && !QFile::remove(udest_file))
    print<GeneralWarnCtrl>(std::cerr, "Unable to remove the existing file.");
  writeDeltaLog(dlog, {Action::DELETEACT, Category::FILE, position, QString()});
  if constexpr (bs_debug_mode) {
    print<GeneralDebugCtrl>(std::cout, "[DoAddActionLog][File]");
    print<GeneralDebugCtrl>(std::cout,
                            "[Source: " + file.absoluteFilePath() + "]");
    print<GeneralDebugCtrl>(std::cout, "[Dest: " + tdir.absolutePath() + "]");
    print<GeneralDebugCtrl>(std::cout, "[Position: " + position + "]");
  }
  QString target = file.filePath();
  if (QFile::copy(target, udest_file)) {
    writeDeltaLog(alog, {Action::ADD, Category::FILE, position, QString()});
    print<GeneralSuccessCtrl>(std::cout, "Copy succeed.");
    return true;
  } else {  // Copy failed.
    OTAError::S_file_copy_fail xerror{
        ::std::move(target), ::std::move(udest_file), STRING_SOURCE_LOCATION};
    throw OTAError{::std::move(xerror)};
  }
}

/* Copy the "newdir" into "tdir" and log "add" in alog, "delete" in dlog. */
bool doAddActionLog(QFileInfo* newdir, const QDir& tdir, const QDir& nroot,
                    QTextStream& alog, QTextStream& dlog) {
  QDir dir(newdir->filePath());
  copyDir(dir, tdir);
  QString position = nroot.relativeFilePath(dir.absolutePath());
  try {
    writeDeltaLog(alog, {Action::ADD, Category::DIR, position, QString()});
    writeDeltaLog(dlog,
                  {Action::DELETEACT, Category::DIR, position, QString()});
  } catch (::std::exception& e) {
    print<GeneralFerrorCtrl>(::std::cerr, e.what());
    OTAError::S_general xerror{QStringLiteral("Do \"Add\" action fails.")};
    throw xerror;
  }

  if constexpr (bs_debug_mode) {
    print<GeneralDebugCtrl>(std::cout, "[DoAddActionLog][Directory]");
    print<GeneralDebugCtrl>(std::cout,
                            "[Source: " + newdir->absolutePath() + "]");
    print<GeneralDebugCtrl>(std::cout, "[Dest: " + tdir.absolutePath() + "]");
    print<GeneralDebugCtrl>(std::cout, "[Postion: " + position + "]");
  }
  return true;
}

/* Apply the bsdiff algorithm to generate delta file and log. */
bool doChangeActionLog(QByteArray& buffer_old, QByteArray& buffer_new,
                       QDir& dest, const QString& name_delta,
                       const QString& pos, QTextStream& ulog) {
  // Generate the delta file to "dest" and log it in ulog.
  // Info in the log : [delta][filepath]
  QFile delta(dest.filePath(name_delta));
  if (delta.open(QFile::WriteOnly | QFile::Truncate)) {
    bsdiff_stream stream = {&delta, malloc, free, &plainWrite};

    if (buffer_old.isEmpty()) {
      OTAError::S_delta_file_generate_fail xerror{
          pos,
          QStringLiteral(
              "Doing change action fails.Cannot read old file into buffer.") +
              STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }
    if (buffer_new.isEmpty()) {
      OTAError::S_delta_file_generate_fail xerror{
          pos,
          QStringLiteral(
              "Doing change action fails.Cannot read new file into buffer.") +
              STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }

    //
    bool success = bsdiff(reinterpret_cast<const uint8_t*>(buffer_old.data()),
                          buffer_old.size(),
                          reinterpret_cast<const uint8_t*>(buffer_new.data()),
                          buffer_new.size(), &stream) == 0;
    delta.close();
    if (success) {
      // Additional info stores in opaque.
      // opaque ::= _1/
      // _1 : The size of new file.
      ::std::string opaque = ::std::to_string(buffer_new.size());
      writeDeltaLog(ulog, {Action::DELTA, Category::FILE, pos,
                           QString::fromStdString(opaque)});

      return true;
    } else {  // bsdiff failed.
      OTAError::S_delta_file_generate_fail xerror{
          pos, QStringLiteral(
                   "Doing change action fails.Function \"bsdiff()\" failed.") +
                   STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }
  } else {
    OTAError::S_delta_file_generate_fail xerror{
        pos, QStringLiteral(
                 "Doing change action fails.Cannot create delta patch file.") +
                 STRING_SOURCE_LOCATION};
    throw OTAError{::std::move(xerror)};
  }
}

bool generateDeltaFile(QFile* oldfile, QFile* newfile, QDir& udest, QDir& rdest,
                       const QDir& oroot, const QDir& nroot, QTextStream& ulog,
                       QTextStream& rlog) {
  try {
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
  } catch (::std::exception& e) {
    //
    print<GeneralFerrorCtrl>(::std::cerr, e.what());
    OTAError::S_general xerror{QStringLiteral("") + STRING_SOURCE_LOCATION};
  }
}

bool generateDeltaDir(QFileInfo* olddir, QFileInfo* newdir, QDir& udest,
                      QDir& rdest, const QDir& oroot, const QDir& nroot,
                      QTextStream& ulog, QTextStream& rlog) {
  if constexpr (bs_debug_mode) {
    QString odir_info = !olddir ? "null" : olddir->filePath();
    QString ndir_info = !newdir ? "null" : newdir->filePath();

    print<GeneralDebugCtrl>(std::cout, "[GenerateDeltaDir]");
    print<GeneralDebugCtrl>(std::cout, "[Old][" + odir_info + "]");
    print<GeneralDebugCtrl>(std::cout, "[New][" + ndir_info + "]");
    print<GeneralDebugCtrl>(std::cout,
                            "[UpdatePackDest][" + udest.absolutePath() + "]");
    print<GeneralDebugCtrl>(std::cout,
                            "[RollbackPackDest][" + rdest.absolutePath() + "]");
  }
  try {
    // relativeFilePath
    if (!olddir && !newdir)
      return true;
    else if (olddir && !newdir) {
      // Only the old directory exists, which will be removed in the new
      // version.
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
          generateDeltaDir(&oinfo, &(*ninfo), usp, rsp, oroot, nroot, ulog,
                           rlog);
          ndirs.erase(ninfo);
          continue;
        } else {  // Dir not found, which means dir only exists in old version.
          QString sp = oinfo.fileName();
          if (!rdest.exists(sp)) rdest.mkdir(sp);
          QDir rsp(rdest);
          rsp.cd(sp);
          // udest and nroot is not used.
          generateDeltaDir(&oinfo, nullptr, udest, rsp, oroot, nroot, ulog,
                           rlog);
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
          QString po = oinfo.absoluteFilePath();
          QString pn = ninfo->absoluteFilePath();
          QFile oldfile(po);
          QFile newfile(pn);
          if (oldfile.open(QFile::Truncate | QFile::ReadOnly) &&
              newfile.open(QFile::Truncate | QFile::ReadOnly)) {
            try {
              generateDeltaFile(&oldfile, &newfile, udest, rdest, oroot, nroot,
                                ulog, rlog);
            } catch (::std::exception& e) {
              newfile.close();
              oldfile.close();
              throw e;
            }
            newfile.close();
            oldfile.close();
            map.erase(ninfo);
            continue;
          } else {  // Open failed.
            if (!oldfile.isOpen()) {
              OTAError::S_file_open_fail xerror{::std::move(po),
                                                STRING_SOURCE_LOCATION};
              throw OTAError{::std::move(xerror)};
            }
            if (!newfile.isOpen()) {
              OTAError::S_file_open_fail xerror{::std::move(pn),
                                                STRING_SOURCE_LOCATION};
              throw OTAError{::std::move(xerror)};
            }
          }
        } else {  // File in new version not found.
          QString p = oinfo.absoluteFilePath();
          QFile oldfile(p);
          if (oldfile.open(QFile::Truncate | QFile::ReadOnly)) {
            try {
              generateDeltaFile(&oldfile, nullptr, udest, rdest, oroot, nroot,
                                ulog, rlog);
            } catch (::std::exception& e) {
              oldfile.close();
              throw e;
            }
            oldfile.close();
            continue;
          } else {  // Open failed.
            if (!oldfile.isOpen()) {
              OTAError::S_file_open_fail xerror{::std::move(p),
                                                STRING_SOURCE_LOCATION};
              throw OTAError{::std::move(xerror)};
            }
          }
        }
      }

      // Process the remaining new files.
      for (auto& ninfo : map) {
        QString p = ninfo.absoluteFilePath();
        QFile newfile(p);
        if (newfile.open(QFile::Truncate | QFile::ReadOnly)) {
          try {
            generateDeltaFile(nullptr, &newfile, udest, rdest, oroot, nroot,
                              ulog, rlog);
          } catch (::std::exception& e) {
            newfile.close();
            throw e;
          }

          newfile.close();
          continue;
        } else {  // Open failed.
          if (!newfile.isOpen()) {
            OTAError::S_file_open_fail xerror{::std::move(p),
                                              STRING_SOURCE_LOCATION};
            throw OTAError{::std::move(xerror)};
          }
        }
      }
    }

    return true;
  } catch (::std::exception& e) {
    print<GeneralFerrorCtrl>(::std::cerr, e.what());
    OTAError::S_general xerror{QStringLiteral("Generate Dirctory failed.") +
                               STRING_SOURCE_LOCATION};
    throw OTAError{::std::move(xerror)};
  }
}

}  // namespace

bool generateDeltaPack(QDir& dir_old, QDir& dir_new, QDir& dest_rpack,
                       QDir& dest_upack) {
  if (!dir_old.exists() || !dir_new.exists()) return false;

  if (!dest_rpack.exists() && !dest_rpack.mkpath(dest_rpack.absolutePath())) {
    print<GeneralFerrorCtrl>(
        ::std::cerr,
        QStringLiteral(
            "Can't find destination directory path for rollback pack.") +
            STRING_SOURCE_LOCATION);
    return false;
  }
  if (!dest_upack.exists() && !dest_upack.mkpath(dest_upack.absolutePath())) {
    print<GeneralFerrorCtrl>(
        ::std::cerr,
        QStringLiteral(
            "Can't find destination directory path for update pack.") +
            STRING_SOURCE_LOCATION);
    return false;
  }
  //
  QFile ulogf(dest_upack.absoluteFilePath("update_log"));
  QFile rlogf(dest_rpack.absoluteFilePath("rollback_log"));
  bool success = false;
  if (ulogf.open(QFile::WriteOnly | QFile::Truncate) &&
      rlogf.open(QFile::WriteOnly | QFile::Truncate)) {
    QFileInfo olddir(dir_old.absolutePath());
    QFileInfo newdir(dir_new.absolutePath());
    QTextStream ulog(&ulogf);
    QTextStream rlog(&rlogf);
    try {
      success = generateDeltaDir(&olddir, &newdir, dest_upack, dest_rpack,
                                 dir_old, dir_new, ulog, rlog);
    } catch (::std::exception& e) {
      ulogf.close();
      rlogf.close();
      print<GeneralFerrorCtrl>(::std::cerr, e.what());
      return false;
    }
  } else {
    print<GeneralFerrorCtrl>(
        ::std::cerr,
        QStringLiteral("Generation didn't happen because of the failure "
                       "to open or create log files.") +
            STRING_SOURCE_LOCATION);
    return false;
  }

  ulogf.close();
  rlogf.close();
  return success;
}

namespace {

static int plainRead(const bspatch_stream* stream, void* buffer, int length) {
  // Read from the patch file.
  if (static_cast<QFile*>(stream->opaque)
          ->read(static_cast<char*>(buffer), length) == length)
    return 0;
  else
    return -1;
}

bool doAdd(const DeltaInfo& info, const QDir& pack, const QDir& root) {
  QString spath = pack.absoluteFilePath(info.position);
  QString dpath = root.absoluteFilePath(info.position);
  switch (info.category) {
    case Category::DIR: {
      copyDir(spath, dpath + "\\");
      if (QDir(spath).exists() && QDir(dpath).exists()) return true;
      OTAError::S_general xerror{
          QStringLiteral("Add action failed. Directory copy failed.") +
          STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }
    case Category::FILE: {
      if (QFile::copy(spath, dpath)) return true;
      OTAError::S_general xerror{
          QStringLiteral("Add action failed. File copy failed.") +
          STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }
    default: {
      OTAError::S_general xerror{
          QStringLiteral("Add action failed. Invalid info read.") +
          STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }
  }
  return false;
}

bool doDelete(const DeltaInfo& info, const QDir& pack, const QDir& root) {
  Q_UNUSED(pack);
  QString dpath = root.absoluteFilePath(info.position);
  switch (info.category) {
    case Category::DIR: {
      if (QDir target(dpath); target.exists() && target.removeRecursively())
        return true;

      OTAError::S_general xerror{
          QStringLiteral("Delete action failed. Directory delete failed.") +
          STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }
    case Category::FILE: {
      if (QFile(dpath).exists() && QFile::remove(dpath)) return true;
      OTAError::S_general xerror{
          QStringLiteral("Delete action failed. File delete failed.[") + dpath +
          "]" + STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }
    default: {
      OTAError::S_general xerror{
          QStringLiteral("Delete action failed. Invalid info read.") +
          STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }
  }
  return false;
}

bool doDelta(const DeltaInfo& info, const QDir& pack, const QDir& root) {
  QString patch_path = pack.absoluteFilePath(info.position + ".r");
  QString source_path = root.absoluteFilePath(info.position);
  switch (info.category) {
    case Category::FILE: {
      QFile patch(patch_path);
      QFile target(source_path);
      if (patch.open(QFile::ReadOnly) && target.open(QFile::ReadOnly)) {
        QByteArray buffer_target = target.readAll();
        QStringList slist = info.opaque.split("/", Qt::SkipEmptyParts);
        if (slist.isEmpty()) {
          patch.close();
          target.close();
          OTAError::S_general xerror{
              QStringLiteral(
                  "Applying delta patching Failed. Info is corrupted.") +
              STRING_SOURCE_LOCATION};
          throw OTAError{::std::move(xerror)};
        }

        int filesize = slist.at(0).toUInt();
        if (filesize == 0) {
          patch.close();
          target.close();
          OTAError::S_general xerror{
              QStringLiteral("Applying delta patch failed. Cannot "
                             "allocate valid memory for new file.") +
              STRING_SOURCE_LOCATION};
          throw OTAError{::std::move(xerror)};
        }
        QByteArray buffer_result(filesize, '\0');
        bspatch_stream stream{&patch, &plainRead};
        if (bspatch(reinterpret_cast<uint8_t*>(buffer_target.data()),
                    buffer_target.size(),
                    reinterpret_cast<uint8_t*>(buffer_result.data()),
                    buffer_result.size(), &stream) == 0) {
          // Patch succeed.
          target.close();
          patch.close();

          if (!target.open(QFile::WriteOnly | QFile::Truncate)) {
            OTAError::S_general xerror{
                QStringLiteral("Applying delta patch failed. Cannot write "
                               "date into target file.") +
                STRING_SOURCE_LOCATION};
            throw OTAError{::std::move(xerror)};
          }
          if (target.write(buffer_result) != buffer_result.size()) {
            target.close();
            OTAError::S_general xerror{
                QStringLiteral(
                    "Applying delta patch failed. Unexpected error occurs when "
                    "writing to target file.") +
                STRING_SOURCE_LOCATION};
            throw OTAError{::std::move(xerror)};
          }
          target.close();
          return true;
        } else {  // Patch failed
          patch.close();
          target.close();
          OTAError::S_general xerror{
              QStringLiteral("Function \"bspatch()\" failed, cannot apply "
                             "delta patch to target file.") +
              STRING_SOURCE_LOCATION};
          throw OTAError{::std::move(xerror)};
        }
      } else {  // Open failed.
        if (!patch.isOpen()) {
          OTAError::S_file_open_fail xerror{
              ::std::move(patch_path),
              QStringLiteral("Applying delta patch failed.") +
                  STRING_SOURCE_LOCATION};
          throw OTAError{::std::move(xerror)};
        }
        if (!target.isOpen()) {
          OTAError::S_file_open_fail xerror{
              ::std::move(source_path),
              QStringLiteral("Applying delta patch failed.") +
                  STRING_SOURCE_LOCATION};
          throw OTAError{::std::move(xerror)};
        }
      }
      break;
    }
      // Impossible
    case Category::DIR:
    default:
      OTAError::S_general xerror{
          QStringLiteral("Applying delta patch failed. Invalid info read.") +
          STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
  }

  return false;
}

bool doApply(const QDir& pack, const QDir& target, QTextStream& log,
             QTextStream& dlog) {
  // Read the info from the dlog
  DeltaInfoStream dstrm = readDeltaLog(dlog);
  auto hasDone = [&dstrm](const DeltaInfo& action) {
    for (const auto& info : dstrm)
      if (info == action) return true;
    return false;
  };
  DeltaInfoStream stream = readDeltaLog(log);

  while (!stream.isEmpty()) {
    // Read from the back.
    bool success = false;
    const auto& info = stream.back();
    if (hasDone(info)) {
      // Skip the action if it had already done.
      stream.pop_back();
      continue;
    }

    try {
      switch (info.action) {
        case Action::ADD:
          success = doAdd(info, pack, target);
          break;
        case Action::DELETEACT:
          success = doDelete(info, pack, target);
          break;
        case Action::DELTA:
          success = doDelta(info, pack, target);
          break;
        default: {
          OTAError::S_general xerror{QStringLiteral("Invalid info read.") +
                                     STRING_SOURCE_LOCATION};
          throw OTAError{::std::move(xerror)};
        }
      }
    } catch (::std::exception& e) {
      print<GeneralFerrorCtrl>(::std::cerr, e.what());
      OTAError::S_general xerror{QStringLiteral("Applying patch failed.") +
                                 STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }

    // Record the successful action in dlog.
    if (success) {
      try {
        writeDeltaLog(dlog, info);
      } catch (::std::exception& e) {
        print<GeneralFerrorCtrl>(::std::cerr, e.what());
        OTAError::S_general xerror{QStringLiteral("Applying patch failed.") +
                                   STRING_SOURCE_LOCATION};
        throw OTAError{::std::move(xerror)};
      }
    } else {
      OTAError::S_general xerror{
          QStringLiteral("Applying patch failed. Error occurs during the "
                         "perform of action.") +
          STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }
    stream.pop_back();
  }
  return true;
}

}  // namespace

bool applyDeltaPack(QDir& pack, QDir& target) {
  if constexpr (bs_debug_mode)
    print<GeneralDebugCtrl>(std::cout, "[applyDeltaPack]");

  if (!pack.exists() || !target.exists()) {
    if (!pack.exists()) {
      print<GeneralFerrorCtrl>(
          ::std::cerr,
          QStringLiteral("[Invalid pack dir]") + STRING_SOURCE_LOCATION);
      return false;
    } else {
      print<GeneralFerrorCtrl>(
          ::std::cerr,
          QStringLiteral("[Invalid target dir]") + STRING_SOURCE_LOCATION);
      return false;
    }
  }

  // Generate a done-list.
  QFile dlogf(pack.absoluteFilePath("done_log"));

  // If pack is a update pack.
  QFile ulogf(pack.absoluteFilePath("update_log"));
  if (ulogf.open(QFile::ReadOnly) &&
      dlogf.open(QFile::ReadWrite | QFile::Append)) {
    if constexpr (bs_debug_mode) {
      print<GeneralDebugCtrl>(std::cout, "[Update]");
      print<GeneralDebugCtrl>(std::cout,
                              "[ Pack  : " + pack.absolutePath() + "]");
      print<GeneralDebugCtrl>(std::cout,
                              "[Target : " + target.absolutePath() + "]");
    }
    QTextStream ulog(&ulogf);
    QTextStream dlog(&dlogf);
    try {
      doApply(pack, target, ulog, dlog);
    } catch (::std::exception& e) {
      dlogf.close();
      ulogf.close();
      print<GeneralFerrorCtrl>(std::cerr, e.what());
      return false;
    }

    dlogf.close();
    ulogf.close();
    return true;
  }

  // If pack is a rollback pack.
  QFile rlogf(pack.absoluteFilePath("rollback_log"));
  if (rlogf.open(QFile::ReadOnly) &&
      dlogf.open(QFile::ReadWrite | QFile::Append)) {
    if constexpr (bs_debug_mode) {
      print<GeneralDebugCtrl>(std::cout, "[Rollback]");
      print<GeneralDebugCtrl>(std::cout,
                              "[ Pack  : " + pack.absolutePath() + "]");
      print<GeneralDebugCtrl>(std::cout,
                              "[Target : " + target.absolutePath() + "]");
    }
    QTextStream rlog(&rlogf);
    QTextStream dlog(&dlogf);
    try {
      doApply(pack, target, rlog, dlog);
    } catch (::std::exception& e) {
      dlogf.close();
      rlogf.close();
      print<GeneralFerrorCtrl>(::std::cerr, e.what());
      return false;
    }
    dlogf.close();
    rlogf.close();
    return true;
  }
  print<GeneralFerrorCtrl>(
      ::std::cerr,
      QStringLiteral("Patch didn't happen, bacause of the failure to open "
                     "log files.") +
          STRING_SOURCE_LOCATION);
  return false;
}

}  // namespace otalib::bs
