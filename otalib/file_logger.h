#ifndef FILE_LOGGER_H
#define FILE_LOGGER_H

#include <QDir>

namespace otalib {
struct file_logger {
  explicit file_logger(const QString &log_file_name) : file_(log_file_name) {
    file_.open(QIODevice::ReadWrite);
  }

  void close() { file_.close(); }

  void append_dir(const QString &dir_name) {
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
        append_dir(name);
      } else if (file.isFile()) {
        file_.write(name.toLocal8Bit());
        file_.write("\r\n");
        file_.flush();
      }
    }
  }

  void append(const QString &file) {
    file_.write(file.toLocal8Bit());
    file_.write("\r\n");
    file_.flush();
  }

 private:
  QFile file_;
};

}  // namespace otalib

#endif  // FILE_LOGGER_H
