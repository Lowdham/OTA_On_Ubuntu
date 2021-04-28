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
#if defined(_WIN64) || defined(_WIN32)
#include <Windows.h>
#endif  // !defined(_WIN64) && !defined(_WIN32)

namespace otalib {

constexpr int32_t kKeyLength = 1024;
constexpr int32_t kPublicExponent = 59;
constexpr int32_t kPublicKeyPem = 1;
constexpr int32_t kPrivateKeyPem = 0;
constexpr auto kHashAlgorithm = QCryptographicHash::Md5;
constexpr int kSignHashAlgorithm = NID_sha1;
static const ::std::string kSignHashAlgorithmCmd = "-sha256 ";
void generateKeyPair();

// Requrie the file has already opened.
// Return the signature of file after RSA encrpyting.
QByteArray sign(QFile* file, RSA* private_key);

//
bool verify(const QByteArray& hval, QByteArray& sig,
            const QFileInfo& kfile) noexcept;

// Generate the key under current directory.
void genKey(const QString& prikey_file, const QString& pubkey_file);

// Sign the target.
bool sign(const QFileInfo& target, const QFileInfo& prikey);

// Verify the signature
bool verify(const QFileInfo& hash, const QFileInfo& signature,
            const QFileInfo& pubkey);

}  // namespace otalib

#endif  // SIGNATURE_H
