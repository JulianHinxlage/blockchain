//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "sha.h"
#include <openssl/sha.h>

Blob<256> sha256(const std::string& data) {
    return sha256(data.data(), data.size());
}

Blob<256> sha256(const char* data, int size) {
    Blob<256> hash;
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, data, size);
    SHA256_Final((uint8_t*)&hash, &ctx);
    return hash;
}
