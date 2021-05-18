#include "otalib/sha256_hash.h"

using namespace otalib;

void hashfile_test() {
  // Server generate hash file
  if (generate_hash_log_file("./subdir", "merkle_tree_hash.log"))
    print<GeneralSuccessCtrl>(
        std::cout, "server generate merkle hash log file successfully!");

  // Simulate client verify
  QStringList file_hash;
  merkle_hash_t bad_hash(
      "17ab19df4a0d41741e353676afaf8e08aefa987e06f43841e7829d39da07166e");
  merkle_hash_t proof_root_hash;

  if (read_lines_hash_log_file("merkle_tree_hash.log", file_hash,
                               proof_root_hash)) {
    print<GeneralSuccessCtrl>(std::cout,
                              "client read merkle hash log file successfully!");
    for (auto item : file_hash) print<GeneralInfoCtrl>(std::cout, item);
  }

  print<GeneralSuccessCtrl>(std::cout, proof_root_hash.to_string());

  if (verify_merkle_tree("./subdir", proof_root_hash)) {
    //  if (verify_merkle_tree("./subdir", bad_hash)) {
    print<GeneralSuccessCtrl>(std::cout, "client verify successfully!");
  } else {
    print<GeneralErrorCtrl>(std::cout, "client verify failed!");
  }
}
