#ifndef FILE_LOGGER_H
#define FILE_LOGGER_H

#include <QDir>

#include "sha256_hash.h"

namespace otalib {
struct FileLogger {
  explicit FileLogger(const QString &log_file) : file_(log_file) {
    file_.open(QIODevice::ReadWrite);
  }

  static merkle_hash_t GetHashFromLogFile(const QString &log_file,
                                          const QString &prefix = "") {
    QFile file(log_file);
    if (!file.open(QIODevice::ReadOnly)) return merkle_hash_t();
    char buf[1024]{0};
    merkle_tree_t tree;
    uint8_t md[kSha256Len]{0};
    QString path;

    while (-1 != file.readLine(buf, sizeof(buf))) {
      QString filename = QString(buf).trimmed();
      if (filename.isEmpty()) continue;
      filename = filename.remove(0, 2);  // remvoe "./"
      if (prefix.isEmpty())
        path = filename;
      else
        path = prefix + filename;
      Sha256HashFile(path.toStdString(), md);
      tree.insert(merkle_hash_t(md));
    }
    file.close();
    auto root = tree.root();
    return root;
  }

  void Close() { file_.close(); }

  void AppendDir(const QString &dir_name) {
    QDir dir(dir_name);
    if (!dir.exists()) return;

    dir.setFilter(QDir::Dirs | QDir::Files);
    dir.setSorting(QDir::DirsLast);

    auto list = dir.entryInfoList();
    for (auto &file : list) {
      if (file.fileName() == "." || file.fileName() == "..") continue;
      QString name =
          QDir::fromNativeSeparators(dir_name + "/" + file.fileName());
      if (file.isDir()) {
        AppendDir(name);
      } else if (file.isFile()) {
        file_.write(name.toLocal8Bit());
        file_.write("\r\n");
        file_.flush();
      }
    }
  }

  void Append(const QString &file) {
    file_.write(file.toLocal8Bit());
    file_.write("\r\n");
    file_.flush();
  }

 private:
  QFile file_;
};

}  // namespace otalib

#endif  // FILE_LOGGER_H
