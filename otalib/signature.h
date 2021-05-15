#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>
#include <stdio.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <iostream>

#include "logger/logger.h"
#include "shell_cmd.hpp"

namespace otalib {

constexpr int32_t kKeyLength = 1024;
constexpr auto kHashAlgorithm = QCryptographicHash::Md5;
static const ::std::string kSignHashAlgorithmCmd = "-sha256 ";

// Generate the key under current directory.
void genKey(const QString& prikey_file, const QString& pubkey_file);

// Sign the target.
bool sign(const QFileInfo& target, const QFileInfo& prikey) noexcept;

// Verify the signature
bool verify(const QFileInfo& hash, const QFileInfo& signature,
            const QFileInfo& pubkey) noexcept;

}  // namespace otalib

#endif  // SIGNATURE_H
