#include "server/include/server.h"

#include <sys/stat.h>

#include <QJsonDocument>

#include "server/include/FileLoader.hpp"
using namespace otaserver;

using std::placeholders::_1;

namespace {
constexpr static const char* kAllDeltaPackTmpDir = "./tmpAllDeltaPack/";
constexpr static const char* kCompletePackDir = "./CompletePack/";
constexpr static const char* kGenDeltaPackDir = "./DeltaPack/";
constexpr static const char* kDoneDeltaPackDir = "./DoneDeltaPack/";
constexpr static const char* kSigDir = "./Sigs/";
constexpr static const char* kHashDir = "./Hashs/";

constexpr static const char* kServerSSLKey = "./ssl/";
constexpr static const char* kServerSSLPriKey = "./ssl/private.pem";
constexpr static const char* kServerSSLPubCert = "./ssl/public.crt";

constexpr static const char* kSigKeyDir = "./key/";
constexpr static const char* kSigPriKeyFile = "./key/prikey";
constexpr static const char* kSigPubKeyFile = "./key/pubkey";

constexpr static const char* kIndxVerMapData = "./verMap.idb";

constexpr static const size_t kIdleTimeout = 2000;
constexpr static const size_t kServerPort = 5555;
}  // namespace

::std::tuple<QFileInfo, QFileInfo, QFileInfo> findPackCallback(
    const GeneralVersion& prev, const GeneralVersion& next);

bool findPackVCMCallback(const GeneralVersion&, const GeneralVersion&);

OTAServer::OTAServer(QObject* parent) : QObject(parent), indxFp_(nullptr) {
  server_ = new TcpServer;
  server_->set_new_conn_cb(std::bind(&OTAServer::newConnection, this, _1));
  server_->set_close_conn_cb(std::bind(&OTAServer::closeConnection, this, _1));
  server_->set_msg_cb(std::bind(&OTAServer::readMessage, this, _1));
  server_->set_idle_timeout(kIdleTimeout);

  // Initialize Environment
  initializeEnv();
}

OTAServer::~OTAServer() {
  if (server_) delete server_;
  server_ = nullptr;

  if (indxFp_) {
    ::fflush(indxFp_);
    ::fsync(::fileno(indxFp_));
    ::fclose(indxFp_);
    indxFp_ = nullptr;
  }
}

void OTAServer::initDirectoryListener() {
  watcher_.setDirChangedCb(std::bind(&OTAServer::directoryChanged, this, _1));
  watcher_.startWatch(kCompletePackDir);
}

void OTAServer::initializeEnv() {
  // server generate public.crt and private.pem
  // openssl genrsa -out private.pem 2048
  // openssl req -new -key private.pem -out server.csr -subj
  // "/C=CN/ST=myprovince/L=mycity/O=myorganization/OU=mygroup/CN=myServer

  // openssl x509 -req -days 99999 -in server.csr -signkey private.pem -out
  // public.crt
  mkDir(kServerSSLKey);
  server_->set_certificate(kServerSSLPubCert, kServerSSLPriKey);
  // generate public and private key
  mkDir(kSigKeyDir);
  // genKey(kSigPriKeyFile, kSigPubKeyFile);

  // create directory
  mkDir(kAllDeltaPackTmpDir);
  mkDir(kCompletePackDir);
  mkDir(kGenDeltaPackDir);
  mkDir(kDoneDeltaPackDir);
  mkDir(kSigDir);
  mkDir(kHashDir);

  initVersionMap();
  initDirectoryListener();
}

void OTAServer::initVersionMap() {
  gVcm_.setCallback<bool(const GeneralVersion&, const GeneralVersion&)>(
      findPackVCMCallback);

  struct stat st;
  if (-1 != ::stat(kIndxVerMapData, &st)) {
    indxFp_ = ::fopen(kIndxVerMapData, "r+");
    print<GeneralInfoCtrl>(std::cout, "Load VersionMap from [", kIndxVerMapData,
                           "]");
  } else {
    indxFp_ = ::fopen(kIndxVerMapData, "w+");
  }
  // reconstruct VersionMap
  char buffer[1024]{0};
  // line by line
  while (::fgets(buffer, 1024, indxFp_) != nullptr) {
    QString version = QString::fromStdString(std::string(buffer)).trimmed();
    if (version.isEmpty()) continue;
    print<GeneralInfoCtrl>(std::cout, version);
    gVcm_.append(version, true);
  }
}

