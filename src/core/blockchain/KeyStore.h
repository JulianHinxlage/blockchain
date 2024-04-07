//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "cryptography/ecc.h"
#include <string>

class KeyStore {
public:
	void createKey(const std::string& file);
	void loadKey(const std::string& file);
	void createOrLoadKey(const std::string& file);

	EccPublicKey getPublicKey();
	EccPrivateKey getPrivateKey();

private:
	std::string file;
	EccPrivateKey priv;
	EccPublicKey pub;
};
