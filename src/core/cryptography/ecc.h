//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "util/Blob.h"

typedef Blob<256> EccPrivateKey;
typedef Blob<264> EccPublicKey;
typedef Blob<576> EccSignature;

void eccGenerate(EccPrivateKey &privateKey, EccPublicKey &publicKey);
EccPublicKey eccToPublicKey(const EccPrivateKey &privateKey);
bool eccValidPublicKey(const EccPublicKey& publicKey);
EccSignature eccCreateSignature(const void* msg, int bytes, const EccPrivateKey &privateKey);
bool eccVerifySignature(const void* msg, int bytes, const EccPublicKey &publicKey, const EccSignature &signature);
