//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "aes.h"
#include <openssl/evp.h>

bool aesEncrypt(const std::string& clearText, std::string& chiperText, Blob<256> key, Blob<256> iv, Blob<128> &tag) {
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx) {
		return false;
	}

	int res = EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
	if (res != 1) {
		return false;
	}

	res = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, sizeof(iv), nullptr);
	if (res != 1) {
		return false;
	}

	res = EVP_EncryptInit_ex(ctx, nullptr, nullptr, (uint8_t*)&key, (uint8_t*)&iv);
	if (res != 1) {
		return false;
	}

	chiperText.resize(clearText.size() + EVP_CIPHER_CTX_block_size(ctx));
	int len = chiperText.size();
	res = EVP_EncryptUpdate(ctx, (uint8_t*)chiperText.data(), &len, (uint8_t*)clearText.data(), clearText.size());
	if (res != 1) {
		return false;
	}
	
	int len2 = chiperText.size() - len;
	res = EVP_EncryptFinal(ctx, (uint8_t*)chiperText.data() + len, &len2);
	if (res != 1) {
		return false;
	}
	chiperText.resize(len + len2);

	res = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, sizeof(tag), &tag);
	if (res != 1) {
		return false;
	}

	EVP_CIPHER_CTX_free(ctx);
	return true;
}

bool aesDecrypt(const std::string& chiperText, std::string& clearText, Blob<256> key, Blob<256> iv, Blob<128> tag) {
	EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
	if (!ctx) {
		return false;
	}

	int res = EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), nullptr, nullptr, nullptr);
	if (res != 1) {
		return false;
	}

	res = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, sizeof(iv), nullptr);
	if (res != 1) {
		return false;
	}

	res = EVP_DecryptInit_ex(ctx, nullptr, nullptr, (uint8_t*)&key, (uint8_t*)&iv);
	if (res != 1) {
		return false;
	}

	clearText.resize(chiperText.size() + EVP_CIPHER_CTX_block_size(ctx));
	int len = clearText.size();
	res = EVP_DecryptUpdate(ctx, (uint8_t*)clearText.data(), &len, (uint8_t*)chiperText.data(), chiperText.size());
	if (res != 1) {
		return false;
	}

	res = EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, sizeof(tag), &tag);
	if (res != 1) {
		return false;
	}

	int len2 = clearText.size() - len;
	res = EVP_DecryptFinal(ctx, (uint8_t*)clearText.data() + len, &len2);
	clearText.resize(len + len2);
	EVP_CIPHER_CTX_free(ctx);

	return res > 0;
}
