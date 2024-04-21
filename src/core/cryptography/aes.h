//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "util/Blob.h"
#include <string>

bool aesEncrypt(const std::string& clearText, std::string& chiperText, Blob<256> key, Blob<256> iv, Blob<128>& tag);

bool aesDecrypt(const std::string& chiperText, std::string& clearText, Blob<256> key, Blob<256> iv, Blob<128> tag);
