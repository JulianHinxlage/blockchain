#pragma once

#include <cstdint>

template<int> struct BlobWord {};
template<> struct BlobWord<0> { typedef uint8_t Type; };
template<> struct BlobWord<1> { typedef uint16_t Type; };
template<> struct BlobWord<2> { typedef uint32_t Type; };
template<> struct BlobWord<3> { typedef uint64_t Type; };

template<int bitCount>
struct Blob {
public:
	static const int byteCount = ((bitCount - 1) / 8) + 1;
	typedef typename BlobWord<(byteCount % 2 == 0) + (byteCount % 4 == 0) + (byteCount % 8 == 0)>::Type Word;
	static const int wordCount = ((byteCount - 1) / sizeof(Word)) + 1;
	union {
		Word words[wordCount];
		uint8_t bytes[byteCount];
	};

    Blob() {}

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

    template<typename T>
    explicit Blob(const T& t) {
        operator=(t);
    }

    bool operator==(const Blob &blob) const {
        for (int i = 0; i < wordCount; i++) {
            if (words[i] != blob.words[i]) {
                return false;
            }
        }
        return true;
    }
};
