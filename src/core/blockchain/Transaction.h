//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "type.h"
#include "cryptography/ecc.h"
#include <vector>
#include <string>

class TransactionHeader {
public:
	uint32_t version = 0;
	uint32_t transactionNumber = 0;
	uint64_t timestamp = 0;
	EccPublicKey sender = 0;
	EccPublicKey recipient = 0;
	uint64_t amount = 0;
	uint64_t fee = 0;
	Hash dataHash = 0;
	EccSignature signature = 0;

	Hash caclulateHash() const;
	void sign(const EccPrivateKey& privateKey);
	bool verifySignature() const;
	std::string serial() const;
	int deserial(const std::string& str);
};

class Transaction {
public:
	TransactionHeader header;
	std::vector<uint8_t> data;
	Hash transactionHash;

	std::string serial() const;
	int deserial(const std::string& str);
};
