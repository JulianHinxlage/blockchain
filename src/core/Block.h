#pragma once

#include "Transaction.h"
#include <unordered_map>


enum class BlockError {
	NONE,
	INVALID_VERSION,
	INVALID_PREVIOUS,
	INVALID_BLOCK_NUMBER,
	INVALID_SIGNATURE,
	INVALID_STATE,
	INVALID_HASH,
	INVALID_BENEFICIARY,
	INVALID_VALIDATOR,
	INVALID_TRANSACTION_ROOT,
	INVALID_KEY,
	INVALID_FEE,
	INVALID_TRANSACTION,
};

class BlockHeader {
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

	BlockHeader();

	bool serial(std::ostream& stream) const;
	bool deserial(std::istream& stream);
	bool signSerial(std::ostream& stream) const;
	Hash getHash() const;
	void sign(const EccPrivateKey& key);
	bool verifySignature() const;
};

class TransactionTree {
public:
	std::vector<Hash> transactionHashes;

	bool serial(std::ostream& stream) const;
	bool deserial(std::istream& stream);
	Hash getHash() const;
};

class Block {
public:
	BlockHeader header;
	Hash blockHash;
	TransactionTree transactionTree;

	Block();
	bool serial(std::ostream& stream) const;
	bool deserial(std::istream& stream);
	Hash getHash() const;
};
