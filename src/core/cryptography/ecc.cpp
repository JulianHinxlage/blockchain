//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#include "ecc.h"
#include <openssl/evp.h>
#include <openssl/ec.h>

static int eccNid = NID_secp256k1;
static auto eccHash = EVP_sha256();

void eccGenerate(EccPrivateKey& privateKey, EccPublicKey& publicKey) {
	EC_KEY* key = EC_KEY_new_by_curve_name(eccNid);
	EC_KEY_generate_key(key);
	EC_POINT_point2oct(EC_KEY_get0_group(key), EC_KEY_get0_public_key(key), POINT_CONVERSION_COMPRESSED, (uint8_t*)&publicKey, sizeof(publicKey), nullptr);
	EC_KEY_priv2oct(key, (uint8_t*)&privateKey, sizeof(privateKey));
	EC_KEY_free(key);
}

EccPublicKey eccToPublicKey(const EccPrivateKey& privateKey) {
    EccPublicKey publicKey;
    EC_KEY* key = EC_KEY_new_by_curve_name(eccNid);
    EC_KEY_oct2priv(key, (unsigned char*)&privateKey, sizeof(privateKey));

    auto group = EC_KEY_get0_group(key);
    EC_POINT* p = EC_POINT_new(group);
    EC_POINT_mul(group, p, EC_KEY_get0_private_key(key), nullptr, nullptr, nullptr);
    EC_KEY_set_public_key(key, p);
    if (EC_KEY_check_key(key) == 1) {
        EC_POINT_point2oct(EC_KEY_get0_group(key), EC_KEY_get0_public_key(key), POINT_CONVERSION_COMPRESSED, (unsigned char*)&publicKey, sizeof(publicKey), nullptr);
    }

    EC_POINT_free(p);
    EC_KEY_free(key);
    return publicKey;
}

bool eccValidPublicKey(const EccPublicKey& publicKey) {
    EC_KEY* key = EC_KEY_new_by_curve_name(eccNid);
    EC_POINT* p = EC_POINT_new(EC_KEY_get0_group(key));
    EC_POINT_oct2point(EC_KEY_get0_group(key), p, (unsigned char*)&publicKey, sizeof(publicKey), nullptr);
    EC_KEY_set_public_key(key, p);
    bool valid = EC_KEY_check_key(key) == 1;
    EC_POINT_free(p);
    EC_KEY_free(key);
    return valid;
}

EccSignature eccCreateSignature(const void* msg, int bytes, const EccPrivateKey& privateKey) {
    EccSignature signature;

    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_PKEY* pkey = EVP_PKEY_new();
    EC_KEY* key = EC_KEY_new_by_curve_name(eccNid);

    EC_KEY_oct2priv(key, (unsigned char*)&privateKey, sizeof(privateKey));
    EVP_PKEY_assign_EC_KEY(pkey, key);

    bool valid = false;
    if (EVP_DigestSignInit(ctx, nullptr, eccHash, nullptr, pkey) == 1) {
        if (EVP_DigestSignUpdate(ctx, msg, bytes) == 1) {
            size_t len = sizeof(signature);
            if (EVP_DigestSignFinal(ctx, (unsigned char*)&signature, &len) == 1) {
                valid = true;
            }
        }
    }

    EVP_PKEY_free(pkey);
    EVP_MD_CTX_free(ctx);
    return signature;
}

bool eccVerifySignature(const void* msg, int bytes, const EccPublicKey& publicKey, const EccSignature& signature) {
    EVP_MD_CTX* ctx = EVP_MD_CTX_new();
    EVP_PKEY* pkey = EVP_PKEY_new();
    EC_KEY* key = EC_KEY_new_by_curve_name(eccNid);

    const EC_GROUP* group = EC_KEY_get0_group(key);
    EC_POINT* p = EC_POINT_new(group);
    EC_POINT_oct2point(group, p, (unsigned char*)&publicKey, sizeof(publicKey), nullptr);
    EC_KEY_set_public_key(key, p);
    EVP_PKEY_assign_EC_KEY(pkey, key);

    bool valid = false;
    if (EC_KEY_check_key(key)) {
        if (EVP_DigestVerifyInit(ctx, nullptr, eccHash, nullptr, pkey) == 1) {
            if (EVP_DigestVerifyUpdate(ctx, msg, bytes) == 1) {
                size_t len = signature.bytes[1] + 2;
                if (len <= sizeof(signature)) {
                    if (EVP_DigestVerifyFinal(ctx, (unsigned char*)&signature, len) == 1) {
                        valid = true;
                    }
                }
            }
        }
    }

    EC_POINT_free(p);
    EVP_PKEY_free(pkey);
    EVP_MD_CTX_free(ctx);
    return valid;
}