//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include <random>

template<typename T>
T random() {
	static std::mt19937_64* rng = nullptr;
	if (rng == nullptr) {
		rng = new std::mt19937_64();
		rng->seed(std::random_device()());
	}
	T output = T();
	uint64_t value = rng->operator()();
	int j = 0;
	for (int i = 0; i < sizeof(T); i++) {
		if (j >= sizeof(value)) {
			value = rng->operator()();
			j = 0;
		}
		((uint8_t*)&output)[i] = ((uint8_t*)&value)[j++];
	}
	return output;
}