bool OTAServer::mkDir(const QString& dir) {
  struct stat st;
  std::string d = dir.toStdString();
  // directory not exist
  if (-1 == stat(d.c_str(), &st)) {
    if (0 == ::mkdir(d.c_str(), 0755)) return false;
  }
  return true;
}

void OTAServer::directoryChanged(const QString& version) {
  char c;
  std::cout << "Found version [" + version.toStdString() +
                   "] was added, is it generating a new deltapack? [y/n]: "
            << std::flush;
  std::cin >> c;
  if (c == 'y' || c == 'Y') {
    bool success = gVcm_.append(version, false);
    if (success) {
      std::string ver = version.toStdString();
      ::fwrite(ver.data(), sizeof(char), ver.size(), indxFp_);
      ::fwrite("\n", sizeof(char), 1, indxFp_);
      ::fflush(indxFp_);
      ::fsync(::fileno(indxFp_));
      print<GeneralSuccessCtrl>(std::cout, "append new version successfully!");
    } else
      print<GeneralErrorCtrl>(std::cout, "append new version failed!");
  } else {
    print<GeneralWarnCtrl>(std::cout, "cancel!");
    bool success = gVcm_.append(version, true);
    if (success) {
      std::string ver = version.toStdString();
      ::fwrite(ver.data(), sizeof(char), ver.size(), indxFp_);
      ::fwrite("\n", sizeof(char), 1, indxFp_);
      ::fflush(indxFp_);
      ::fsync(::fileno(indxFp_));
    }
  }
}

void OTAServer::quit() { server_->quit(); }

void OTAServer::start(quint16 port) {
  if (port == 0) port = kServerPort;
  auto address = InetAddress(port);
  print<GeneralInfoCtrl>(std::cout, "OTAServer listen:", address.to_string());
  // Server start listening...
  server_->start(address);
}

void OTAServer::newConnection(TcpConnectionPtr conn) {
  print<GeneralSuccessCtrl>(std::cout,
                            "Client App:", conn->peer_address().to_string(),
                            "fd: ", conn->fd());
}

void OTAServer::closeConnection(TcpConnectionPtr /*conn*/) {
  print<GeneralErrorCtrl>(std::cout, "Client App had closed!");
}

// generate hash file for complete pack special version
// when system administrator added a new version package
QFileInfo genHashFileFromCompletePack(const QString& version) {
  // ./CompletePack/1.0.1/file_log
  // ./CompletePack/1.1.0/file_log
  QString prefix = kCompletePackDir + version;
  QString filelog(prefix + "/" + kFileLogName);
  merkle_hash_t rootHash =
      FileLogger::GetHashFromLogFile(filelog, prefix + "/");

  // save hash string to file
  // ./Hashs/1.1.0_hash
  QString hashfile = kHashDir + version + "_hash";
  QFile file(hashfile);
  file.open(QFile::WriteOnly | QFile::Truncate);
  std::string hashv = rootHash.to_string();
  file.write(hashv.data(), hashv.size());
  file.flush();
  file.close();
  // return ./Hashs/1.1.0_hash filepath
  return QFileInfo(hashfile);
}

QFileInfo genDeltaPackSigFile(const QString& delVersion) {
  // update and rollback
  // ./DoneDeltaPack/1.0.0-1.0.2.tat.gz
  QString deltaPackTarGzFile = kDoneDeltaPackDir + delVersion + ".tar.gz";
  // ./Sigs/1.0.0-1.0.2_sig
  QString sigfile = kSigDir + delVersion + "_sig";
  otalib::sign(QFileInfo(deltaPackTarGzFile), QFileInfo(kSigPriKeyFile),
               delVersion);
  // ./DoneDeltaPack/1.0.0-1.0.2_sig
  QFile::copy(kDoneDeltaPackDir + delVersion + "_sig", sigfile);
  QFile::remove(kDoneDeltaPackDir + delVersion + "_sig");
  QFile::remove(QString(kDoneDeltaPackDir) + "hash");
  return QFileInfo(sigfile);
}

