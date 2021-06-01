#ifndef APP_PROPERTIES_HPP
#define APP_PROPERTIES_HPP

#include <QJsonDocument>
#include <QJsonObject>

#include "../otalib/otaerr.hpp"
#include "../otalib/property.hpp"
#include "../otalib/update_strategy.hpp"
#include "../otalib/version.hpp"

namespace otalib::app {
using ::otalib::GeneralVersion;
// update module
static inline const QString kUpdateModule = "./update";
// app infos
using AppVersionType = GeneralVersion;
// network
static inline const QString kIpAddr = "127.0.0.1";
static inline const quint16 kPort = 5555;
static inline const quint32 kTimeOut = 30;  // second
// temporary directory
static inline const QString kOtaTmpDir = "/tmp/ota_demo/";
static inline const QString kOtaTmpFile = kOtaTmpDir + "tmpfile.tar.gz";
// public key file for verifying signature.
static inline const QString kPubkeyFile = "pubkey";

}  // namespace otalib::app

#endif  // APP_PROPERTIES_HPP
