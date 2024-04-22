//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <string>

static uint8_t hexCharToInt(char hex) {
	if (hex >= '0' && hex <= '9') {
		return hex - '0';
	}
	else if (hex >= 'a' && hex <= 'f') {
		return hex - 'a' + 10;
	}
	else if (hex >= 'A' && hex <= 'F') {
		return hex - 'A' + 10;
	}
	return 0;
}

static char intToHexChar(uint8_t value) {
	return "0123456789abcdef"[value & 0x0f];
}

static std::string toHex(uint8_t* ptr, int size) {
	std::string str;
	for (int i = 0; i < size; i++) {
		str += intToHexChar((ptr[i] >> 4) & 0xf);
		str += intToHexChar((ptr[i] >> 0) & 0xf);
	}
	return str;
}

template<typename T>
static std::string toHex(const T& t) {
	return toHex((uint8_t*)&t, sizeof(T));
}

template<typename T>
static T fromHex(const std::string& str) {
	T t;
	for (int i = 0; i < str.size(); i += 2) {
		uint8_t v = 0;
		v |= hexCharToInt(str[i]) << 4;
		v |= hexCharToInt(str[i + 1]) << 0;
		((uint8_t*)&t)[i / 2] = v;
	}
	return t;
}

static std::string stringToHex(const std::string &str) {
	return toHex((uint8_t*)str.data(), str.size());
}

static std::string stringfromHex(const std::string& str) {
	std::string result;
	for (int i = 0; i < str.size(); i += 2) {
		uint8_t v = 0;
		v |= hexCharToInt(str[i]) << 4;
		v |= hexCharToInt(str[i + 1]) << 0;
		result.push_back(v);
	}
	return result;
}