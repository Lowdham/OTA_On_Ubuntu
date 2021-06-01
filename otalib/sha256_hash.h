#ifndef SHA256_HASH_H
#define SHA256_HASH_H

#include <QDir>
#include <unordered_map>

#include "logger/logger.h"
#include "merklecpp.h"

namespace otalib {

namespace {
constexpr const static size_t kChunkSize = 2048;
constexpr const static size_t kSha256Len = SHA256_DIGEST_LENGTH;
}  // namespace

using merkle_tree_t = merkle::TreeT<kSha256Len, merkle::sha256_openssl>;
using merkle_hash_t = merkle::Hash;
using hash_table_t = std::unordered_map<std::string, std::string>;

static void Sha256HashFile(const std::string &filename, uint8_t md[kSha256Len]) {
  FILE *fp = nullptr;
  fp = ::fopen(filename.c_str(), "rb");
  if (fp == nullptr) return;

  SHA256_CTX ctx;
  uint8_t buffer[kChunkSize];
  int n;
  ::SHA256_Init(&ctx);
  while ((n = ::fread(buffer, sizeof(uint8_t), kChunkSize, fp)) > 0) {
    ::SHA256_Update(&ctx, buffer, n);
  }
  ::SHA256_Final(md, &ctx);
  ::fclose(fp);
}

///
/// \brief CalcFileSha256Hash
/// \param dir_name
/// \param tree
/// \param htable
/// \return
///
static bool CalcFileSha256Hash(const QString &dir_name, merkle_tree_t &tree, hash_table_t &htable,
                               size_t &cnt) {
  QDir dir(dir_name);
  if (!dir.exists()) return false;

  dir.setFilter(QDir::Dirs | QDir::Files);
  // Directory first!
  dir.setSorting(QDir::DirsFirst);

  auto list = dir.entryInfoList();
  for (auto &file : list) {
    if (file.fileName() == "." || file.fileName() == "..") continue;
    QString name = QDir::fromNativeSeparators(dir_name + "/" + file.fileName());
    if (file.isDir()) {
      CalcFileSha256Hash(name, tree, htable, cnt);
    } else if (file.isFile()) {
      uint8_t md[kSha256Len];
      Sha256HashFile(name.toStdString(), md);
      cnt++;

      merkle_hash_t hash(md);
      tree.insert(hash);
      htable.emplace(name.toStdString(), hash.to_string());
    }
  }
  return true;
}

/// Client used to verify merkle tree hash
/// \brief verify_generate_merkle_tree
/// \param dir_name
/// \param proof_hash
/// \return
///
static bool VerifyMerkleTree(const QString &dir_name, const merkle_hash_t &proof_hash) {
  size_t files = 0;
  merkle_tree_t tree;
  hash_table_t htable;
  if (!CalcFileSha256Hash(dir_name, tree, htable, files)) return false;
  if (files == 0) return false;
  return tree.root() == proof_hash;
}

/// Server used to generate hash log file
/// \brief GenerateHashLogFile
/// \param dir_name
/// \param gen_log_file
/// \return
///
static bool GenerateHashLogFile(const QString &dir_name, const QString &gen_log_file) {
  FILE *fp = ::fopen(gen_log_file.toStdString().c_str(), "w");
  if (!fp) return false;

  merkle_tree_t tree;
  hash_table_t htable;

  size_t files = 0;
  if (!CalcFileSha256Hash(dir_name, tree, htable, files)) return false;
  if (files == 0) return false;

  for (auto &[path, hashv] : htable) {
    std::string line = path + "|" + hashv + "\n";
    ::fwrite(line.data(), sizeof(char), line.size(), fp);
    ::fflush(fp);
  }
  if (tree.root().size()) {
    std::string merkletree_hashv = "**" + tree.root().to_string() + "**";
    ::fwrite(merkletree_hashv.data(), sizeof(char), merkletree_hashv.size(), fp);
  }
  ::fflush(fp);
  ::fclose(fp);
  return true;
}

/// Server and Client used
/// \brief read_line_hash_log_file
/// \param log_file
/// \param lines
/// \param proof_root_hash
/// \return
///
static bool ReadLinesHashLogFile(const QString &log_file, QStringList &lines,
                                 merkle_hash_t &proof_root_hash) {
  QFile file(log_file);
  if (!file.open(QIODevice::ReadOnly)) return false;
  char buf[1024];
  while (-1 != file.readLine(buf, sizeof(buf))) {
    lines.append(QString(buf).trimmed());
  }
  if (lines.empty()) throw false;
  QString proof_hash = lines.back();
  if (proof_hash.startsWith("**") && proof_hash.endsWith("**")) {
    proof_hash.remove(0, 2);
    proof_hash.remove(proof_hash.size() - 2, 2);
    proof_root_hash = merkle_hash_t(proof_hash.toStdString());
    return true;
  }
  return false;
}

}  // namespace otalib

#endif
