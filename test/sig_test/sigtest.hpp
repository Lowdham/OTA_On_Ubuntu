
#include "../../otalib/logger/logger.h"
#include "../../otalib/signature.h"

using namespace otalib;
bool testfunc() {
   generateKeyPair();

  QFile file("test.html");
  if (file.open(QFile::ReadOnly)) {
    FILE* priio = NULL;
    if ((priio = fopen("private_key", "rb")) == nullptr) {
      print<GeneralFerrorCtrl>(
          std::cerr, "Error occured in openning \"private_key\" file.");
      return false;
    }

    RSA* private_key = RSA_new();
    if (PEM_read_RSAPrivateKey(priio, &private_key, NULL, NULL) == nullptr) {
      fclose(priio);
      RSA_free(private_key);
      print<GeneralFerrorCtrl>(std::cerr,
                               "Error occured in reading private_key.");
      return false;
    };

    fclose(priio);
    QByteArray sig = sign(&file, private_key);
    RSA_free(private_key);
    if (sig.isEmpty()) {
      print<GeneralFerrorCtrl>(std::cerr, "Error occured in sign().");
      return false;
    }

    print<GeneralSuccessCtrl>(
        std::cout,
        "Succeessfully generate the signature of \"test.html\"'s summary.");

    //
    QFileInfo public_key("public_key");
    QCryptographicHash hash(kHashAlgorithm);
    hash.addData(&file);
    QByteArray hval = hash.result();
    if (verify(hval, sig, public_key)) {
      print<GeneralSuccessCtrl>(std::cout,
                                "Succeessfully verify the signature of "
                                "\"test.html\"'s summary.");
      return true;
    } else {
      print<GeneralFerrorCtrl>(
          std::cerr,
          "Failed to verify the signature of \"test.html\"'s summary.");
      return false;
    }
  } else {
    print<GeneralFerrorCtrl>(std::cerr, "Cannot open private_key file.");
    return false;
  }
}