QFileInfo genDeltaPackTarGzFile(const QString& deltaPackDir,
                                const QString delVersion) {
  // ./DeltaPack/1.0.0-1.0.2
  // ./DoneDeltaPack/1.0.0-1.0.2.tar.gz
  QString doneDeltaPackFile = kDoneDeltaPackDir + delVersion + ".tar.gz";
  tar_create_archive_file_gzip(deltaPackDir, doneDeltaPackFile);
  return QFileInfo(doneDeltaPackFile);
}

bool findPackVCMCallback(const GeneralVersion& prev,
                         const GeneralVersion& next) {
  findPackCallback(prev, next);
  return true;
}

::std::tuple<QFileInfo, QFileInfo, QFileInfo> findPackCallback(
    const GeneralVersion& prev, const GeneralVersion& next) {
  // ./CompletePack/1.0.0
  // ./CompletePack/1.0.2
  QDir vPrev(kCompletePackDir + prev.toString());
  QDir vNext(kCompletePackDir + next.toString());
  if (!vPrev.exists() || !vNext.exists())
    return {QFileInfo(""), QFileInfo(""), QFileInfo("")};

  // rollback: ./DeltaPack/1.0.2-1.0.0
  // update:   ./DeltaPack/1.0.0-1.0.2
  QString rollbackStr = next.toString() + "-" + prev.toString();
  QString updateStr = prev.toString() + "-" + next.toString();

  QDir rollbackPack(kGenDeltaPackDir + rollbackStr);
  QDir updatePack(kGenDeltaPackDir + updateStr);

  // generate delta packages
  if (!updatePack.exists() && !rollbackPack.exists()) {
    bool success = generateDeltaPack(vPrev, vNext, rollbackPack, updatePack);
    if (!success) return {QFileInfo(""), QFileInfo(""), QFileInfo("")};
  }
  // rollback
  // ./DoneDeltaPack/1.0.2-1.0.0.tar.gz
  QFileInfo rollbackDelta(kDoneDeltaPackDir + rollbackStr + ".tar.gz");
  if (!rollbackDelta.exists()) {
    rollbackDelta =
        genDeltaPackTarGzFile(rollbackPack.absolutePath(), rollbackStr);
  }
  // update
  // ./DoneDeltaPack/1.0.0-1.0.2.tar.gz
  QFileInfo updateDelta(kDoneDeltaPackDir + updateStr + ".tar.gz");
  if (!updateDelta.exists()) {
    updateDelta = genDeltaPackTarGzFile(updatePack.absolutePath(), updateStr);
  }
  // ./Hashs/1.0.2_hash
  QFileInfo updateHash(kHashDir + next.toString() + "_hash");
  if (!updateHash.exists()) {
    updateHash = genHashFileFromCompletePack(next.toString());
  }
  // rollback deltapack tar.gz sig
  // ./Sigs/1.0.2-1.0.0_sig
  QFileInfo rollbackSig(kSigDir + rollbackStr + "_sig");
  if (!rollbackSig.exists()) {
    rollbackSig = genDeltaPackSigFile(rollbackStr);
  }
  // update deltapack tar.gz sig
  // ./Sigs/1.0.0-1.0.2_sig
  QFileInfo updateSig(kSigDir + updateStr + "_sig");
  if (!updateSig.exists()) {
    updateSig = genDeltaPackSigFile(updateStr);
  }

  // {file.tar.gz, update_hash, update_sig}
  return {updateDelta, updateHash, updateSig};
}

void OTAServer::dumpAppClientInfo(const QString& client,
                                  const ClientInfo<GeneralVersion>& cinfo) {
  print<GeneralSuccessCtrl>(std::cout, "Client info: ", client);
  print<GeneralInfoCtrl>(std::cout, "\tname", cinfo.name);
  print<GeneralInfoCtrl>(std::cout, "\ttype", cinfo.type);
  print<GeneralInfoCtrl>(std::cout, "\tversion", cinfo.version.toString());
}

