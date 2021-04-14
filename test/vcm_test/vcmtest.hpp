#include "../../otalib/vcm.hpp"
#include "../../otalib/logger/logger.h"

using namespace otalib;

void test_vcm() {
  VersionMap<int> vm;
  for (int i = 0; i <= 5000; i++) vm.append(i, true);
  auto r = vm.search<SearchStrategy::vUpdate>(2310, 3211);
  if (r.empty()) {
    print<GeneralWarnCtrl>(std::cout, "Not Found!");
    return;
  }

  size_t update_size = 0;
  size_t rollback_size = 0;
  for (auto& y : r) {
    if (y.first < y.second)
      ++update_size;
    else
      ++rollback_size;
    std::cout << y.first << "->" << y.second << " ";
  }
  auto size = r.size();
  std::cout << "\n";
  print<GeneralSuccessCtrl>(std::cout, "Package:" + std::to_string(size));
  print<GeneralInfoCtrl>(std::cout, "Update:" + std::to_string(update_size));
  print<GeneralInfoCtrl>(std::cout,
                         "Rollback:" + std::to_string(rollback_size));
}
