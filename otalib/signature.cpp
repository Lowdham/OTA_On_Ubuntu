#include "signature.h"

namespace otalib {
namespace sig_details {

bool rsaSignThroughCmd(const QString& from, const QString& to,
                       const QString& prikey) {
  //
  static const ::std::string prefix = "openssl dgst -sign ";
  ::std::string cmd = prefix + prikey.toStdString() + " " +
                      kSignHashAlgorithmCmd + "-out " + to.toStdString() + " " +
                      from.toStdString();
  system(cmd.c_str());
  return true;
}

bool rsaVerifyThroughCmd(const QString& sig, const QString& hash,
                         const QString& pubkey) {
  //
  static const ::std::string prefix = "openssl dgst -verify ";
  ::std::string cmd = prefix + pubkey.toStdString() + " " +
                      kSignHashAlgorithmCmd + "-signature " +
                      sig.toStdString() + " " + hash.toStdString();
  ::std::string ret = getCmdResult(cmd);
  if (ret == "Verified OK")
    return true;
  else  // "Verification Failure"
    return false;
}

}  // namespace sig_details
void genKey(const QString& prikey_file, const QString& pubkey_file) {
  // Generate rsa keys in current directory.
  // "cmd": openssl genrsa -out prikey_file kKeyLength
  ::std::string cmd = "openssl genrsa -out " + prikey_file.toStdString() + " " +
                      ::std::to_string(kKeyLength) + " > /dev/null 2>&1";
  system(cmd.c_str());
  cmd.clear();

  // openssl rsa -pubout -in prikey_file -out pubkey_file
  cmd = "openssl rsa -pubout -in " + prikey_file.toStdString() + " -out " +
        pubkey_file.toStdString() + " > /dev/null 2>&1";
  system(cmd.c_str());
}

bool sign(const QFileInfo& target, const QFileInfo& prikey) noexcept {
  // Get the hash value of target, and then generate the signature.
  QFile tfile(target.absoluteFilePath());
  QString hfilepath = target.absoluteDir().filePath(QStringLiteral("hash"));
  QFile hfile(hfilepath);
  QString dfilepath = target.absoluteDir().filePath(QStringLiteral("sig"));
  QFile dfile(dfilepath);
  if (tfile.open(QFile::ReadOnly)) {
    //
    QCryptographicHash hash(kHashAlgorithm);
    hash.addData(&tfile);
    QByteArray hashval = hash.result();
    tfile.close();
    if (hfile.open(QFile::WriteOnly | QFile::Truncate)) {
      if (hfile.write(hashval) != hashval.size()) {
        hfile.close();
        return false;
      }
      hfile.close();
      // do signature through cmd.
      return sig_details::rsaSignThroughCmd(hfilepath, dfilepath,
                                            prikey.absoluteFilePath());
    } else {
      return false;
    }
  } else {
    // Open failed.
    return false;
  }
}

bool verify(const QFileInfo& hash, const QFileInfo& signature,
            const QFileInfo& pubkey) noexcept {
  //
  return sig_details::rsaVerifyThroughCmd(signature.absoluteFilePath(),
                                          hash.absoluteFilePath(),
                                          pubkey.absoluteFilePath());
}

}  // namespace otalib