///
/// \brief OTAServer::readMessage
/// \param conn
/// \return
/// return true => Read data completely. Start to send all data at once
/// return false => The received data is incomplete and needs to be read again
///
bool OTAServer::readMessage(TcpConnectionPtr conn) {
  auto bytes = QByteArray(conn->recver()->peek(), conn->recver()->readable());
  // receive all data
  conn->recver()->retired_all();

  auto type = Tell<GeneralVersion>(bytes);
  // request
  if (type == JsonType::Request) {
    auto clientInfo =
        getClientInfo<GeneralVersion>(QJsonDocument::fromJson(bytes).object());

    // dump client information
    dumpAppClientInfo(conn->peer_address().to_string(), clientInfo);

    auto ciOpt = matchStrategy(clientInfo, gVcm_);

    QJsonDocument jDoc;
    if (!ciOpt.has_value()) {
      print<GeneralErrorCtrl>(std::cout, "match strategy failed!");
      jDoc = MakeResponse(sAction::None, StrategyType::error,
                          clientInfo.version, &clientInfo.version);
    } else {
      auto [saction, stype, dvertype] = ciOpt.value();

      print<GeneralInfoCtrl>(std::cout,
                             "client version: ", clientInfo.version.toString());
      print<GeneralInfoCtrl>(std::cout,
                             "match strategy version: ", dvertype.toString());
      sAction sac = sAction::None;

      if (clientInfo.version != dvertype) {
        if (saction == StrategyAction::update)
          sac = sAction::Update;
        else if (saction == StrategyAction::rollback)
          sac = sAction::Rollback;
      }

      jDoc = MakeResponse(sac, stype, clientInfo.version, &dvertype);
    }
    auto bytes = jDoc.toJson();
    // Send response to app client
    conn->sender()->append(bytes.data(), bytes.size());

    // confirm
  } else if (type == JsonType::Confirm) {
    auto resp = ParseConfirm<GeneralVersion>(bytes);
    if (!resp.has_value()) return true;
    const auto& [sac, stype, from, dest] = resp.value();
    if (sac == sAction::None) return true;

    std::vector<VersionMap<GeneralVersion>::EdgeType> verPath;
    if (sac == sAction::Update) {
      verPath = gVcm_.search<SearchStrategy::vUpdate>(from, dest);
    } else if (sac == sAction::Rollback) {
      verPath = gVcm_.search<SearchStrategy::vRollback>(from, dest);
    }
    // path empty ...
    if (verPath.empty()) {
      return true;
    }

    // ./tmpAllDeltaPack/1.0.0_1.0.2
    // ./tmpAllDeltaPack/1.0.0_1.0.2.tar.gz
    QString destDir =
        kAllDeltaPackTmpDir + from.toString() + "_" + dest.toString();
    ::mkdir(destDir.toStdString().c_str(), 0755);

    QString completeDeltaPackFile = destDir + ".tar.gz";

    /*
        // 1.0.0 -> 1.0.1
        ./tmpAllDeltaPack/1.0.0_1.0.2/1.0.0-1.0.1.tar.gz
        ./tmpAllDeltaPack/1.0.0_1.0.2/1.0.1_hash
        ./tmpAllDeltaPack/1.0.0_1.0.2/1.0.1_sig
        // 1.0.1 -> 1.0.2
        ./tmpAllDeltaPack/1.0.0_1.0.2/1.0.1-1.0.2.tar.gz
        ./tmpAllDeltaPack/1.0.0_1.0.2/1.0.2_hash
        ./tmpAllDeltaPack/1.0.0_1.0.2/1.0.2_sig
        ...
        ./tmpAllDeltaPack/1.0.0_1.0.2/apply_log
    */
    if (!QFile(completeDeltaPackFile).exists()) {
      getPackFromPaths<GeneralVersion,
                       ::std::tuple<QFileInfo, QFileInfo, QFileInfo>(
                           const GeneralVersion&, const GeneralVersion&)>(
          QDir(destDir), verPath, findPackCallback);

      // ./tmpAllDeltaPack/1.0.0_1.0.2 -> ./tmpAllDeltaPack/1.0.0_1.0.2.tar.gz
      tar_create_archive_file_gzip(destDir, completeDeltaPackFile);
    }

    // map v1_v2.tar.gz file into net::Buffer
    FileLoader(completeDeltaPackFile.toStdString()).readAll(conn->sender());

    QDir(destDir).removeRecursively();
  }
  // sending
  return true;
}
