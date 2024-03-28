//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Amount.h"

std::string amountToCoin(Amount amount) {
	std::string str;
	while (amount > 0) {
		str = std::to_string(amount % 10) + str;
		amount /= 10;
	}
	while (str.size() < AMOUNT_DECIMAL_PLACES + 1) {
		str = "0" + str;
	}
	str.insert(str.begin() + str.size() - AMOUNT_DECIMAL_PLACES, '.');
	while (str.size() > 0 && str.back() == '0') {
		str.erase(str.begin() + str.size() - 1);
	}
	if (str.size() > 0 && str.back() == '.') {
		str.erase(str.begin() + str.size() - 1);
	}
	return str;
}

Amount coinToAmount(const std::string& str) {
	int pointPos = -1;
	for (int i = 0; i < str.size(); i++) {
		if (str[i] == '.') {
			if (pointPos != -1) {
				return 0;
			}
			pointPos = i;
		}
	}
	if (pointPos == -1) {
		pointPos = str.size();
	}

	Amount amount = 0;
	Amount factor = 1;
	for (int i = 0; i < AMOUNT_DECIMAL_PLACES + pointPos; i++) {
		int index = pointPos + AMOUNT_DECIMAL_PLACES - i;
		if (i >= AMOUNT_DECIMAL_PLACES) {
			index--;
		}
		int value = 0;
		if (index >= 0 && index < str.size()) {
			char c = str[index];
			if (c >= '0' && c <= '9') {
				value = c - '0';
			}
			else {
				return 0;
			}
		}

		amount += value * factor;
		factor *= 10;
	}
	return amount;
}
