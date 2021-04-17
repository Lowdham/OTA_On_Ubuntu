/*
 * MIT License
 *
 * Copyright (c) 2020 Batuhan AVLAYAN
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Copyright (c) 2021 THF5000 thf5000y2k@gmail.com
 */

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

/*
 * @brief   public_ecrypt function encrypts data.
 * @return  If It is fail, return -1
 */
int public_encrypt(int flen, unsigned char* from, unsigned char* to, RSA* key,
                   int padding) {
  int result = RSA_public_encrypt(flen, from, to, key, padding);
  return result;
}

/*
 * @brief   private_decrypt function decrypt data.
 * @return  If It is fail, return -1
 */
int private_decrypt(int flen, unsigned char* from, unsigned char* to, RSA* key,
                    int padding) {
  int result = RSA_private_decrypt(flen, from, to, key, padding);
  return result;
}

/*
 * @brief   create_ecrypted_file function creates .bin file. It contains
 * encrypted data.
 */
void create_encrypted_file(char* encrypted, RSA* key_pair) {
  FILE* encrypted_file = fopen("encrypted_file.bin", "w");
  fwrite(encrypted, sizeof(*encrypted), RSA_size(key_pair), encrypted_file);
  fclose(encrypted_file);
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

QByteArray encrypt(QFile* file, RSA* public_key) {
  if (!file->isOpen()) return QByteArray();

  // Get the hash value of file.
  QCryptographicHash hash(kHashAlgorithm);
  hash.addData(file);
  QByteArray summary = hash.result();

  // KeyEncrypt
  char* sig = static_cast<char*>(malloc(RSA_size(public_key)));
  if (!sig) {
    print<GeneralFerrorCtrl>(std::cerr,
                             "Cannot allocate memories for signature.");
    return QByteArray();
  }

  int sig_length = sig_details::public_encrypt(
      summary.size() + 1, reinterpret_cast<unsigned char*>(summary.data()),
      reinterpret_cast<unsigned char*>(sig), public_key,
      RSA_PKCS1_OAEP_PADDING);
  if (sig_length == -1) {
    print<GeneralFerrorCtrl>(std::cerr, "Error occurred in encrypt().");
    free(sig);
    return QByteArray();
  }

  QByteArray data(sig, sig_length);
  free(sig);
  return data;
}

bool verify(const QByteArray& hval, QByteArray& sig,
            const QFileInfo& kfile) noexcept {
  //
  BIO* pubio = NULL;
  pubio = BIO_new_file(kfile.absolutePath().toStdString().c_str(), "rb");
  RSA* pubkey = PEM_read_bio_RSAPrivateKey(pubio, &pubkey, NULL, NULL);
  BIO_vfree(pubio);

  char* plain = static_cast<char*>(malloc(sig.size()));
  if (!plain) {
    print<GeneralFerrorCtrl>(std::cerr,
                             "Cannot allocate memories for plaintext.");
    return false;
  }
  int plain_length = sig_details::private_decrypt(
      sig.size(), reinterpret_cast<unsigned char*>(sig.data()),
      reinterpret_cast<unsigned char*>(plain), pubkey, RSA_PKCS1_OAEP_PADDING);
  if (plain_length == -1) {
    print<GeneralFerrorCtrl>(std::cerr, "Error occurred in decrypt().");
    free(plain);
    return false;
  }

  // Do a quick comparsion.
  if (plain_length != hval.size()) {
    free(plain);
    return false;
  }
  for (int i = 0; i < plain_length; ++i)
    if (plain[i] != hval[i]) {
      free(plain);
      return false;
    }

  free(plain);
  return true;
}

}  // namespace otalib
