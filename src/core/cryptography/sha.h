//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "util/Blob.h"
#include <string>

Blob<256> sha256(const std::string& data);
Blob<256> sha256(const char *data, int size);
