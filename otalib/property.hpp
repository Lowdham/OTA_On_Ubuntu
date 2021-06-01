#ifndef OTALIB_PROPERTY_HPP
#define OTALIB_PROPERTY_HPP

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QString>
#include "otaerr.hpp"

namespace otalib {
//
static inline const QString kPropertyPath = "./property.json";
static inline const QString kFileLogPath = "./file_log";
static inline const QString kFileLogName = "file_log";
static inline const QString kApplyLogName = "apply_log";

struct Property {
  QString app_name_;
  QString app_type_;
  QString app_version_;
};

static Property ReadProperty(const QString& path = kPropertyPath) {
  // Read the property file.
  QFile fproperty(path);
  if (!fproperty.open(QFile::ReadOnly)) {
    OTAError::S_file_open_fail xerror{::std::move(path),
                                      STRING_SOURCE_LOCATION};
    throw OTAError{::std::move(xerror)};
  }

  // Parse the json.
  QJsonParseError jerr;
  QJsonDocument jdoc = QJsonDocument::fromJson(fproperty.readAll(), &jerr);
  fproperty.close();
  if (jerr.error != QJsonParseError::NoError || !jdoc.isObject()) {
    throw OTAError::S_general{
        QStringLiteral("Failed to parse property files.") +
        STRING_SOURCE_LOCATION};
  }
  QJsonObject jobj = jdoc.object();
  if (!jobj.contains("App Name") || !jobj.value("App Name").isString() ||
      !jobj.contains("App Type") || !jobj.value("App Type").isString() ||
      !jobj.contains("App Version") || !jobj.value("App Version").isString()) {
    throw OTAError::S_general{
        QStringLiteral("Necessary field in property file corrupts.") +
        STRING_SOURCE_LOCATION};
  }

  // Get the data.
  return Property{jobj.value("App Name").toString(),
                  jobj.value("App Type").toString(),
                  jobj.value("App Version").toString()};
}

}  // namespace otalib

#endif  // OTALIB_PROPERTY_HPP
