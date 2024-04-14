//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "cryptography/ecc.h"
#include <string>

class KeyStore {
public:
	void create(const std::string& file);
	void load(const std::string& file);
	void loadOrCreate(const std::string& file);

	EccPublicKey getPublicKey();
	EccPrivateKey getPrivateKey();

private:
	std::string file;
	EccPrivateKey priv;
	EccPublicKey pub;
};
