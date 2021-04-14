#include "../../otalib/vcm.hpp"
using namespace std;

using namespace otalib;

void test_vcm() {
  VersionMap<int> vm;
  for (int i = 0; i <= 500; i++) vm.append(i, true);
  auto r = vm.search<vUpdate>(251, 489);
  if (r.empty()) {
    cout << "Not Found!" << std::endl;
    return;
  }

  for (auto &y : r) {
    std::cout << y.first << "->" << y.second << " ";
  }
  std::cout << "\n";
}
