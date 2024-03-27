//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <cstdint>

template<int bitCount>
struct Blob {
public:
	static const int byteCount = ((bitCount - 1) / 8) + 1;
	uint8_t bytes[byteCount];

    Blob() {
        for (int i = 0; i < byteCount; i++) {
            bytes[i] = 0;
        }
    }

    template<typename T>
    Blob(const T& t) {
        operator=(t);
    }

    template<typename T>
    Blob& operator=(const T& t) {
        int size = sizeof(*this) < sizeof(t) ? sizeof(*this) : sizeof(t);
        for (int i = 0; i < size; i++) {
            bytes[i] = ((uint8_t*)&t)[i];
        }
        for (int i = size; i < sizeof(*this); i++) {
            bytes[i] = 0;
        }
        return *this;
    }

    template<typename T>
    explicit operator T() const {
        T t;
        int size = sizeof(*this) < sizeof(t) ? sizeof(*this) : sizeof(t);
        for (int i = 0; i < size; i++) {
            ((uint8_t*)&t)[i] = bytes[i];
        }
        for (int i = size; i < sizeof(t); i++) {
            ((uint8_t*)&t)[i] = 0;
        }
        return t;
    }

    bool operator==(const Blob& blob) const {
        for (int i = 0; i < byteCount; i++) {
            if (bytes[i] != blob.bytes[i]) {
                return false;
            }
        }
        return true;
    }

    bool operator!=(const Blob& blob) const {
        return !operator==(blob);
    }

    bool operator<(const Blob& blob) const {
        for (int i = byteCount - 1; i >= 0; i--) {
            if (bytes[i] < blob.bytes[i]) {
                return true;
            }
            else if (bytes[i] > blob.bytes[i]) {
                return false;
            }
        }
        return false;
    }

    bool operator>(const Blob& blob) const {
        for (int i = byteCount - 1; i >= 0; i--) {
            if (bytes[i] > blob.bytes[i]) {
                return true;
            }
            else if (bytes[i] < blob.bytes[i]) {
                return false;
            }
        }
        return false;
    }

    bool operator<=(const Blob& blob) const {
        return !operator>(blob);
    }

    bool operator>=(const Blob& blob) const {
        return !operator<(blob);
    }

    Blob operator&(const Blob& blob) const {
        Blob result;
        for (int i = 0; i < byteCount; i++) {
            result.bytes[i] = bytes[i] & blob.bytes[i];
        }
        return result;
    }

    Blob operator|(const Blob& blob) const {
        Blob result;
        for (int i = 0; i < byteCount; i++) {
            result.bytes[i] = bytes[i] | blob.bytes[i];
        }
        return result;
    }

    Blob operator^(const Blob& blob) const {
        Blob result;
        for (int i = 0; i < byteCount; i++) {
            result.bytes[i] = bytes[i] ^ blob.bytes[i];
        }
        return result;
    }

    Blob operator~() const {
        Blob result;
        for (int i = 0; i < byteCount; i++) {
            result.bytes[i] = ~bytes[i];
        }
        return result;
    }

    Blob& operator&=(const Blob& blob) {
        *this = *this & blob;
        return *this;
    }

    Blob& operator|=(const Blob& blob) {
        *this = *this | blob;
        return *this;
    }

    Blob& operator^=(const Blob& blob) {
        *this = *this ^ blob;
        return *this;
    }

    Blob operator<<(int shift) {
        Blob result(0);
        int byteShift = shift / 8;
        int bitShift = shift % 8;
        for (int i = 0; i < byteCount; i++) {
            if (i - byteShift >= 0) {
                if (bitShift == 0) {
                    result.bytes[i] = bytes[i - byteShift];
                }
                else {
                    result.bytes[i] = (bytes[i - byteShift] << bitShift);
                    if (i - byteShift - 1 >= 0) {
                        result.bytes[i] |= (bytes[i - byteShift - 1] >> (8 - bitShift));
                    }
                }
            }
        }
        return result;
    }

    Blob operator>>(int shift) {
        Blob result(0);
        int byteShift = shift / 8;
        int bitShift = shift % 8;
        for (int i = 0; i < byteCount; i++) {
            if (i + byteShift < byteCount) {
                if (bitShift == 0) {
                    result.bytes[i] = bytes[i + byteShift];
                }
                else {
                    result.bytes[i] = (bytes[i + byteShift] >> bitShift);
                    if (i + byteShift + 1 < byteCount) {
                        result.bytes[i] |= (bytes[i + byteShift + 1] << (8 - bitShift));
                    }
                }
            }
        }
        return result;
    }
};
