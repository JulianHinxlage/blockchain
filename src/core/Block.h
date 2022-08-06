#pragma once

#include "Transaction.h"
#include <unordered_map>

class Block {
public:
	uint16_t version;
	uint64_t timestamp;
	Hash previousBlock;
	uint64_t blockNumber;
	EccPublicKey beneficiary;
	EccPublicKey validator;
	Hash transactionRoot;
	Hash stateRoot;
	EccSignature signature;

	Block();

	bool serial(std::ostream& stream);
	bool deserial(std::istream& stream);
	bool signSerial(std::ostream& stream);
	Hash getHash();
};

class TransactionTree {
public:
	std::vector<Hash> transactionHashes;

	bool serial(std::ostream& stream);
	bool deserial(std::istream& stream);
	Hash getHash();
};

class FullBlock {
public:
	Block block;
	Hash blockHash;
	TransactionTree transactionTree;

	bool serial(std::ostream& stream);
	bool deserial(std::istream& stream);
	Hash getHash();
};
