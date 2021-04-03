#include <iostream>

#include "logger.h"

using namespace std;

int main() {
    LOG_INFO("INFO");
    LOG_DEBUG("DEBUG");
    LOG_ERROR("ERROR");
    LOG_WARN("WARN");

    return 0;
}
