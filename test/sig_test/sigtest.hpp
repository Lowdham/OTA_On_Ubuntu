
#include "../../otalib/logger/logger.h"
#include "../../otalib/signature.h"

using namespace otalib;
bool testfunc() {
  // generateKeyPair();

  QFile file("test.html");
  if (file.open(QFile::ReadOnly)) {
    FILE* pubio = NULL;
    pubio = fopen("public_key", "rb");
    RSA* t = RSA_new();
    RSA* public_key = PEM_read_RSAPublicKey(pubio, &t, NULL, NULL);

    fclose(pubio);
    QByteArray sig = encrypt(&file, public_key);
    RSA_free(t);
    if (sig.isEmpty()) {
      print<GeneralFerrorCtrl>(std::cerr, "Error occured in encypt().");
      return false;
    }

    print<GeneralSuccessCtrl>(
        std::cout,
        "Succeessfully generate the signature of \"test.html\"'s summary.");

    //
    QFileInfo private_key("private_key");
    QCryptographicHash hash(kHashAlgorithm);
    hash.addData(&file);
    QByteArray hval = hash.result();
    if (verify(hval, sig, private_key)) {
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
