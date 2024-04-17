//
// Copyright (c) 2024 Julian Hinxlage. All rights reserved.
//

#pragma once

#include "Transaction.h"
#include <vector>

class BlockHeader {
public:
	uint32_t version = 0;
	uint32_t transactionCount = 0;
	uint64_t timestamp = 0;
	uint64_t blockNumber = 0;
	Amount totalStakeAmount = 0;
	Hash previousBlockHash = 0;
	EccPublicKey validator = 0;
	EccPublicKey beneficiary = 0;
	uint32_t slot = 0;
	Hash seed = 0;
	Hash transactionTreeRoot = 0;
	Hash accountTreeRoot = 0;
	Hash validatorTreeRoot = 0;
	EccSignature signature = 0;

	Hash caclulateHash() const;
	void sign(const EccPrivateKey& privateKey);
	bool verifySignature() const;

	std::string serial() const;
	int deserial(const std::string& str);
};

class TransactionTree {
public:
	std::vector<Hash> transactionHashes;
	Hash calculateRoot() const;
};

class Block {
public:
	BlockHeader header;
	TransactionTree transactionTree;
	Hash blockHash = 0;

	std::string serial() const;
	int deserial(const std::string &str);
};
