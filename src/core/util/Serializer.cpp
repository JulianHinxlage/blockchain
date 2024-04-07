//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "Serializer.h"

Serializer::Serializer() {}

Serializer::Serializer(uint8_t* data, int size) {
	dataPtr = data;
	dataSize = size;
	writeIndex = dataSize;
}

Serializer::Serializer(const std::string& str) {
	dataPtr = (uint8_t*)str.data();
	dataSize = str.size();
	buffer.resize(dataSize);
	for (int i = 0; i < dataSize; i++) {
		buffer[i] = dataPtr[i];
	}
	dataPtr = buffer.data();
	writeIndex = dataSize;
}

std::string Serializer::toString() {
	return std::string((char*)data(), size());
}

bool Serializer::hasDataLeft() {
	return readIndex < dataSize;
}

int Serializer::getReadIndex() {
	return readIndex;
}

void Serializer::skip(int bytes) {
	readIndex += bytes;
}

void Serializer::writeBytes(const uint8_t* data, int size) {
	int count = size;
	for (int i = 0; i < count; i++) {
		if (writeIndex >= dataSize) {
			if (writeIndex == 0) {
				buffer.resize(1);
			}
			else {
				if (dataPtr != buffer.data()) {
					buffer.resize(writeIndex);
					for (int i = 0; i < dataSize; i++) {
						buffer[i] = dataPtr[i];
					}
				}
				buffer.resize(writeIndex * 2);
			}
			dataPtr = buffer.data();
			dataSize = buffer.size();
		}
		dataPtr[writeIndex++] = data[i];
	}
}

void Serializer::readBytes(uint8_t* data, int size) {
	for (int i = 0; i < size; i++) {
		if (readIndex < dataSize) {
			data[i] = dataPtr[readIndex++];
		}
		else {
			data[i] = 0;
		}
	}
}

void Serializer::writeStr(const std::string& str) {
	write<int>(str.size());
	writeBytes((uint8_t*)str.data(), str.size());
}

void Serializer::readStr(std::string& str) {
	int size = read<int>();
	str.resize(size);
	readBytes((uint8_t*)str.data(), size);
}


uint8_t *Serializer::data() {
	return dataPtr;
}

int Serializer::size() {
	return writeIndex;
}
