
#include "../../otalib/signature.h"
#include "../../otalib/logger/logger.h"

namespace otalib {
bool testfunc() {
// generateKeyPair();

QFile file("testfile.txt");
if (file.open(QFile::ReadOnly)) {
  FILE* priio = NULL;
  priio = fopen("private_key", "rb");
  RSA* private_key = PEM_read_RSAPublicKey(priio, &private_key, NULL, NULL);
  fclose(priio);
  QByteArray sig = encrypt(&file, private_key);
  if (sig.isEmpty()) {
    print<GeneralFerrorCtrl>(std::cerr, "Error occured in encypt().");
    return false;
  }

  print<GeneralFerrorCtrl>(
      std::cout,
      "Succeessfully generate the signature of \"testfile.txt\"'s summary.");

  //
  QFileInfo public_key("public_key");
  QCryptographicHash hash(kHashAlgorithm);
  hash.addData(&file);
  QByteArray hval = hash.result();
  if (verify(hval, sig, public_key)) {
    print<GeneralFerrorCtrl>(std::cout,
                              "Succeessfully verify the signature of "
                              "\"testfile.txt\"'s summary.");
    return true;
  } else {
    print<GeneralFerrorCtrl>(
        std::cerr,
        "Failed to verify the signature of \"testfile.txt\"'s summary.");
    return false;
  }
} else {
  print<GeneralFerrorCtrl>(std::cerr, "Cannot open private_key file.");
  return false;
}
}
}