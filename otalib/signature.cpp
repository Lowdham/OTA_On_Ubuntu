#include "signature.h"

namespace otalib {
namespace sig_details {

// Create_RSA function creates public key and private key file
RSA* create_RSA(RSA* keypair, int pem_type, const char* file_name) {
  RSA* rsa = NULL;
  FILE* fp = NULL;

  if (pem_type == kPublicKeyPem) {
    fp = fopen(file_name, "w");
    PEM_write_RSAPublicKey(fp, keypair);
    fclose(fp);

    fp = fopen(file_name, "rb");
    PEM_read_RSAPublicKey(fp, &rsa, NULL, NULL);
    fclose(fp);

  } else if (pem_type == kPrivateKeyPem) {
    fp = fopen(file_name, "w");
    PEM_write_RSAPrivateKey(fp, keypair, NULL, NULL, 0, NULL, NULL);
    fclose(fp);

    fp = fopen(file_name, "rb");
    PEM_read_RSAPrivateKey(fp, &rsa, NULL, NULL);
    fclose(fp);
  }

  return rsa;
}

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
#endif  // !__linuex__
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
  ::std::string cmd = prefix + prikey.toStdString() + kSignHashAlgorithmCmd +
                      "-out " + to.toStdString() + " " + from.toStdString();
  system(cmd.c_str());
  return true;
}

bool rsaVerifyThroughCmd(const QString& sig, const QString& hash,
                         const QString& pubkey) {
  //
  static const ::std::string prefix = "openssl dgst -verify ";
  ::std::string cmd = prefix + pubkey.toStdString() + kSignHashAlgorithmCmd +
                      "-signature " + sig.toStdString() + " " +
                      hash.toStdString();
  ::std::string ret = getCmdResult(cmd);
  if (ret == "Verified OK")
    return true;
  else  // "Verification Failure"
    return false;
}

}  // namespace sig_details

void generateKeyPair() {
  RSA* private_key = NULL;
  RSA* public_key = NULL;
  RSA* keypair = NULL;
  BIGNUM* bne = NULL;
  int ret = 0;

  char private_key_pem[12] = "private_key";
  char public_key_pem[11] = "public_key";
  bne = BN_new();
  ret = BN_set_word(bne, kPublicExponent);
  if (ret != 1) {
    print<GeneralFerrorCtrl>(std::cerr,
                             "An error occurred in BN_set_word() method");
    free(bne);
    return;
  }
  keypair = RSA_new();
  ret = RSA_generate_key_ex(keypair, kKeyLength, bne, NULL);
  if (ret != 1) {
    print<GeneralFerrorCtrl>(
        std::cerr, "An error occurred in RSA_generate_key_ex() method");
    free(bne);
    free(keypair);
    return;
  }
  private_key =
      sig_details::create_RSA(keypair, kPrivateKeyPem, private_key_pem);
  print<GeneralSuccessCtrl>(std::cout,
                            "Private key pem file has been created.");

  public_key = sig_details::create_RSA(keypair, kPublicKeyPem, public_key_pem);
  print<GeneralSuccessCtrl>(std::cout, "Public key pem file has been created.");

  free(private_key);
  free(public_key);
  free(keypair);
  free(bne);
}

QByteArray sign(QFile* file, RSA* private_key) {
  if (!file->isOpen()) return QByteArray();

  // Get the hash value of file.
  QCryptographicHash hash(kHashAlgorithm);
  hash.addData(file);
  QByteArray summary = hash.result();

  unsigned char buffer[4096];
  memset(buffer, 0, sizeof(char) * 4096);
  unsigned int sig_size = 0;
  if (RSA_sign(kSignHashAlgorithm,
               reinterpret_cast<const unsigned char*>(summary.data()),
               summary.size(), buffer, &sig_size, private_key) == 1) {
    return QByteArray(reinterpret_cast<char*>(buffer), sig_size);
  } else {
    return QByteArray();
  }
}

bool verify(const QByteArray& hval, QByteArray& sig,
            const QFileInfo& kfile) noexcept {
  FILE* pubio = NULL;
  RSA* pubkey = RSA_new();
  pubio = fopen(kfile.absoluteFilePath().toStdString().c_str(), "rb");
  if ((PEM_read_RSAPublicKey(pubio, &pubkey, NULL, NULL)) == nullptr) {
    fclose(pubio);
    return false;
  } else {
    fclose(pubio);
  }
  ERR_print_errors_fp(stderr);

  if (RSA_verify(kSignHashAlgorithm,
                 reinterpret_cast<const unsigned char*>(hval.data()),
                 hval.size(),
                 reinterpret_cast<const unsigned char*>(sig.data()), sig.size(),
                 pubkey) == 1) {
    ERR_print_errors_fp(stderr);
    RSA_free(pubkey);
    return true;
  } else {
    RSA_free(pubkey);
    ERR_print_errors_fp(stderr);
    return false;
  }
}

void genKeyPairByCmd() {}

bool sign(const QFileInfo& target, const QFileInfo& prikey) {
  // Get the hash value of target, and then generate the signature.
  QFile tfile(target.absolutePath());
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
                                            prikey.absolutePath());
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
  QString hfilepath = hash.absolutePath();
  QDir dir = hash.absoluteDir();
  QString sfilepath = dir.filePath(QStringLiteral("hash"));
  QString pfilepath = dir.filePath(QStringLiteral("sig"));
  return sig_details::rsaVerifyThroughCmd(sfilepath, hfilepath, pfilepath);
}

}  // namespace otalib
