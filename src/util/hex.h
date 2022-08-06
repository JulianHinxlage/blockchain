#pragma once

#include <string>

static uint8_t toHex(uint8_t v) {
	if (v < 16) {
		return "0123456789abcdef"[v];
	}
	else {
		return -1;
	}
}

static uint8_t fromHex(uint8_t v) {
	if (v >= '0' && v <= '9') {
		return v - '0';
	}
	else if (v >= 'a' && v <= 'f') {
		return v - 'a' + 10;
	}
	else {
		return -1;
	}
}

template<typename T>
void toHex(T& data, std::string& result) {
	int size = sizeof(data);
	result.resize(size * 2);
	for (int i = 0; i < size; i++) {
		uint8_t byte = *((uint8_t*)&data + i);
		uint8_t v1 = (byte >> 0) & 0xf;
		uint8_t v2 = (byte >> 4) & 0xf;
		result[i * 2 + 0] = toHex(v1);
		result[i * 2 + 1] = toHex(v2);
	}
}

template<typename T>
bool fromHex(std::string& data, T& result) {
	int size = data.size() / 2;
	for (int i = 0; i < size; i++) {
		uint8_t v1 = fromHex(data[i * 2 + 0]);
		uint8_t v2 = fromHex(data[i * 2 + 1]);
		if (v1 == -1 || v2 == -1) {
			return false;
		}
		uint8_t byte = (v2 << 4) | (v1 << 0);
		*((uint8_t*)&result + i) = byte;
	}

	for (int i = size; i < sizeof(result); i++) {
		*((uint8_t*)&result + i) = 0;
	}
	return true;
}
