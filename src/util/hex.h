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

static void toHex(const void* data, int dataBytes, std::string& result) {
	result.resize(dataBytes * 2);
	for (int i = 0; i < dataBytes; i++) {
		uint8_t byte = *((uint8_t*)data + i);
		uint8_t v1 = (byte >> 0) & 0xf;
		uint8_t v2 = (byte >> 4) & 0xf;
		result[i * 2 + 0] = toHex(v1);
		result[i * 2 + 1] = toHex(v2);
	}
}

template<typename T>
void toHex(const T& data, std::string& result) {
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

static bool fromHex(const std::string& data, std::string& result) {
	int size = data.size() / 2;
	result.resize(size);
	for (int i = 0; i < size; i++) {
		uint8_t v1 = fromHex(data[i * 2 + 0]);
		uint8_t v2 = fromHex(data[i * 2 + 1]);
		if (v1 == -1 || v2 == -1) {
			return false;
		}
		uint8_t byte = (v2 << 4) | (v1 << 0);
		result[i] = byte;
	}

	for (int i = size; i < sizeof(result); i++) {
		result[i] = 0;
	}
	return true;
}

template<typename T>
bool fromHex(const std::string& data, T& result) {
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

static void swapNibbles(std::string& data) {
	int size = data.size() / 2;
	for (int i = 0; i < size; i++) {
		uint8_t v1 = data[i * 2 + 0];
		uint8_t v2 = data[i * 2 + 1];
		data[i * 2 + 0] = v2;
		data[i * 2 + 1] = v1;
	}
}

static void swapEndianness(std::string& data, int swapElementSize = 2) {
	int size = data.size() / swapElementSize;
	for (int i = 0; i < size / 2; i++) {
		int j = size - 1 - i;
		for (int k = 0; k < swapElementSize; k++) {
			uint8_t v1 = data[i * swapElementSize + k];
			uint8_t v2 = data[j * swapElementSize + k];
			data[i * swapElementSize + k] = v2;
			data[j * swapElementSize + k] = v1;
		}
	}
}
