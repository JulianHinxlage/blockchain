//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <string>
#include <vector>

class Serializer {
public:
	Serializer();
	Serializer(uint8_t* data, int size);
	Serializer(const std::string& str);
	std::string toString();

	bool hasDataLeft();
	int getReadIndex();
	void skip(int bytes);

	void writeBytes(const uint8_t* data, int size);
	void readBytes(uint8_t* data, int size);
	void writeStr(const std::string &str);
	void readStr(std::string& str);

	template<typename T>
	void write(const T& t) {
		writeBytes((uint8_t*)&t, sizeof(T));
	}

	template<typename T>
	void read(T& t) {
		readBytes((uint8_t*)&t, sizeof(T));
	}

	template<typename T>
	T read() {
		T t;
		readBytes((uint8_t*)&t, sizeof(T));
		return t;
	}

	std::string readAll();

	uint8_t *data();
	int size();

private:
	std::vector<uint8_t> buffer;
	uint8_t* dataPtr = nullptr;
	int dataSize = 0;
	int readIndex = 0;
	int writeIndex = 0;
};
