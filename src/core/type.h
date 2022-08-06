#pragma once

#include "util/Blob.h"
#include "cryptography/ecc.h"
#include <cstdint>
#include <xhash>

typedef uint64_t Amount;
typedef Blob<256> Hash;

namespace std {
	template<> struct hash<Hash> {
		size_t operator()(const Hash& hash) const {
			return (size_t)hash;
		}
	};
	template<> struct hash<EccPublicKey> {
		size_t operator()(const EccPublicKey& pub) const {
			return (size_t)pub;
		}
	};
}
