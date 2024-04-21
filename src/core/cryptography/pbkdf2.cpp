//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "pbkdf2.h"
#include <openssl/evp.h>

Blob<256> pbkdf2(const std::string& password, const std::string& salt, int iterations) {
	Blob<256> key;
	PKCS5_PBKDF2_HMAC(password.data(), password.size(), (uint8_t*)salt.data(), salt.size(), iterations, EVP_sha256(), sizeof(key), (uint8_t*)&key);
	return key;
}
