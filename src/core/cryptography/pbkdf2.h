//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "util/Blob.h"
#include <string>

Blob<256> pbkdf2(const std::string& password, const std::string& salt, int iterations);
