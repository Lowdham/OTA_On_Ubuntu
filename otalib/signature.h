/*
 * MIT License
 *
 * Copyright (c) 2020 Batuhan AVLAYAN
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 * Copyright (c) 2021 THF5000
 */

#ifndef SIGNATURE_H
#define SIGNATURE_H

#include <openssl/err.h>
#include <openssl/pem.h>
#include <openssl/rsa.h>

#include <QByteArray>
#include <QCryptographicHash>
#include <QFile>
#include <QFileInfo>
#include <iostream>

#include "logger/logger.h"

namespace otalib {

constexpr int32_t kKeyLength = 1024;
constexpr int32_t kPublicExponent = 59;
constexpr int32_t kPublicKeyPem = 1;
constexpr int32_t kPrivateKeyPem = 0;
constexpr auto kHashAlgorithm = QCryptographicHash::Md5;
constexpr int kSignHashAlgorithm = NID_sha1;
void generateKeyPair();

// Requrie the file has already opened.
// Return the signature of file after RSA encrpyting.
QByteArray sign(QFile* file, RSA* private_key);

//
bool verify(const QByteArray& hval, QByteArray& sig,
            const QFileInfo& kfile) noexcept;

}  // namespace otalib

#endif  // SIGNATURE_H
