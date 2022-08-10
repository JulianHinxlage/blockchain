#pragma once

#include <random>

template<typename T>
void randomBytes(T& t) {
	static std::mt19937 mt;
	static bool init = false;
	if (!init) {
		mt.seed(std::random_device()());
		init = true;
	}

	using Word = unsigned int;
	int count = sizeof(T) / sizeof(Word);
	for (int i = 0; i < count; i++) {
		*(((Word*)&t) + i) = mt();
	}
}

template<typename T>
T randomBytes() {
	T t;
	randomBytes(t);
	return t;
}
