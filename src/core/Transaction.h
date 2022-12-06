#pragma once

#include "type.h"
#include <vector>
#include <fstream>

enum class TransactionError {
	NONE,
	INVALID_VERSION,
	INVALID_NONCE,
	INVALID_SIGNATURE,
	INVALID_BALANCE,
	INVALID_FEE,
	INVALID_KEY,
	INVALID_HASH,
	INVALID_FROM,
	INVALID_TO,
};

class TransactionHeader {
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

	TransactionHeader();
	TransactionHeader(const TransactionHeader& transaction);
	TransactionHeader &operator=(const TransactionHeader &transaction);

	bool serial(std::ostream& stream) const;
	bool deserial(std::istream &stream);
	bool signSerial(std::ostream& stream) const;
	Hash getHash() const;
	void sign(const EccPrivateKey& key);
	bool verifySignature() const;
};

class Transaction {
public:
	TransactionHeader header;
	Hash transactionHash;
	std::vector<uint8_t> data;

	Transaction();
	bool serial(std::ostream& stream) const;
	bool deserial(std::istream& stream);
	Hash getHash() const;
};
