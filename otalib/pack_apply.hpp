#ifndef PACK_APPLY_HPP
#define PACK_APPLY_HPP

#include <QDir>
#include <QFileInfo>
#include <QString>
#include <QTextStream>
#include <vector>

#include "diff.h"
#include "file_logger.h"
#include "otaerr.hpp"
#include "property.hpp"
#include "shell_cmd.hpp"
#include "signature.h"

namespace otalib {
using namespace ::otalib::bs;
namespace {
bool lverify(const QString& file, const QFileInfo& pubkey,
             const QFileInfo& sig) {
  QFile target(file);
  if (!target.open(QFile::ReadOnly)) {
    OTAError::S_file_open_fail xerr{::std::move(file), STRING_SOURCE_LOCATION};
    throw OTAError{::std::move(xerr)};
  }

  QCryptographicHash hash(kHashAlgorithm);
  hash.addData(&target);
  QByteArray hashval = hash.result();
  target.close();

  QFile tmphash("tmphash");
  if (!tmphash.open(QFile::WriteOnly | QFile::Truncate)) {
    OTAError::S_file_open_fail xerr{tmphash.fileName(), STRING_SOURCE_LOCATION};
    throw OTAError{::std::move(xerr)};
  }

  if (tmphash.write(hashval) != hashval.size()) {
    tmphash.close();
    OTAError::S_file_write_fail xerr{tmphash.fileName(),
                                     STRING_SOURCE_LOCATION};
    throw OTAError{::std::move(xerr)};
  };

  tmphash.close();
  // Verifiy.
  bool succ = verify(QFileInfo(tmphash.fileName()), sig, pubkey);

  if (!tmphash.remove()) {
    OTAError::S_file_delete_fail xerr{tmphash.fileName(),
                                      STRING_SOURCE_LOCATION};
    throw OTAError{::std::move(xerr)};
  }

  return succ;
}
}  // namespace
// desc: Generate the whole pack for patching.
// param:
//      dest_dir: Destination directory.
// param:
//      paths: The paths generate by VersionMap::search().
// param:
//      callback: How to find the target pack and the file containing hash value
//      of the new version.
// callback ret:
//      First : The pack's filename
//      Second: The app's hash file.
//      Third : The pack's signature.
template <typename VersionType, typename CallbackOnFind,
          typename EdgeType = ::std::pair<VersionType, VersionType>>
void getPackFromPaths(const QDir& dest_dir,
                      const ::std::vector<EdgeType>& paths,
                      CallbackOnFind&& callback) {
  using CallbackType = ::std::tuple<QFileInfo, QFileInfo, QFileInfo>(
      const VersionType&, const VersionType&);
  static_assert(::std::is_same_v<CallbackOnFind, CallbackType>,
                "Callback function type dismatched.");
  //
  QString lpath = dest_dir.filePath(kApplyLogName);
  QFile log(lpath);
  if (!log.open(QFile::WriteOnly | QFile::Truncate)) {
    OTAError::S_file_open_fail xerror{::std::move(lpath),
                                      STRING_SOURCE_LOCATION};
    throw OTAError{::std::move(xerror)};
  }
  // Log records the order of applying delta pack,
  // and the hash info.
  QTextStream stream(&log);
  for (auto& p : paths) {
    //
    const auto& [prev, next] = p;
    const auto [file, hash, sig] = callback(prev, next);
    // Copy the pack into "dest_dir", and then generate the current info.
    QString targetf = file.absoluteFilePath();
    QString targeth = hash.absoluteFilePath();
    QString targets = sig.absoluteFilePath();
    QString destf = dest_dir.filePath(file.fileName());
    QString desth = dest_dir.filePath(hash.fileName());
    QString dests = dest_dir.filePath(sig.fileName());

    // copy the pack into destination.
    if (!QFile::copy(targetf, destf)) {
      log.close();
      OTAError::S_file_copy_fail xerror{
          ::std::move(targetf), ::std::move(destf), STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }

    // copy the hash info into destination.
    if (!QFile::copy(targeth, desth)) {
      log.close();
      OTAError::S_file_copy_fail xerror{
          ::std::move(targeth), ::std::move(desth), STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }

    // copy the signature into destination.
    if (!QFile::copy(targets, dests)) {
      log.close();
      OTAError::S_file_copy_fail xerror{
          ::std::move(targets), ::std::move(dests), STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }

    stream << file.fileName() << "|" << hash.fileName() << "|" << sig.fileName()
           << "\n";
  }
  log.close();
}

// desc: After the uncompress, all the pack stores in "pack_root". This func is
// to apply all the packs on app.

// safe mode: Under saft mode, there's hash check for app after every step.
template <typename VersionType,
          typename EdgeType = ::std::pair<VersionType, VersionType>>
void applyPackOnApp(const QDir& app_root, const QDir& pack_root,
                    const QFileInfo& pubkey, bool safe_mode = true) {
  // Apply sequence is decicded according to apply_log.
  QString log_path = pack_root.filePath(kApplyLogName);
  QFile log_file(log_path);
  if (!log_file.open(QFile::ReadOnly)) {
    OTAError::S_file_open_fail xerror{::std::move(log_path),
                                      STRING_SOURCE_LOCATION};
    throw OTAError{::std::move(xerror)};
  }

  QTextStream log(&log_file);
  QString line;
  QStringList info;
  while (log.readLineInto(&line)) {
    // remake the temp directory.
    pack_root.mkdir("TmPdIc");
    QDir tmp_root(pack_root);
    tmp_root.cd("TmPdIc");

    // clear the list.
    info.clear();

    // Info: packname|hashname|signame
    info = line.split("|");
    if (info.size() != 3) {
      log_file.close();
      tmp_root.removeRecursively();
      OTAError::S_general xerr{"Invalid apply_log info." +
                               STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerr)};
    }

    // Verify the signature of current pack. Then uncompress and apply.
    QString packname = info.at(0);
    QString packfile = pack_root.filePath(packname);
    QString signame = info.at(2);
    QFileInfo signature{pack_root.filePath(signame)};

    // Verify
    bool succ = lverify(packfile, pubkey, signature);
    if (!succ) {
      log_file.close();
      tmp_root.removeRecursively();
      packfile.prepend("[");
      packfile.append("]");
      packfile.append("current pack verify fails.");
      OTAError::S_verify_fail xerr{::std::move(packfile),
                                   STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerr)};
    }

    // Verify succeed.
    tar_extract_archive_file_gzip(packfile, tmp_root.absolutePath());
    succ = applyDeltaPack(tmp_root, app_root);
    if (!succ) {
      log_file.close();
      tmp_root.removeRecursively();
      OTAError::S_apply_pack_unexpected_fail xerr{::std::move(packname),
                                                  STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerr)};
    }

    if (!safe_mode) {
      tmp_root.removeRecursively();
      continue;
    }

    Property pp = ReadProperty();

    // Do hash check.
    QString hash_filename = pack_root.filePath(info.at(1));
    QFile hashfile(hash_filename);
    if (!hashfile.open(QFile::ReadOnly)) {
      log_file.close();
      tmp_root.removeRecursively();
      OTAError::S_file_open_fail xerror{::std::move(log_path),
                                        STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }
    ::std::string base_value = hashfile.readAll().toStdString();
    hashfile.close();
    ::std::string app_value =
        FileLogger::GetHashFromLogFile(kFileLogPath).to_string();
    if (app_value != base_value) {
      log_file.close();
      tmp_root.removeRecursively();
      OTAError::S_hash_check_fail xerror{::std::move(pp.app_version_),
                                         STRING_SOURCE_LOCATION};
      throw OTAError{::std::move(xerror)};
    }

    // remove the previsou content.
    tmp_root.removeRecursively();
  }
}

}  // namespace otalib

#endif  // PACK_APPLY_HPP
