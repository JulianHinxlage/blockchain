#include "Buffer.h"

Buffer::Buffer() {
    readIndex = 0;
    writeIndex = 0;
}

void Buffer::reserve(int bytes) {
    buffer.reserve(bytes);
}

void Buffer::resize(int bytes, uint8_t value) {
    buffer.resize(bytes + readIndex, value);
    writeIndex = bytes;
}

void Buffer::add(const void* ptr, int bytes) {
    for (int i = 0; i < bytes; i++) {
        if (buffer.size() > writeIndex) {
            buffer[writeIndex++] = ((uint8_t*)ptr)[i];
        }
        else {
            buffer.push_back(((uint8_t*)ptr)[i]);
            writeIndex++;
        }
    }
}

void Buffer::get(void* ptr, int bytes) {
    for (int i = 0; i < bytes; i++) {
        if (readIndex < writeIndex) {
            ((uint8_t*)ptr)[i] = buffer[readIndex++];
        }
        else {
            ((uint8_t*)ptr)[i] = 0;
        }
    }
}

std::string Buffer::getStr() {
    std::string str;
    while (readIndex < writeIndex) {
        if (buffer[readIndex] == '\0') {
            readIndex++;
            break;
        }
        else {
            str.push_back(buffer[readIndex++]);
        }
    }
    return str;
}

void Buffer::addStr(const std::string& str) {
    for (int i = 0; i < str.size(); i++) {
        if (buffer.size() > writeIndex) {
            buffer[writeIndex++] = str[i];
        }
        else {
            buffer.push_back(str[i]);
            writeIndex++;
        }
    }
    if (buffer.size() > writeIndex) {
        buffer[writeIndex++] = '\0';
    }
    else {
        buffer.push_back('\0');
        writeIndex++;
    }
}

void Buffer::skip(int bytes) {
    readIndex = std::min(writeIndex, readIndex + bytes);
}

void Buffer::unskip(int bytes) {
    readIndex = std::max(0, readIndex - bytes);
}

uint8_t* Buffer::data() {
    return buffer.data() + readIndex;
}

int Buffer::size() {
    return writeIndex - readIndex;
}

std::string Buffer::getRemaining() {
    return std::string((char*)buffer.data() + readIndex, writeIndex - readIndex);
}

std::string Buffer::toString() {
    return std::string((char*)buffer.data(), writeIndex);
}
