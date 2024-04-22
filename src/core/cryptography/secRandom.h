//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

bool secRandom(void* ptr, int bytes);

template<typename T>
T secRandom() {
	T t;
	secRandom((void*)&t, sizeof(t));
	return t;
}
