#pragma once

#include "util/Blob.h"
#include <string>

Blob<256> sha256(const void* msg, int bytes);
Blob<256> sha256(const std::string &msg);
Blob<512> sha512(const void* msg, int bytes);
Blob<512> sha512(const std::string& msg);

Blob<256> sha3_256(const void* msg, int bytes);
Blob<256> sha3_256(const std::string& msg);
Blob<512> sha3_512(const void* msg, int bytes);
Blob<512> sha3_512(const std::string& msg);
