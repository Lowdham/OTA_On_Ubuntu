
#include "../../otalib/logger/logger.h"
#include "../../otalib/signature.h"

using namespace otalib;


bool testfunc2(){
    genKey("private_key","public_key");
    if(sign(QFileInfo("test.html"),QFileInfo("private_key")))
        print<GeneralSuccessCtrl>(std::cout,"Sign succeed.");
    else {
        print<GeneralFerrorCtrl>(std::cerr,"Sign failed.");
        return false;
    }
    //
    if(verify(QFileInfo("hash"),QFileInfo("sig"),QFileInfo("public_key")))
        print<GeneralSuccessCtrl>(std::cout,"Verified succeed.");
    else {
        print<GeneralFerrorCtrl>(std::cerr,"Verification failed.");
        return false;
    }
    return true;
}
