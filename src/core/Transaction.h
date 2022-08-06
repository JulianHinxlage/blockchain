#pragma once

#include "type.h"
#include <vector>

class Transaction {
public:
	uint16_t version;
	uint64_t timestamp;
	EccPublicKey from;
	EccPublicKey to;
	Amount amount;
	Amount fee;
	Hash dataHash;
	uint32_t nonce;
	EccSignature signature;

	Transaction();
	Transaction(Transaction& transaction);
	Transaction &operator=(Transaction &transaction);

	bool serial(std::ostream& stream);
	bool deserial(std::istream &stream);
	bool signSerial(std::ostream& stream);
	Hash getHash();
};

class FullTransaction {
public:
	Transaction transaction;
	Hash transactionHash;
	std::vector<uint8_t> data;

	bool serial(std::ostream& stream);
	bool deserial(std::istream& stream);
	Hash getHash();
};
