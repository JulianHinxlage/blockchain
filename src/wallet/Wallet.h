#pragma once

#include "core/Transaction.h"
#include "core/Block.h"

class Wallet {
public:
	EccPrivateKey privateKey;
	EccPublicKey publicKey;
	std::string file;

	Wallet();
	bool init(const std::string& file);
	void createKey();

    std::string selectFile(const std::string& directory);
	std::string selectDirectory(const std::string& directory);
};
