
#include "sha.h"
#include <openssl/sha.h>
#include <openssl/evp.h>

Blob<256> sha256(const void* msg, int bytes) {
	Blob<256> hash;
	SHA256((unsigned char*)msg, bytes, (unsigned char*)&hash);
	return hash;
}

Blob<256> sha256(const std::string& msg) {
	return sha256(msg.data(), msg.size());
}

Blob<512> sha512(const void* msg, int bytes) {
	Blob<512> hash;
	SHA512((unsigned char*)msg, bytes, (unsigned char*)&hash);
	return hash;
}

Blob<512> sha512(const std::string& msg) {
	return sha512(msg.data(), msg.size());
}

Blob<256> sha3_256(const void* msg, int bytes) {
	Blob<256> hash;
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	unsigned int len = sizeof(hash);
	EVP_Digest(msg, bytes, (unsigned char*)&hash, &len, EVP_sha3_256(), nullptr);
	EVP_MD_CTX_free(ctx);
	return hash;
}

Blob<256> sha3_256(const std::string& msg) {
	return sha3_256(msg.data(), msg.size());
}

Blob<512> sha3_512(const void* msg, int bytes) {
	Blob<512> hash;
	EVP_MD_CTX* ctx = EVP_MD_CTX_new();
	unsigned int len = sizeof(hash);
	EVP_Digest(msg, bytes, (unsigned char*)&hash, &len, EVP_sha3_512(), nullptr);
	EVP_MD_CTX_free(ctx);
	return hash;
}

Blob<512> sha3_512(const std::string& msg) {
	return sha3_512(msg.data(), msg.size());
}
