//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "secRandom.h"
#include <openssl/rand.h>

bool secRandom(void* ptr, int bytes) {
	int res = RAND_bytes((uint8_t*)ptr, bytes);
	if (res != 1) {
		return false;
	}
	return true;
}
