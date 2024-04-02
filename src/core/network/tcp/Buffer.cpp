//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#include "Buffer.h"
#include <cstring>

Buffer::Buffer() {
	dataPtr = nullptr;
	dataSize = 0;
	readIndex = 0;
	writeIndex = 0;
}

Buffer::Buffer(void* data, int bytes) {
	dataPtr = nullptr;
	dataSize = 0;
	readIndex = 0;
	writeIndex = 0;
	setData(data, bytes);
}

Buffer::Buffer(const Buffer& buffer) {
	dataPtr = nullptr;
	dataSize = 0;
	readIndex = 0;
	writeIndex = 0;
	writeBytes(buffer.data(), buffer.size());
}

void Buffer::writeBytes(const void* ptr, int bytes) {
	int left = dataSize - writeIndex;
	if (bytes > left) {
		reserve(writeIndex + bytes);
	}
	memcpy(dataPtr + writeIndex, ptr, bytes);
	writeIndex += bytes;
}

void Buffer::readBytes(void* ptr, int bytes) {
	int left = dataSize - readIndex;
	int min = bytes < left ? bytes : left;
	memcpy(ptr, dataPtr + readIndex, min);
	readIndex += min;
	memset((uint8_t*)ptr + min, 0, bytes - min);
}

void Buffer::skip(int bytes) {
	readIndex += bytes;
}

void Buffer::unskip(int bytes) {
	readIndex =  std::max(0, readIndex - bytes);
}

void Buffer::skipWrite(int bytes) {
	writeIndex += bytes;
}

void Buffer::unskipWrite(int bytes) {
	writeIndex = std::max(0, writeIndex - bytes);
}

void Buffer::reset() {
	readIndex = 0;
	writeIndex = 0;
}

uint8_t* Buffer::data() const {
	return dataPtr + readIndex;
}

int Buffer::size() const {
	return dataSize - readIndex;
}

uint8_t* Buffer::dataWrite() const {
	return dataPtr + writeIndex;
}

int Buffer::sizeWrite() const {
	return dataSize - writeIndex;
}

void Buffer::reserve(int bytes) {
	if (buffer.data() == dataPtr) {
		buffer.resize(bytes);
		dataSize = (int)buffer.size();
		dataPtr = buffer.data();
	}
	else {
		buffer.resize(bytes);
		memcpy(buffer.data(), dataPtr, dataSize);
		dataSize = (int)buffer.size();
		dataPtr = buffer.data();
	}
}

void Buffer::setData(void* data, int bytes) {
	dataPtr = (uint8_t*)data;
	dataSize = bytes;
	writeIndex = 0;
	readIndex = 0;
	buffer.clear();
}

void Buffer::clear() {
	dataPtr = nullptr;
	buffer.clear();
	dataSize = 0;
	readIndex = 0;
	writeIndex = 0;
}

int Buffer::getReadIndex() const {
	return readIndex;
}

int Buffer::getWriteIndex() const {
	return writeIndex;
}

bool Buffer::hasDataLeft() const {
    return readIndex < dataSize;
}

bool Buffer::isOwningData() const {
    return buffer.data() == dataPtr;
}

void Buffer::writeStr(const std::string& str) {
    writeBytes(str.c_str(), (int)str.size() + 1);
}

void Buffer::readStr(std::string& str) {
	str.clear();
    while (true) {
        char c = '\0';
        readBytes(&c, 1);
        if (c != '\0') {
            str.push_back(c);
        }
        else {
            break;
        }
    }
}

std::string Buffer::readStr() {
    std::string str;
    readStr(str);
    return str;
}

void Buffer::writeVarInt(const int64_t& value) {
	uint8_t* ptr = (uint8_t*)&value;
	if (value >= 0 && value < (1 << 7)) {
		writeBytes(&ptr[0], 1);
		return;
	}

	int readBit = 0;
	int writeBit = 0;
	int zeroBits = 0;
	uint8_t byte = 0;
	for (int i = 0; i < sizeof(value) * 8; i++) {
		uint8_t bit = (ptr[readBit / 8] >> (readBit % 8)) & 1u;
		readBit++;
		if (bit) {
			for (int j = 0; j < zeroBits; j++) {
				if (writeBit >= 7) {
					byte |= (1 << 7);
					writeBytes(&byte, 1);
					byte = 0;
					writeBit = 0;
				}

				writeBit++;
			}
			zeroBits = 0;

			if (writeBit >= 7) {
				byte |= (1 << 7);
				writeBytes(&byte, 1);
				byte = 0;
				writeBit = 0;
			}
			byte |= (1 << writeBit);
			writeBit++;
		}
		else {
			zeroBits++;
		}
	}

	if (writeBit >= 1 || zeroBits > 0) {
		writeBytes(&byte, 1);
		byte = 0;
		writeBit = 0;
	}
}

void Buffer::readVarInt(int64_t& value) {
	int readBit = 0;
	int writeBit = 0;
	uint8_t* ptr = (uint8_t*)&value;
	uint8_t byte = 0;

	while (true) {
		readBytes(&byte, 1);

		for (int i = 0; i < 7; i++) {
			uint8_t bit = (byte >> i) & 1u;

			if (writeBit < sizeof(value) * 8) {
				if (bit) {
					ptr[writeBit / 8] |= (1 << (writeBit % 8));
				}
				else {
					ptr[writeBit / 8] &= ~(1 << (writeBit % 8));
				}
				writeBit++;
			}
		}

		if (!((byte >> 7) & 1u)) {
			break;
		}
	}

}
