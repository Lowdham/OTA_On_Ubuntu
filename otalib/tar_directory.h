#ifndef TAR_DIRECTORY_H
#define TAR_DIRECTORY_H

#include <QString>
namespace otalib {

static void tar_create_archive_file_gzip(const QString& directory,
                                         const QString& archive_file) {
  QString cmd =
      QString("tar -zcf \"%1\" \"%2\"").arg(archive_file).arg(directory);
  ::system(cmd.toStdString().c_str());
}
static void tar_extract_archive_file_gzip(const QString& archive_file,
                                          const QString& directory) {
  QString cmd =
      QString("tar -zxf \"%1\" \"%2\"").arg(archive_file).arg(directory);
  ::system(cmd.toStdString().c_str());
}
}  // namespace otalib

#endif  // TAR_DIRECTORY_H
