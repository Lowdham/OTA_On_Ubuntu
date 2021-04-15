#include "../../otalib/logger/logger.h"
#include "../../otalib/vcm.hpp"

using namespace otalib;

GeneralVersion randomGen() {
  int _1 = rand() % 7;
  int _2 = rand() % 10;
  int _3 = rand() % 9 + 1;
  QString info = QString::number(_1) + "." + QString::number(_2) + "." +
                 QString::number(_3);
  return GeneralVersion(info);
}

void test_vcm() {
  print<GeneralInfoCtrl>(
      std::cout,
      "Mid's capacity:" + std::to_string(VcmControl<VCM::Mid>::vcm_capacity));

  VersionMap<GeneralVersion> vm;
  GeneralVersion vbase("0.0.1");
  auto vcur = vbase;
  for (uint64_t i = 0; i <= GeneralVersion::vcm_capacity; i++) {
    vm.append(vcur, true);
    vcur = GeneralVersion::add(vcur, vbase);
  }

  auto start = clock();
  auto end = clock();
  auto during = end;
  uint64_t success = 0;
  uint64_t fail = 0;
  std::vector<std::pair<GeneralVersion, GeneralVersion>> fails;
  for (uint64_t i = 0; i < 100000; ++i) {
    auto s = randomGen();
    GeneralVersion d;
    while (true) {
      d = randomGen();
      if (s != d) break;
    }
    start = clock();
    auto r = vm.search<SearchStrategy::vUpdate>(s, d);
    end = clock();
    during += end - start;
    if (r.empty()) {
      ++fail;
      fails.emplace_back(s, d);
    } else
      ++success;
  }
  auto sec = (static_cast<double>(during) / CLOCKS_PER_SEC);
  print<GeneralInfoCtrl>(std::cout,
                         "Total cost: " + QString::number(sec) + " seconds");
  print<GeneralSuccessCtrl>(std::cout,
                            "Total success: " + QString::number(success));
  print<GeneralErrorCtrl>(std::cout, "Total fail: " + QString::number(fail));
  print<GeneralErrorCtrl>(std::cout, "Failed path: ");
  for (auto i : fails)
    print<GeneralErrorCtrl>(std::cout, "[" + i.first.toString() + "]->[" +
                                           i.second.toString() + "]");
}
