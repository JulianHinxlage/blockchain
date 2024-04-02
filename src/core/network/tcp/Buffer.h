//
// Copyright (c) 2023 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <vector>
#include <string>

class Buffer {
public:
    Buffer();
    Buffer(void* data, int bytes);
    Buffer(const Buffer &buffer);

    void writeBytes(const void* ptr, int bytes);
    void readBytes(void* ptr, int bytes);

    void skip(int bytes);
    void unskip(int bytes);
    void skipWrite(int bytes);
    void unskipWrite(int bytes);

    void reset();
    uint8_t* data() const;
    int size() const;
    uint8_t* dataWrite() const;
    int sizeWrite() const;

    void reserve(int bytes);
    void setData(void* data, int bytes);
    void clear();
    int getReadIndex() const;
    int getWriteIndex() const;
    bool hasDataLeft() const;
    bool isOwningData() const;

    void writeStr(const std::string& str);
    void readStr(std::string& str);
    std::string readStr();

    template<typename T>
    void write(const T& value) {
        writeBytes(&value, sizeof(value));
    }

    template<typename T>
    void read(T& value) {
        readBytes(&value, sizeof(value));
    }

    template<typename T>
    T read() {
        T value;
        read<T>(value);
        return value;
    }

    void writeVarInt(const int64_t& value);
    void readVarInt(int64_t& value);

private:
    std::vector<uint8_t> buffer;
    uint8_t* dataPtr;
    int dataSize;
    int readIndex;
    int writeIndex;
};
