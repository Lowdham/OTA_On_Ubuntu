#include "signature.h"

#include <unistd.h>
namespace otalib {
namespace sig_details {

::std::string getCmdResult(const ::std::string& cmd) {
#ifdef __linux__
  if (cmd.empty()) return ::std::string();

  FILE* pipe = NULL;
  if ((pipe = popen(cmd.c_str(), "r")) == NULL) return ::std::string();

  ::std::string ret;
  char buffer[1024 * 8] = {0};
  while (fgets(buffer, sizeof(buffer), pipe)) ret += buffer;
  pclose(pipe);

  uint32_t size = ret.size();
  if (size > 0 && ret[size - 1] == '\n') ret = ret.substr(0, size - 1);
  return ret;
#endif  // !__linux__
#if defined(_WIN64) || defined(_WIN32)
  if (cmd.empty()) return ::std::string();

  SECURITY_ATTRIBUTES sa = {sizeof(SECURITY_ATTRIBUTES), NULL, TRUE};
  HANDLE hRead, hWrite;
  if (!CreatePipe(&hRead, &hWrite, &sa, 0)) return ::std::string();

  STARTUPINFO si = {sizeof(STARTUPINFO)};
  GetStartupInfo(&si);
  si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
  si.wShowWindow = SW_HIDE;
  si.hStdError = hWrite;
  si.hStdOutput = hWrite;

  PROCESS_INFORMATION pi;
  ::std::wstring wstr(cmd.size(), '\0');
  mbstowcs(wstr.data(), cmd.c_str(), cmd.size());
  if (!CreateProcess(NULL, wstr.data(), NULL, NULL, TRUE, NULL, NULL, NULL, &si,
                     &pi))
    return ::std::string();

  CloseHandle(hWrite);

  std::string strRet;
  char buff[1024 * 8] = {0};
  DWORD dwRead = 0;
  while (ReadFile(hRead, buff, 1024, &dwRead, NULL))
    strRet.append(buff, dwRead);

  CloseHandle(hRead);

  return strRet;
#endif  // !defined(_WIN64) && !defined(_WIN32)
}

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

bool sign(const QFileInfo& target, const QFileInfo& prikey) {
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
            const QFileInfo& pubkey) {
  //
  return sig_details::rsaVerifyThroughCmd(signature.absoluteFilePath(),
                                          hash.absoluteFilePath(),
                                          pubkey.absoluteFilePath());
}

}  // namespace otalib
