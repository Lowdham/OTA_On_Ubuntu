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
    //    FILE* file = fopen("verify.sig", "wb");
    //    fwrite(sig.data(), sizeof(char), sig.size(), file);
    //    fclose(file);
    return false;
  }
}

}  // namespace otalib
