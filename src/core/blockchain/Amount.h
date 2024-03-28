//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <cstdint>
#include <string>

typedef uint64_t Amount;

#define AMOUNT_DECIMAL_PLACES 9

std::string amountToCoin(Amount amount);

Amount coinToAmount(const std::string& str);
